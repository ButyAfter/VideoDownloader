#include "httpdownloader.h"

#include <QFileInfo>
#include <QDir>
#include <QNetworkRequest>
#include <QDateTime>

HttpDownloader::HttpDownloader(QObject *parent)
    : QObject(parent),
      m_reply(nullptr),
      m_existingSize(0),
      m_paused(false),
      m_cancelled(false),
      m_speedBytes(0)
{
}

void HttpDownloader::start(const QString &url, const QString &outputPath)
{
    cancel();

    m_url = url;
    m_outputPath = outputPath;
    m_partPath = outputPath + ".part";
    m_paused = false;
    m_cancelled = false;

    beginRequest(true);
}

void HttpDownloader::beginRequest(bool tryResume)
{
    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    m_existingSize = 0;

    if (tryResume && QFile::exists(m_partPath)) {
        m_existingSize = QFileInfo(m_partPath).size();
    }

    QUrl urlObj(m_url);
    QNetworkRequest request(urlObj);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      "VideoDownloader/1.0");

    if (m_existingSize > 0)
        request.setRawHeader("Range",
                             QByteArray("bytes=") +
                             QByteArray::number(m_existingSize) + "-");

    QIODevice::OpenMode mode =
        (m_existingSize > 0) ? QIODevice::Append : QIODevice::WriteOnly;

    m_file.setFileName(m_partPath);
    if (!m_file.open(mode)) {
        emit error("无法打开临时文件: " + m_partPath);
        return;
    }

    m_reply = m_manager.get(request);

    connect(m_reply, &QNetworkReply::readyRead,
            this, &HttpDownloader::onReadyRead);
    connect(m_reply, &QNetworkReply::downloadProgress,
            this, &HttpDownloader::onProgress);
    connect(m_reply, &QNetworkReply::finished,
            this, &HttpDownloader::onFinished);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_reply, &QNetworkReply::errorOccurred,
            this, &HttpDownloader::onError);
#else
    connect(m_reply,
            static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this,
            &HttpDownloader::onError);
#endif

    m_speedTimer.restart();
    m_speedBytes = 0;

    emit status(m_existingSize > 0
                ? QString("开始断点续传，已完成 %1 字节").arg(m_existingSize)
                : "开始下载");
}

void HttpDownloader::onReadyRead()
{
    if (!m_reply)
        return;

    QByteArray data = m_reply->readAll();
    if (!data.isEmpty()) {
        m_file.write(data);
        m_speedBytes += data.size();
    }
}

void HttpDownloader::onProgress(qint64 received, qint64 total)
{
    qint64 actualReceived = m_existingSize + received;
    qint64 actualTotal = total > 0 ? m_existingSize + total : total;

    emit progress(actualReceived, actualTotal);

    if (m_speedTimer.elapsed() >= 1000) {
        emit speed(m_speedBytes * 1000 / m_speedTimer.elapsed());
        m_speedBytes = 0;
        m_speedTimer.restart();
    }
}

void HttpDownloader::onFinished()
{
    if (!m_reply)
        return;

    onReadyRead();

    const int statusCode =
        m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (m_cancelled) {
        m_file.close();
        m_reply->deleteLater();
        m_reply = nullptr;
        emit status("已取消");
        return;
    }

    if (m_paused) {
        m_file.close();
        m_reply->deleteLater();
        m_reply = nullptr;
        emit status("已暂停");
        return;
    }

    if (m_existingSize > 0 && statusCode == 200) {
        // 服务器不支持 Range，重新完整下载。
        m_file.close();
        QFile::remove(m_partPath);
        m_reply->deleteLater();
        m_reply = nullptr;
        emit status("服务器不支持断点续传，重新下载");
        beginRequest(false);
        return;
    }

    if (m_reply->error() != QNetworkReply::NoError) {
        m_file.close();
        m_reply->deleteLater();
        m_reply = nullptr;
        return;
    }

    m_file.flush();
    m_file.close();

    if (QFile::exists(m_outputPath))
        QFile::remove(m_outputPath);

    if (!QFile::rename(m_partPath, m_outputPath)) {
        m_reply->deleteLater();
        m_reply = nullptr;
        emit error("下载完成，但无法重命名文件");
        return;
    }

    m_reply->deleteLater();
    m_reply = nullptr;

    emit progress(QFileInfo(m_outputPath).size(),
                  QFileInfo(m_outputPath).size());
    emit speed(0);
    emit status("下载完成");
    emit finished(m_outputPath);
}

void HttpDownloader::onError(QNetworkReply::NetworkError)
{
    if (m_reply && m_reply->error() != QNetworkReply::OperationCanceledError)
        emit error(m_reply->errorString());
}

void HttpDownloader::pause()
{
    if (!m_reply)
        return;

    m_paused = true;
    m_reply->abort();
}

void HttpDownloader::resume()
{
    if (!m_paused)
        return;

    m_paused = false;
    m_cancelled = false;
    beginRequest(true);
}

void HttpDownloader::cancel()
{
    m_cancelled = true;

    if (m_reply)
        m_reply->abort();

    if (m_file.isOpen())
        m_file.close();

    m_reply = nullptr;
}
