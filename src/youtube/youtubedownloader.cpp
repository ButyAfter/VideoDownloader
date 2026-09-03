#include "youtubedownloader.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

YouTubeDownloader::YouTubeDownloader(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &YouTubeDownloader::onReadyRead);
    connect(&m_process, &QProcess::readyReadStandardError,
            this, &YouTubeDownloader::onReadyRead);
    connect(&m_process,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
                &QProcess::finished),
            this,
            &YouTubeDownloader::onFinished);
}

QString YouTubeDownloader::executable() const
{
#ifdef Q_OS_WIN
    QString local =
        QCoreApplication::applicationDirPath() + "/tools/yt-dlp.exe";
    if (QFileInfo::exists(local))
        return local;
    return "yt-dlp.exe";
#else
    QString local =
        QCoreApplication::applicationDirPath() + "/tools/yt-dlp";
    if (QFileInfo::exists(local))
        return local;
    return "yt-dlp";
#endif
}

QString YouTubeDownloader::buildOutputTemplate(
        const QString &outputPath) const
{
    QFileInfo info(outputPath);
    return QDir::toNativeSeparators(
        info.absolutePath() + "/" + info.completeBaseName() + ".%(ext)s");
}

void YouTubeDownloader::start(const QString &url,
                              const QString &outputPath)
{
    cancel();

    m_url = url;
    m_outputPath = outputPath;

    QStringList args;
    args << "--newline"
         << "-f" << "bv*+ba/b"
         << "--merge-output-format" << "mp4"
         << "-o" << buildOutputTemplate(outputPath)
         << url;

    emit status("正在启动 yt-dlp...");

    m_process.start(executable(), args);

    if (!m_process.waitForStarted(3000)) {
        emit error("无法启动 yt-dlp，请将 yt-dlp.exe 放入 tools/ 或加入 PATH");
    }
}

void YouTubeDownloader::onReadyRead()
{
    QByteArray data = m_process.readAllStandardOutput();
    data += m_process.readAllStandardError();

    const QString text = QString::fromLocal8Bit(data);
    if (!text.trimmed().isEmpty())
        emit status(text.trimmed());
}

void YouTubeDownloader::onFinished(int exitCode,
                                   QProcess::ExitStatus exitStatus)
{
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        emit error("yt-dlp 下载失败");
        return;
    }

    QFileInfo info(m_outputPath);
    QString expected = info.absolutePath() + "/" +
                       info.completeBaseName() + ".mp4";

    if (!QFileInfo::exists(expected)) {
        emit error("yt-dlp 已结束，但没有找到输出 MP4");
        return;
    }

    emit progress(1, 1);
    emit status("YouTube 下载完成");
    emit finished(expected);
}

void YouTubeDownloader::pause()
{
    // yt-dlp 本身可以利用部分下载文件继续。
    // 这里先终止进程，resume 时重新执行同一个命令。
    if (m_process.state() != QProcess::NotRunning)
        m_process.terminate();

    emit status("YouTube 下载已暂停");
}

void YouTubeDownloader::resume()
{
    if (m_url.isEmpty())
        return;

    emit status("继续 YouTube 下载...");
    start(m_url, m_outputPath);
}

void YouTubeDownloader::cancel()
{
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(1000);
    }
}
