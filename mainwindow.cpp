#include "mainwindow.h"

#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QPlainTextEdit>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QWidget>
#include <QMessageBox>
#include <QFileInfo>
#include <QUrl>

/*
 * 构造函数
 *
 * 初始化 UI、信号连接、窗口属性。
 * 初始状态：暂停和取消按钮禁用。
 */
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    m_urlEdit(nullptr),
    m_pathEdit(nullptr),
    m_startButton(nullptr),
    m_pauseButton(nullptr),
    m_cancelButton(nullptr),
    m_progressBar(nullptr),
    m_statusLabel(nullptr),
    m_speedLabel(nullptr),
    m_sizeLabel(nullptr),
    m_logEdit(nullptr),
    m_downloader(new Downloader(this))
{
    setupUi();
    setupConnections();

    resize(800, 500);

    setWindowTitle(
        QStringLiteral("Qt 视频下载器"));

    m_pauseButton->setEnabled(false);
    m_cancelButton->setEnabled(false);
}

MainWindow::~MainWindow()
{
}

/*
 * setupUi - 构建界面布局
 *
 * 从上到下依次为：
 *   1. URL 输入行（标签 + 输入框 + 开始按钮）
 *   2. 保存路径行（标签 + 输入框 + 选择按钮）
 *   3. 进度条
 *   4. 状态信息行（状态标签 | 大小 | 速度）
 *   5. 控制按钮行（暂停 | 取消）
 *   6. 运行日志区
 */
void MainWindow::setupUi()
{
    QWidget *central =
        new QWidget(this);

    setCentralWidget(central);

    /*
     * 第一行：URL 输入
     */
    QLabel *urlLabel =
        new QLabel(
            QStringLiteral("视频地址："),
            this);

    m_urlEdit =
        new QLineEdit(this);

    m_urlEdit->setPlaceholderText(
        QStringLiteral(
            "请输入 MP4 / M3U8 等视频地址"));

    m_startButton =
        new QPushButton(
            QStringLiteral("开始下载"),
            this);

    QHBoxLayout *urlLayout =
        new QHBoxLayout;

    urlLayout->addWidget(urlLabel);
    urlLayout->addWidget(m_urlEdit);
    urlLayout->addWidget(m_startButton);

    /*
     * 第二行：保存路径
     */
    QLabel *pathLabel =
        new QLabel(
            QStringLiteral("保存位置："),
            this);

    m_pathEdit =
        new QLineEdit(this);

    m_pathEdit->setText(
        QDir::homePath() +
        "/Downloads/video.mp4");

    QPushButton *browseButton =
        new QPushButton(
            QStringLiteral("选择"),
            this);

    QHBoxLayout *pathLayout =
        new QHBoxLayout;

    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(m_pathEdit);
    pathLayout->addWidget(browseButton);

    /* "选择"按钮点击后弹出文件保存对话框 */
    connect(
        browseButton,
        &QPushButton::clicked,
        this,
        [this]() {

            QString path =
                QFileDialog::getSaveFileName(
                    this,
                    QStringLiteral("选择保存位置"),
                    m_pathEdit->text(),
                    QStringLiteral(
                        "MP4 Video (*.mp4);;"
                        "All Files (*.*)"));

            if (!path.isEmpty()) {
                m_pathEdit->setText(path);
            }
        });

    /*
     * 第三行：进度条
     */
    m_progressBar =
        new QProgressBar(this);

    m_progressBar->setRange(0, 100);

    m_progressBar->setValue(0);

    /*
     * 第四行：状态信息
     * 左侧状态标签，右侧大小和速度标签
     */
    m_statusLabel =
        new QLabel(
            QStringLiteral("等待下载"),
            this);

    m_speedLabel =
        new QLabel(
            QStringLiteral("速度：0 B/s"),
            this);

    m_sizeLabel =
        new QLabel(
            QStringLiteral("大小：0 B"),
            this);

    QHBoxLayout *infoLayout =
        new QHBoxLayout;

    infoLayout->addWidget(m_statusLabel);
    infoLayout->addStretch();
    infoLayout->addWidget(m_sizeLabel);
    infoLayout->addWidget(m_speedLabel);

    /*
     * 第五行：控制按钮
     * 右对齐，暂停和取消按钮
     */
    m_pauseButton =
        new QPushButton(
            QStringLiteral("暂停"),
            this);

    m_cancelButton =
        new QPushButton(
            QStringLiteral("取消"),
            this);

    QHBoxLayout *buttonLayout =
        new QHBoxLayout;

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_pauseButton);
    buttonLayout->addWidget(m_cancelButton);

    /*
     * 第六行：运行日志
     * 只读文本框，记录下载过程事件
     */
    m_logEdit =
        new QPlainTextEdit(this);

    m_logEdit->setReadOnly(true);

    /* 主垂直布局：组装所有行 */
    QVBoxLayout *mainLayout =
        new QVBoxLayout;

    mainLayout->addLayout(urlLayout);
    mainLayout->addLayout(pathLayout);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(m_progressBar);
    mainLayout->addLayout(infoLayout);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(
        new QLabel(
            QStringLiteral("运行日志："),
            this));
    mainLayout->addWidget(m_logEdit);

    central->setLayout(mainLayout);
}

