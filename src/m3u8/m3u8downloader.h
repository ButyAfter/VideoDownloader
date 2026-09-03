#ifndef M3U8DOWNLOADER_H
#define M3U8DOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QDir>
#include <QElapsedTimer>
#include "m3u8parser.h"

class M3U8Downloader : public QObject
{
    Q_OBJECT
public:
    explicit M3U8Downloader(QObject *parent = nullptr);

    void start(const QString &url, const QString &outputPath);
    void pause();
    void resume();
    void cancel();

signals:
    void progress(qint64 received, qint64 total);
    void speed(qint64 bytesPerSecond);
    void status(const QString &text);
    void finished(const QString &filePath);
    void error(const QString &text);

private slots:
    void onPlaylistFinished();
    void onSegmentReadyRead();
    void onSegmentFinished();
    void onError(QNetworkReply::NetworkError);

private:
    void requestPlaylist(const QUrl &url);
    void startNextSegment();
    void finishDownload();
    void fail(const QString &text);
    void cleanup();

private:
    QNetworkAccessManager m_manager;
    QNetworkReply *m_reply;
    QFile m_segmentFile;

    QString m_playlistUrl;
    QString m_outputPath;
    QString m_tempDir;

    M3U8Playlist m_playlist;
    int m_currentIndex;
    qint64 m_downloadedBytes;
    qint64 m_totalBytes;
    bool m_paused;
    bool m_cancelled;

    QElapsedTimer m_speedTimer;
    qint64 m_speedBytes;
};

#endif
