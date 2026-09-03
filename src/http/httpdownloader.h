#ifndef HTTPDOWNLOADER_H
#define HTTPDOWNLOADER_H

#include <QObject>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QElapsedTimer>

class HttpDownloader : public QObject
{
    Q_OBJECT
public:
    explicit HttpDownloader(QObject *parent = nullptr);

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
    void onReadyRead();
    void onProgress(qint64 received, qint64 total);
    void onFinished();
    void onError(QNetworkReply::NetworkError code);

private:
    void beginRequest(bool tryResume);

private:
    QNetworkAccessManager m_manager;
    QNetworkReply *m_reply;
    QFile m_file;
    QString m_url;
    QString m_outputPath;
    QString m_partPath;
    qint64 m_existingSize;
    bool m_paused;
    bool m_cancelled;
    QElapsedTimer m_speedTimer;
    qint64 m_speedBytes;
};

#endif