/*
 * setupConnections - 连接信号与槽
 *
 * 三类连接：
 *   1. 按钮点击 -> MainWindow 处理函数
 *   2. Downloader 信号 -> MainWindow UI 更新函数
 *   3. "选择"按钮在 setupUi() 中已单独连接（lambda）
 */
void MainWindow::setupConnections()
{
    /* 按钮点击 -> 处理函数 */
    connect(
        m_startButton,
        &QPushButton::clicked,
        this,
        &MainWindow::onStartClicked);

    connect(
        m_pauseButton,
        &QPushButton::clicked,
        this,
        &MainWindow::onPauseClicked);

    connect(
        m_cancelButton,
        &QPushButton::clicked,
        this,
        &MainWindow::onCancelClicked);

    /* Downloader 信号 -> UI 更新 */
    connect(
        m_downloader,
        &Downloader::started,
        this,
        &MainWindow::onStarted);

    connect(
        m_downloader,
        &Downloader::progress,
        this,
        &MainWindow::onProgress);

    connect(
        m_downloader,
        &Downloader::finished,
        this,
        &MainWindow::onFinished);

    connect(
        m_downloader,
        &Downloader::failed,
        this,
        &MainWindow::onFailed);

    connect(
        m_downloader,
        &Downloader::paused,
        this,
        &MainWindow::onPaused);

    connect(
        m_downloader,
        &Downloader::resumed,
        this,
        &MainWindow::onResumed);

    connect(
        m_downloader,
        &Downloader::canceled,
        this,
        &MainWindow::onCanceled);
}

/*
 * onStartClicked - "开始下载"按钮点击处理
 *
 * 校验输入后启动下载任务。
 * 校验项：URL 非空、路径非空、URL 格式合法。
 * 校验通过后禁用开始按钮，启用暂停和取消按钮。
 */
void MainWindow::onStartClicked()
{
    QString urlString =
        m_urlEdit->text().trimmed();

    QString filePath =
        m_pathEdit->text().trimmed();

    if (urlString.isEmpty()) {

        QMessageBox::warning(
            this,
            QStringLiteral("提示"),
            QStringLiteral("请输入视频地址"));

        return;
    }

    if (filePath.isEmpty()) {

        QMessageBox::warning(
            this,
            QStringLiteral("提示"),
            QStringLiteral("请选择保存位置"));

        return;
    }

    QUrl url(urlString);

    if (!url.isValid() ||
        url.scheme() != "http" &&
            url.scheme() != "https") {

        QMessageBox::warning(
            this,
            QStringLiteral("提示"),
            QStringLiteral(
                "请输入有效的 HTTP/HTTPS URL"));

        return;
    }

    m_logEdit->appendPlainText(
        QStringLiteral(
            "开始下载：%1")
            .arg(urlString));

    m_progressBar->setValue(0);

    m_startButton->setEnabled(false);
    m_pauseButton->setEnabled(true);
    m_cancelButton->setEnabled(true);

    m_statusLabel->setText(
        QStringLiteral("正在连接..."));

    m_downloader->start(
        url,
        filePath);
}

/*
 * onPauseClicked - "暂停/继续"按钮点击处理
 *
 * 逻辑：
 *   如果正在下载 -> 暂停
 *   如果已暂停   -> 继续
 *   否则无操作
 */
void MainWindow::onPauseClicked()
{
    if (!m_downloader->isDownloading()) {

        if (m_downloader->isPaused()) {
            m_downloader->resume();
        }

        return;
    }

    m_downloader->pause();

    m_pauseButton->setEnabled(false);

    m_statusLabel->setText(
        QStringLiteral("正在暂停..."));
}

/*
 * onCancelClicked - "取消"按钮点击处理
 *
 * 调用 Downloader::cancel()，禁用暂停和取消按钮。
 */
void MainWindow::onCancelClicked()
{
    m_downloader->cancel();

    m_statusLabel->setText(
        QStringLiteral("正在取消..."));

    m_pauseButton->setEnabled(false);
    m_cancelButton->setEnabled(false);
}

/*
 * onStarted - 下载开始回调
 *
 * 如果 existing > 0，说明是断点续传，
 * 日志中显示已下载的大小。
 */
void MainWindow::onStarted(
    qint64 total,
    qint64 existing)
{
    Q_UNUSED(total)

    if (existing > 0) {

        m_logEdit->appendPlainText(
            QStringLiteral(
                "发现未完成文件，继续下载：%1")
                .arg(formatBytes(existing)));

        m_statusLabel->setText(
            QStringLiteral("断点续传"));
    }
    else {

        m_statusLabel->setText(
            QStringLiteral("下载中"));
    }
}

