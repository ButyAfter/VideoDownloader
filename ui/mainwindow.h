#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLineEdit;
class QPushButton;
class QProgressBar;
class QLabel;
class QPlainTextEdit;
class DownloadManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void startDownload();
    void chooseOutput();
    void onProgress(qint64 received, qint64 total);
    void onSpeed(qint64 bytesPerSecond);
    void onStatus(const QString &text);
    void onFinished(const QString &filePath);
    void onError(const QString &text);

private:
    QLineEdit *m_urlEdit;
    QLineEdit *m_outputEdit;
    QPushButton *m_startButton;
    QPushButton *m_pauseButton;
    QPushButton *m_resumeButton;
    QPushButton *m_cancelButton;
    QProgressBar *m_progress;
    QLabel *m_status;
    QLabel *m_speed;
    QPlainTextEdit *m_log;

    DownloadManager *m_manager;
};

#endif
