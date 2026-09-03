#include "mainwindow.h"
#include "../src/core/downloadmanager.h"
#include "../src/core/downloadtask.h"
#include "../src/resolver/urlresolver.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QPlainTextEdit>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_urlEdit(new QLineEdit(this)),
      m_outputEdit(new QLineEdit(this)),
      m_startButton(new QPushButton("开始", this)),
      m_pauseButton(new QPushButton("暂停", this)),
      m_resumeButton(new QPushButton("继续", this)),
      m_cancelButton(new QPushButton("取消", this)),
      m_progress(new QProgressBar(this)),
      m_status(new QLabel("就绪", this)),
      m_speed(new QLabel("速度：0 B/s", this)),
      m_log(new QPlainTextEdit(this)),
      m_manager(new DownloadManager(this))
{
    setWindowTitle("VideoDownloader");
    resize(900, 600);

    m_urlEdit->setPlaceholderText(
        "输入 MP4 / M3U8 / YouTube URL，例如 https://...");

    m_outputEdit->setText(
        QDir::homePath() + "/Downloads/video.mp4");

    QPushButton *browse = new QPushButton("选择...", this);

    QFormLayout *form = new QFormLayout;
    form->addRow("视频地址：", m_urlEdit);

    QHBoxLayout *outputLayout = new QHBoxLayout;
    outputLayout->addWidget(m_outputEdit);
    outputLayout->addWidget(browse);
    form->addRow("保存位置：", outputLayout);

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addWidget(m_startButton);
    buttons->addWidget(m_pauseButton);
    buttons->addWidget(m_resumeButton);
    buttons->addWidget(m_cancelButton);

    m_progress->setRange(0, 100);
    m_progress->setValue(0);

    m_log->setReadOnly(true);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(form);
    mainLayout->addLayout(buttons);
    mainLayout->addWidget(m_progress);
    mainLayout->addWidget(m_status);
    mainLayout->addWidget(m_speed);
    mainLayout->addWidget(m_log, 1);

    QWidget *central = new QWidget(this);
    central->setLayout(mainLayout);
    setCentralWidget(central);

    connect(browse, &QPushButton::clicked,
            this, &MainWindow::chooseOutput);
    connect(m_startButton, &QPushButton::clicked,
            this, &MainWindow::startDownload);
    connect(m_pauseButton, &QPushButton::clicked,
            m_manager, &DownloadManager::pause);
    connect(m_resumeButton, &QPushButton::clicked,
            m_manager, &DownloadManager::resume);
    connect(m_cancelButton, &QPushButton::clicked,
            m_manager, &DownloadManager::cancel);

    connect(m_manager, &DownloadManager::progress,
            this, &MainWindow::onProgress);
    connect(m_manager, &DownloadManager::speed,
            this, &MainWindow::onSpeed);
    connect(m_manager, &DownloadManager::status,
            this, &MainWindow::onStatus);
    connect(m_manager, &DownloadManager::finished,
            this, &MainWindow::onFinished);
    connect(m_manager, &DownloadManager::error,
            this, &MainWindow::onError);
}

void MainWindow::chooseOutput()
{
    QString path = QFileDialog::getSaveFileName(
        this,
        "选择保存位置",
        m_outputEdit->text(),
        "视频文件 (*.mp4 *.mkv *.ts);;所有文件 (*.*)");

    if (!path.isEmpty())
        m_outputEdit->setText(path);
}

void MainWindow::startDownload()
{
    const QString url = m_urlEdit->text().trimmed();
    const QString output = m_outputEdit->text().trimmed();

    if (url.isEmpty() || output.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入视频地址和保存位置");
        return;
    }

    UrlResolver::Type type = UrlResolver::resolve(url);

    if (type == UrlResolver::Unknown) {
        QMessageBox::warning(this, "提示", "无法识别 URL");
        return;
    }

    DownloadTask task;
    task.url = url;
    task.outputPath = output;

    if (type == UrlResolver::YouTube)
        task.type = DownloadTask::YouTube;
    else if (type == UrlResolver::M3U8)
        task.type = DownloadTask::M3U8;
    else
        task.type = DownloadTask::Direct;

    m_progress->setValue(0);
    m_log->clear();

    m_manager->start(task);
}

void MainWindow::onProgress(qint64 received, qint64 total)
{
    if (total > 0) {
        int value = int((received * 100) / total);
        if (value > 100) value = 100;
        m_progress->setValue(value);
    }
}

void MainWindow::onSpeed(qint64 bytesPerSecond)
{
    double kb = bytesPerSecond / 1024.0;
    double mb = kb / 1024.0;

    if (mb >= 1.0)
        m_speed->setText(QString("速度：%1 MB/s").arg(mb, 0, 'f', 2));
    else
        m_speed->setText(QString("速度：%1 KB/s").arg(kb, 0, 'f', 1));
}

void MainWindow::onStatus(const QString &text)
{
    m_status->setText(text);
    m_log->appendPlainText(text);
}

void MainWindow::onFinished(const QString &filePath)
{
    m_status->setText("完成：" + filePath);
    m_log->appendPlainText("输出文件：" + filePath);
    m_progress->setValue(100);
}

void MainWindow::onError(const QString &text)
{
    m_status->setText("错误");
    m_log->appendPlainText("ERROR: " + text);
}
