#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>
#include "downloadtask.h"

class HttpDownloader;
class M3U8Downloader;
class YouTubeDownloader;

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = nullptr);

    void start(const DownloadTask &task);
    void pause();
    void resume();
    void cancel();

signals:
    void progress(qint64 received, qint64 total);
    void speed(qint64 bytesPerSecond);
    void status(const QString &text);
    void finished(const QString &filePath);
    void error(const QString &text);

private:
    void clearCurrent();

private:
    DownloadTask m_task;
    HttpDownloader *m_http;
    M3U8Downloader *m_m3u8;
    YouTubeDownloader *m_youtube;
};

#endif
