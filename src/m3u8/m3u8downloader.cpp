#include "m3u8downloader.h"
#include "../ffmpeg/ffmpegmanager.h"

#include <QFileInfo>
#include <QDateTime>
#include <QStandardPaths>
#include <QProcess>
#include <QTextStream>

M3U8Downloader::M3U8Downloader(QObject *parent)
    : QObject(parent),
      m_reply(nullptr),
      m_currentIndex(0),
      m_downloadedBytes(0),
      m_totalBytes(0),
      m_paused(false),
      m_cancelled(false),
      m_speedBytes(0)
{
}

void M3U8Downloader::start(const QString &url, const QString &outputPath)
{
    cancel();

    m_playlistUrl = url;
    m_outputPath = outputPath;
    m_currentIndex = 0;
    m_downloadedBytes = 0;
    m_totalBytes = 0;
    m_paused = false;
    m_cancelled = false;

    QString base =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_tempDir = base + "/VideoDownloader_" +
                QString::number(QDateTime::currentMSecsSinceEpoch());

    QDir().mkpath(m_tempDir);

    emit status("正在读取 M3U8...");
    requestPlaylist(QUrl(url));
}

void M3U8Downloader::requestPlaylist(const QUrl &url)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      "VideoDownloader/1.0");

    m_reply = m_manager.get(request);

    connect(m_reply, &QNetworkReply::finished,
            this, &M3U8Downloader::onPlaylistFinished);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_reply, &QNetworkReply::errorOccurred,
            this, &M3U8Downloader::onError);
#else
    connect(m_reply,
            static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this,
            &M3U8Downloader::onError);
#endif
}

void M3U8Downloader::onPlaylistFinished()
{
    if (!m_reply)
        return;

    QByteArray data = m_reply->readAll();

    if (m_reply->error() != QNetworkReply::NoError) {
        fail(m_reply->errorString());
        m_reply->deleteLater();
        m_reply = nullptr;
        return;
    }

    QUrl currentUrl(m_playlistUrl);
    if (m_currentIndex == 0)
        currentUrl = m_reply->url();

    M3U8Playlist playlist = M3U8Parser::parse(data, currentUrl);

    m_reply->deleteLater();
    m_reply = nullptr;

    if (!playlist.valid) {
        fail("不是有效的 M3U8 播放列表");
        return;
    }

    if (playlist.encrypted) {
        fail("检测到 #EXT-X-KEY，加密 M3U8 当前不处理");
        return;
    }

    if (playlist.master) {
        if (playlist.variants.isEmpty()) {
            fail("Master Playlist 没有可用清晰度");
            return;
        }

        int best = 0;
        for (int i = 1; i < playlist.variants.size(); ++i) {
            if (playlist.variants[i].bandwidth >
                playlist.variants[best].bandwidth)
                best = i;
        }

        QString variantUrl = playlist.variants[best].url;
        emit status(QString("选择最高码率：%1").arg(
                        playlist.variants[best].bandwidth));

        m_playlistUrl = variantUrl;
        requestPlaylist(QUrl(variantUrl));
        return;
    }

    m_playlist = playlist;

    if (m_playlist.segments.isEmpty()) {
        fail("M3U8 中没有视频分片");
        return;
    }

    emit status(QString("共发现 %1 个分片").arg(m_playlist.segments.size()));

    m_speedTimer.restart();
    m_speedBytes = 0;

    startNextSegment();
}

void M3U8Downloader::startNextSegment()
{
    if (m_cancelled)
        return;

    if (m_paused)
        return;

    if (m_currentIndex >= m_playlist.segments.size()) {
        finishDownload();
        return;
    }

    const M3U8Segment &seg = m_playlist.segments.at(m_currentIndex);

    QString fileName =
        QString("%1/%2.seg")
        .arg(m_tempDir)
        .arg(m_currentIndex, 8, 10, QChar('0'));

    m_segmentFile.setFileName(fileName);
    if (!m_segmentFile.open(QIODevice::WriteOnly)) {
        fail("无法创建分片文件: " + fileName);
        return;
    }

    QNetworkRequest request(QUrl(seg.url));
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      "VideoDownloader/1.0");

    m_reply = m_manager.get(request);

    connect(m_reply, &QNetworkReply::readyRead,
            this, &M3U8Downloader::onSegmentReadyRead);
    connect(m_reply, &QNetworkReply::finished,
            this, &M3U8Downloader::onSegmentFinished);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_reply, &QNetworkReply::errorOccurred,
            this, &M3U8Downloader::onError);
