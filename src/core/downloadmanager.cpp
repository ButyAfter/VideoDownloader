#include "downloadmanager.h"
#include "../http/httpdownloader.h"
#include "../m3u8/m3u8downloader.h"
#include "../youtube/youtubedownloader.h"

DownloadManager::DownloadManager(QObject *parent)
    : QObject(parent),
      m_http(new HttpDownloader(this)),
      m_m3u8(new M3U8Downloader(this)),
      m_youtube(new YouTubeDownloader(this))
{
    connect(m_http, &HttpDownloader::progress, this, &DownloadManager::progress);
    connect(m_http, &HttpDownloader::speed, this, &DownloadManager::speed);
    connect(m_http, &HttpDownloader::status, this, &DownloadManager::status);
    connect(m_http, &HttpDownloader::finished, this, &DownloadManager::finished);
    connect(m_http, &HttpDownloader::error, this, &DownloadManager::error);

    connect(m_m3u8, &M3U8Downloader::progress, this, &DownloadManager::progress);
    connect(m_m3u8, &M3U8Downloader::speed, this, &DownloadManager::speed);
    connect(m_m3u8, &M3U8Downloader::status, this, &DownloadManager::status);
    connect(m_m3u8, &M3U8Downloader::finished, this, &DownloadManager::finished);
    connect(m_m3u8, &M3U8Downloader::error, this, &DownloadManager::error);

    connect(m_youtube, &YouTubeDownloader::progress, this, &DownloadManager::progress);
    connect(m_youtube, &YouTubeDownloader::status, this, &DownloadManager::status);
    connect(m_youtube, &YouTubeDownloader::finished, this, &DownloadManager::finished);
    connect(m_youtube, &YouTubeDownloader::error, this, &DownloadManager::error);
}

void DownloadManager::start(const DownloadTask &task)
{
    cancel();
    m_task = task;

    if (task.type == DownloadTask::Direct)
        m_http->start(task.url, task.outputPath);
    else if (task.type == DownloadTask::M3U8)
        m_m3u8->start(task.url, task.outputPath);
    else
        m_youtube->start(task.url, task.outputPath);
}

void DownloadManager::pause()
{
    if (m_task.type == DownloadTask::Direct)
        m_http->pause();
    else if (m_task.type == DownloadTask::M3U8)
        m_m3u8->pause();
    else
        m_youtube->pause();
}

void DownloadManager::resume()
{
    if (m_task.type == DownloadTask::Direct)
        m_http->resume();
    else if (m_task.type == DownloadTask::M3U8)
        m_m3u8->resume();
    else
        m_youtube->resume();
}

void DownloadManager::cancel()
{
    if (m_http) m_http->cancel();
    if (m_m3u8) m_m3u8->cancel();
    if (m_youtube) m_youtube->cancel();
}

void DownloadManager::clearCurrent()
{
}
