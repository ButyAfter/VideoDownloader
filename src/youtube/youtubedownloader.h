#ifndef YOUTUBEDOWNLOADER_H
#define YOUTUBEDOWNLOADER_H

#include <QObject>
#include <QProcess>

class YouTubeDownloader : public QObject
{
    Q_OBJECT
public:
    explicit YouTubeDownloader(QObject *parent = nullptr);

    void start(const QString &url, const QString &outputPath);
    void pause();
    void resume();
    void cancel();

signals:
    void progress(qint64 received, qint64 total);
    void status(const QString &text);
    void finished(const QString &filePath);
    void error(const QString &text);

private slots:
    void onReadyRead();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QString executable() const;
    QString buildOutputTemplate(const QString &outputPath) const;

private:
    QProcess m_process;
    QString m_url;
    QString m_outputPath;
};

#endif