/*
 * onProgress - 下载进度回调
 *
 * 更新进度条、大小标签、速度标签。
 * total <= 0 时表示服务器未返回文件大小（流式下载），
 * 此时只显示已下载大小，不显示百分比。
 */
void MainWindow::onProgress(
    qint64 received,
    qint64 total,
    qint64 speed)
{
    if (total > 0) {

        int percent =
            static_cast<int>(
                received * 100 / total);

        if (percent > 100)
            percent = 100;

        m_progressBar->setValue(percent);

        m_sizeLabel->setText(
            QStringLiteral(
                "大小：%1 / %2")
                .arg(formatBytes(received))
                .arg(formatBytes(total)));
    }
    else {

        m_sizeLabel->setText(
            QStringLiteral(
                "已下载：%1")
                .arg(formatBytes(received)));
    }

    m_speedLabel->setText(
        QStringLiteral(
            "速度：%1")
            .arg(formatSpeed(speed)));

    m_statusLabel->setText(
        QStringLiteral("下载中"));
}

/*
 * onFinished - 下载完成回调
 *
 * 重置按钮状态，显示完成提示对话框。
 */
void MainWindow::onFinished(
    const QString &filePath)
{
    m_progressBar->setValue(100);

    m_statusLabel->setText(
        QStringLiteral("下载完成"));

    m_speedLabel->setText(
        QStringLiteral("速度：0 B/s"));

    m_startButton->setEnabled(true);
    m_pauseButton->setEnabled(false);
    m_cancelButton->setEnabled(false);

    m_logEdit->appendPlainText(
        QStringLiteral(
            "下载完成：%1")
            .arg(filePath));

    QMessageBox::information(
        this,
        QStringLiteral("下载完成"),
        QStringLiteral(
            "视频下载完成：\n%1")
            .arg(filePath));
}

/*
 * onFailed - 下载失败回调
 *
 * 重置按钮状态，日志记录错误，弹出错误对话框。
 */
void MainWindow::onFailed(
    const QString &error)
{
    m_statusLabel->setText(
        QStringLiteral("下载失败"));

    m_startButton->setEnabled(true);
    m_pauseButton->setEnabled(false);
    m_cancelButton->setEnabled(false);

    m_logEdit->appendPlainText(
        QStringLiteral(
            "下载失败：%1")
            .arg(error));

    QMessageBox::critical(
        this,
        QStringLiteral("下载失败"),
        error);
}

/*
 * onPaused - 下载已暂停回调
 *
 * 将暂停按钮文本改为"继续"，
 * 用户点击后触发 Downloader::resume()。
 */
void MainWindow::onPaused()
{
    m_statusLabel->setText(
        QStringLiteral("已暂停"));

    m_pauseButton->setText(
        QStringLiteral("继续"));

    m_pauseButton->setEnabled(true);

    m_cancelButton->setEnabled(true);

    m_logEdit->appendPlainText(
        QStringLiteral(
            "下载已暂停，.part 文件已保留"));
}

/*
 * onResumed - 下载已继续回调
 *
 * 将按钮文本改回"暂停"。
 */
void MainWindow::onResumed()
{
    m_statusLabel->setText(
        QStringLiteral("继续下载"));

    m_pauseButton->setText(
        QStringLiteral("暂停"));

    m_pauseButton->setEnabled(true);

    m_cancelButton->setEnabled(true);

    m_logEdit->appendPlainText(
        QStringLiteral("继续下载"));
}

/*
 * onCanceled - 下载已取消回调
 *
 * 重置按钮状态和进度条。
 */
void MainWindow::onCanceled()
{
    m_statusLabel->setText(
        QStringLiteral("已取消"));

    m_progressBar->setValue(0);

    m_startButton->setEnabled(true);
    m_pauseButton->setEnabled(false);
    m_cancelButton->setEnabled(false);

    m_logEdit->appendPlainText(
        QStringLiteral(
            "下载已取消，未完成文件已删除"));
}

/*
 * formatBytes - 将字节数格式化为可读字符串
 *
 * 自动选择 B / KB / MB / GB 单位，保留两位小数。
 * 例如：512 -> "512 B"，1536 -> "1.50 KB"
 */
QString MainWindow::formatBytes(
    qint64 bytes) const
{
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    }

    double value = bytes;

    value /= 1024.0;

    if (value < 1024.0) {
        return QString("%1 KB")
        .arg(value, 0, 'f', 2);
    }

    value /= 1024.0;

    if (value < 1024.0) {
        return QString("%1 MB")
        .arg(value, 0, 'f', 2);
    }

    value /= 1024.0;

    return QString("%1 GB")
        .arg(value, 0, 'f', 2);
}

/*
 * formatSpeed - 将字节/秒格式化为速度字符串
 *
 * 例如：1048576 -> "1.00 MB/s"
 */
QString MainWindow::formatSpeed(
    qint64 bytesPerSecond) const
{
    return formatBytes(bytesPerSecond)
    + "/s";
}
