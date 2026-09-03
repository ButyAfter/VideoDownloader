#include "ffmpegmanager.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>
#include <QDir>

FFmpegManager::FFmpegManager()
{
}

QString FFmpegManager::executable() const
{
#ifdef Q_OS_WIN
    QString local =
        QCoreApplication::applicationDirPath() + "/tools/ffmpeg.exe";
    if (QFileInfo::exists(local))
        return local;
    return "ffmpeg.exe";
#else
    QString local =
        QCoreApplication::applicationDirPath() + "/tools/ffmpeg";
    if (QFileInfo::exists(local))
        return local;
    return "ffmpeg";
#endif
}

bool FFmpegManager::concat(const QString &concatFile,
                           const QString &outputFile)
{
    QProcess process;

    QStringList args;
    args << "-y"
         << "-f" << "concat"
         << "-safe" << "0"
         << "-i" << concatFile
         << "-c" << "copy"
         << outputFile;

    process.start(executable(), args);

    if (!process.waitForStarted(5000)) {
        m_lastError = "无法启动 FFmpeg，请确认 tools/ffmpeg.exe 存在或已加入 PATH";
        return false;
    }

    if (!process.waitForFinished(-1)) {
        m_lastError = "FFmpeg 执行超时";
        process.kill();
        return false;
    }

    if (process.exitCode() != 0) {
        m_lastError =
            "FFmpeg 合并失败：\n" +
            QString::fromLocal8Bit(process.readAllStandardError());
        return false;
    }

    return true;
}

QString FFmpegManager::lastError() const
{
    return m_lastError;
}