#else
    connect(m_reply,
            static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this,
            &M3U8Downloader::onError);
#endif

    emit status(QString("下载分片 %1 / %2")
                .arg(m_currentIndex + 1)
                .arg(m_playlist.segments.size()));
}

void M3U8Downloader::onSegmentReadyRead()
{
    if (!m_reply)
        return;

    QByteArray data = m_reply->readAll();
    if (!data.isEmpty()) {
        m_segmentFile.write(data);
        m_downloadedBytes += data.size();
        m_speedBytes += data.size();
        emit progress(m_downloadedBytes, m_totalBytes);
    }
}

void M3U8Downloader::onSegmentFinished()
{
    if (!m_reply)
        return;

    onSegmentReadyRead();

    if (m_reply->error() != QNetworkReply::NoError) {
        QString err = m_reply->errorString();
        m_reply->deleteLater();
        m_reply = nullptr;
        m_segmentFile.close();
        fail(err);
        return;
    }

    m_segmentFile.flush();
    m_segmentFile.close();

    m_reply->deleteLater();
    m_reply = nullptr;

    if (m_speedTimer.elapsed() >= 1000) {
        emit speed(m_speedBytes * 1000 / m_speedTimer.elapsed());
        m_speedBytes = 0;
        m_speedTimer.restart();
    }

    ++m_currentIndex;
    startNextSegment();
}

void M3U8Downloader::finishDownload()
{
    emit status("分片下载完成，正在调用 FFmpeg 合并...");

    QString concatFile = m_tempDir + "/concat.txt";
    QFile f(concatFile);

    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fail("无法创建 FFmpeg concat 文件");
        return;
    }

    QTextStream out(&f);

    for (int i = 0; i < m_playlist.segments.size(); ++i) {
        QString seg =
            QDir(m_tempDir).absoluteFilePath(
                QString("%1.seg").arg(i, 8, 10, QChar('0')));

        QString escaped = seg;
        escaped.replace("'", "'\\''");

        out << "file '" << escaped << "'\n";
    }

    f.close();

    FFmpegManager ffmpeg;
    if (!ffmpeg.concat(concatFile, m_outputPath)) {
        fail(ffmpeg.lastError());
        return;
    }

    emit progress(1, 1);
    emit speed(0);
    emit status("M3U8 下载完成");
    emit finished(m_outputPath);

    cleanup();
}

void M3U8Downloader::fail(const QString &text)
{
    cleanup();
    emit error(text);
}

void M3U8Downloader::cleanup()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    if (m_segmentFile.isOpen())
        m_segmentFile.close();

    if (!m_tempDir.isEmpty())
        QDir(m_tempDir).removeRecursively();
}

void M3U8Downloader::pause()
{
    m_paused = true;

    if (m_reply)
        m_reply->abort();

    if (m_segmentFile.isOpen())
        m_segmentFile.close();

    emit status("M3U8 已暂停");
}

void M3U8Downloader::resume()
{
    if (!m_paused)
        return;

    m_paused = false;
    emit status("M3U8 继续下载");

    // 当前版本从当前分片重新开始；已完成分片保留。
    startNextSegment();
}

void M3U8Downloader::cancel()
{
    m_cancelled = true;
    cleanup();
}

void M3U8Downloader::onError(QNetworkReply::NetworkError)
{
    if (m_reply &&
        m_reply->error() != QNetworkReply::OperationCanceledError) {
        emit error(m_reply->errorString());
    }
}
