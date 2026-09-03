#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLineEdit;
class QPushButton;
class QProgressBar;
class QLabel;
class QPlainTextEdit;

#include "downloader.h"

/*
 * MainWindow - 主窗口
 *
 * 提供完整的下载管理界面：
 *   - URL 输入和保存路径选择
 *   - 开始/暂停/继续/取消控制
 *   - 实时进度条、速度、大小显示
 *   - 运行日志
 *
 * 所有 UI 更新由 Downloader 信号驱动。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    /* onStartClicked - "开始下载"按钮点击处理 */
    void onStartClicked();

    /*
     * onPauseClicked - "暂停/继续"按钮点击处理
     * 正在下载时 -> 暂停
     * 已暂停时   -> 继续
     */
    void onPauseClicked();

    /* onCancelClicked - "取消"按钮点击处理 */
    void onCancelClicked();

    /*
     * onStarted - 下载开始回调
     * total    - 文件总大小（目前固定 -1）
     * existing - 已下载字节数，> 0 表示断点续传
     */
    void onStarted(qint64 total,
                   qint64 existing);

    /*
     * onProgress - 下载进度回调
     * received - 已下载字节数
     * total    - 文件总大小（-1 表示未知）
     * speed    - 当前速度（字节/秒）
     */
    void onProgress(qint64 received,
                    qint64 total,
                    qint64 speed);

    /* onFinished - 下载完成回调 */
    void onFinished(const QString &filePath);

    /* onFailed - 下载失败回调 */
    void onFailed(const QString &error);

    /* onPaused - 下载已暂停回调 */
    void onPaused();

    /* onResumed - 下载已继续回调 */
    void onResumed();

    /* onCanceled - 下载已取消回调 */
    void onCanceled();

private:
    /* setupUi - 构建界面布局和控件 */
    void setupUi();

    /* setupConnections - 连接按钮信号与 Downloader 信号到对应槽 */
    void setupConnections();

    /*
     * formatBytes - 将字节数格式化为可读字符串
     * 自动选择 B / KB / MB / GB 单位，保留两位小数
     */
    QString formatBytes(qint64 bytes) const;

    /* formatSpeed - 将字节/秒格式化为速度字符串（如 "1.50 MB/s"） */
    QString formatSpeed(qint64 bytesPerSecond) const;

private:
    /* m_urlEdit - 视频 URL 输入框 */
    QLineEdit *m_urlEdit;

    /* m_pathEdit - 保存路径输入框，默认值：~/Downloads/video.mp4 */
    QLineEdit *m_pathEdit;

    /* m_startButton - "开始下载"按钮 */
    QPushButton *m_startButton;

    /*
     * m_pauseButton - "暂停/继续"按钮
     * 下载中显示"暂停"，暂停后显示"继续"
     */
    QPushButton *m_pauseButton;

    /* m_cancelButton - "取消"按钮 */
    QPushButton *m_cancelButton;

    /* m_progressBar - 下载进度条，范围 0-100 */
    QProgressBar *m_progressBar;

    /* m_statusLabel - 状态标签，显示当前状态文本（等待/下载中/暂停/完成/失败） */
    QLabel *m_statusLabel;

    /* m_speedLabel - 速度标签，显示当前下载速度 */
    QLabel *m_speedLabel;

    /* m_sizeLabel - 大小标签，显示"已下载/总大小"或"已下载" */
    QLabel *m_sizeLabel;

    /* m_logEdit - 运行日志区，只读文本框，记录下载过程事件 */
    QPlainTextEdit *m_logEdit;

    /* m_downloader - 下载器实例，负责实际的 HTTP 下载和断点续传 */
    Downloader *m_downloader;
};

#endif
