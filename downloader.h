#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QElapsedTimer>
#include <QUrl>

/*
 * Downloader - HTTP 文件下载器
 *
 * 支持暂停/继续/取消，断点续传（HTTP Range）。
 * 通过信号通知进度、完成、失败等状态。
 *
 * 下载文件保存为 <filePath>.part，完成后再重命名为正式文件。
 */
class Downloader : public QObject
{
    Q_OBJECT

public:
    explicit Downloader(QObject *parent = nullptr);
    ~Downloader();

    /*
     * start - 开始下载
     * url      - 视频地址（仅支持 HTTP/HTTPS）
     * filePath - 保存路径（不含 .part 后缀）
     */
    void start(const QUrl &url, const QString &filePath);

    /* pause - 暂停下载，保留 .part 文件 */
    void pause();

    /* resume - 继续下载，读取已有 .part 文件实现断点续传 */
    void resume();

    /* cancel - 取消下载，保留 .part 文件 */
    void cancel();

    /* isDownloading - 是否正在下载（有活跃的网络请求） */
    bool isDownloading() const;

    /* isPaused - 是否处于暂停状态 */
    bool isPaused() const;

signals:
    /*
     * progress - 下载进度更新
     * received - 已下载字节数（含断点续传部分）
     * total    - 文件总大小，服务器未返回时为 -1
     * speed    - 当前下载速度（字节/秒）
     */
    void progress(qint64 received,
                  qint64 total,
                  qint64 speed);

    /*
     * started - 下载开始
     * total   - 文件总大小（目前固定传 -1）
     * existing - 已下载字节数，> 0 表示断点续传
     */
    void started(qint64 total, qint64 existing);

    /* finished - 下载完成，filePath 为最终保存路径 */
    void finished(const QString &filePath);

    /* failed - 下载失败，error 为错误描述 */
    void failed(const QString &error);

    /* paused - 下载已暂停 */
    void paused();

    /* resumed - 下载已继续 */
    void resumed();

    /* canceled - 下载已取消 */
    void canceled();

private slots:
    /* onReadyRead - 网络数据到达，读取并写入 .part 文件 */
    void onReadyRead();

    /*
     * onDownloadProgress - Qt 网络层进度回调
     * received - 本次请求收到的字节数（不含断点续传部分）
     * total    - 本次请求的总字节数（不含断点续传部分）
     */
    void onDownloadProgress(qint64 received, qint64 total);

    /* onFinished - 网络请求完成，根据状态分情况处理 */
    void onFinished();

    /* onError - 网络错误信号（兼容旧版 Qt，实际处理在 onFinished） */
    void onError(QNetworkReply::NetworkError code);

private:
    /* createRequest - 创建 HTTP 请求，设置 User-Agent 和 Range 头 */
    void createRequest();

    /* reset - 重置所有内部状态（用于新下载开始前） */
    void reset();

    /* closeFile - 关闭并刷新 .part 文件 */
    void closeFile();

private:
    /* m_manager - 网络访问管理器，负责发送 HTTP 请求 */
    QNetworkAccessManager *m_manager;

    /* m_reply - 当前活跃的网络回复对象，nullptr 表示无下载任务 */
    QNetworkReply *m_reply;

    /* m_file - .part 文件对象，下载期间保持打开 */
    QFile m_file;

    /* m_url - 当前下载的 URL */
    QUrl m_url;

    /* m_filePath - 保存路径（不含 .part 后缀） */
    QString m_filePath;

    /* m_existingSize - 已下载的字节数（来自本地 .part 文件大小），用于断点续传 */
    qint64 m_existingSize;

    /* m_totalSize - 文件总大小（目前未使用，保留字段） */
    qint64 m_totalSize;

    /* m_speedTimer - 速度计算用的计时器，每 500ms 更新一次速度 */
    QElapsedTimer m_speedTimer;

    /* m_lastBytes - 上次采样时的已下载字节数，用于计算速度差值 */
    qint64 m_lastBytes;

    /* m_speed - 当前计算的下载速度（字节/秒） */
    qint64 m_speed;

    /* m_paused - 是否处于暂停状态 */
    bool m_paused;

    /* m_canceling - 是否正在取消（避免重复触发） */
    bool m_canceling;
};

#endif
