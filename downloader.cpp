#include "downloader.h"

#include <QNetworkRequest>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

/*
 * 构造函数
 *
 * 创建网络管理器，初始化所有状态为未下载/未暂停/未取消。
 */
Downloader::Downloader(QObject *parent)
    : QObject(parent),
    m_manager(new QNetworkAccessManager(this)),
    m_reply(nullptr),
    m_existingSize(0),
    m_totalSize(0),
    m_lastBytes(0),
    m_speed(0),
    m_paused(false),
    m_canceling(false)
{
}

/*
 * 析构函数
 *
 * 确保下载任务被取消并释放资源。
 */
Downloader::~Downloader()
{
    cancel();
}

/*
 * start - 开始下载
 *
 * 参数：
 *   url      - 视频地址（仅支持 HTTP/HTTPS）
 *   filePath - 保存路径（不含 .part 后缀）
 *
 * 流程：
 *   1. 检查是否已有下载任务
 *   2. 检查 URL 有效性
 *   3. 创建保存目录（如不存在）
 *   4. 打开已有 .part 文件（支持断点续传）
 *   5. 发起 HTTP 请求
 */
void Downloader::start(const QUrl &url, const QString &filePath)
{
    if (m_reply) {
        emit failed(QStringLiteral("当前已经有下载任务"));
        return;
    }

    if (!url.isValid() ||
        (url.scheme() != "http" && url.scheme() != "https")) {
        emit failed(QStringLiteral("URL 无效，只支持 HTTP/HTTPS"));
        return;
    }

    reset();

    m_url = url;
    m_filePath = filePath;

    QFileInfo info(m_filePath);

    QDir dir = info.dir();

    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            emit failed(QStringLiteral("无法创建保存目录"));
            return;
        }
    }

    /*
     * 如果已经存在 .part 文件，则继续下载。
     */
    QString partFilePath = m_filePath + ".part";

    m_file.setFileName(partFilePath);

    if (!m_file.open(QIODevice::ReadWrite)) {
        emit failed(
            QStringLiteral("无法打开文件：%1")
                .arg(partFilePath));

        return;
    }

    m_existingSize = m_file.size();

    if (!m_file.seek(m_existingSize)) {
        closeFile();

        emit failed(QStringLiteral("无法定位文件"));
        return;
    }

    createRequest();
}

/*
 * createRequest - 创建并发送 HTTP 请求
 *
 * 设置 User-Agent 伪装浏览器。
 * 如果已有部分下载，附加 Range 头实现断点续传。
 */
void Downloader::createRequest()
{
    QNetworkRequest request(m_url);

    request.setRawHeader(
        "User-Agent",
        "Mozilla/5.0 "
        "(Windows NT 10.0; Win64; x64) "
        "AppleWebKit/537.36 "
        "(KHTML, like Gecko) "
        "Chrome/120 Safari/537.36");

    /*
     * 如果已有部分文件：
     *
     * Range: bytes=existing-
     *
     * 例如已经下载 100MB：
     *
     * Range: bytes=104857600-
     */
    if (m_existingSize > 0) {
        QByteArray range =
            "bytes=" +
            QByteArray::number(m_existingSize) +
            "-";

        request.setRawHeader("Range", range);
    }

    m_reply = m_manager->get(request);

    connect(
        m_reply,
        SIGNAL(readyRead()),
        this,
        SLOT(onReadyRead()));

    connect(
        m_reply,
        SIGNAL(downloadProgress(qint64,qint64)),
        this,
        SLOT(onDownloadProgress(qint64,qint64)));

    connect(
        m_reply,
        SIGNAL(finished()),
        this,
        SLOT(onFinished()));

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)

    connect(
        m_reply,
        SIGNAL(errorOccurred(QNetworkReply::NetworkError)),
        this,
        SLOT(onError(QNetworkReply::NetworkError)));

#else

    connect(
        m_reply,
        SIGNAL(error(QNetworkReply::NetworkError)),
        this,
        SLOT(onError(QNetworkReply::NetworkError)));

#endif

    m_speedTimer.start();
    m_lastBytes = 0;
    m_speed = 0;

    emit started(-1, m_existingSize);
}

/*
 * onReadyRead - 网络数据到达
 *
 * 读取数据并写入 .part 文件。
 * 如果写入失败（磁盘满等），中止下载。
 */
void Downloader::onReadyRead()
{
    if (!m_reply || !m_file.isOpen()) {
        return;
    }

    QByteArray data = m_reply->readAll();

    if (data.isEmpty()) {
        return;
    }

    qint64 written = m_file.write(data);

    if (written != data.size()) {
        m_reply->abort();

        emit failed(QStringLiteral("写入文件失败"));
    }
}

/*
 * onDownloadProgress - 下载进度更新
 *
 * received/total 是本次请求的字节数，
 * 需要加上 m_existingSize 才是真实进度。
 *
 * 每 500ms 更新一次速度。
 */
void Downloader::onDownloadProgress(
    qint64 received,
    qint64 total)
{
    /*
     * received 是本次 HTTP 请求收到的字节数。
     *
     * 如果我们之前已经下载了 100MB，
     * 那么真实进度应该：
     *
     * existing + received
     */
    qint64 realReceived =
        m_existingSize + received;

    qint64 realTotal = total;

    if (total >= 0) {
        realTotal =
            m_existingSize + total;
    }

    /*
     * 计算速度
     */
    qint64 elapsed =
        m_speedTimer.elapsed();

    if (elapsed >= 500) {

        qint64 delta =
            realReceived - m_lastBytes;

        m_speed =
            delta * 1000 / elapsed;

        m_lastBytes = realReceived;

        m_speedTimer.restart();
    }

    emit progress(
        realReceived,
        realTotal,
        m_speed);
}

/*
 * onFinished - HTTP 请求完成
 *
 * 根据状态分四种情况处理：
 *   1. 用户暂停 -> 保留 .part 文件，发出 paused 信号
 *   2. 用户取消 -> 保留 .part 文件，发出 canceled 信号
 *   3. 网络错误 -> 保留 .part 文件，发出 failed 信号
 *   4. 正常完成 -> .part 重命名为正式文件，发出 finished 信号
 *
 * 特殊情况：服务器不支持 Range（返回 200 而非 206），
 * 此时删除 .part 从头重新下载。
 */
void Downloader::onFinished()
{
    if (!m_reply) {
        return;
    }

    QNetworkReply *reply = m_reply;

    m_reply = nullptr;

    /*
     * 用户暂停
     */
    if (m_paused) {

        reply->deleteLater();

        closeFile();

        emit paused();

        return;
    }

    /*
     * 用户取消
     */
    if (m_canceling) {

        reply->deleteLater();

        closeFile();

        emit canceled();

        return;
    }

    /*
     * 网络错误
     */
    if (reply->error() != QNetworkReply::NoError) {

        QString error =
            reply->errorString();

        reply->deleteLater();

        closeFile();

        qDebug()<<"error::"<<error;
        emit failed(error);

        return;
    }

    /*
     * HTTP 状态码
     */
    int status =
        reply->attribute(
                 QNetworkRequest::HttpStatusCodeAttribute)
            .toInt();

    /*
     * 如果我们要求 Range，
     * 服务器必须返回 206 Partial Content。
     *
     * 如果返回 200，
     * 说明服务器没有支持 Range。
     */
    if (m_existingSize > 0 && status == 200) {

        /*
         * 当前实现为了避免文件重复拼接，
         * 删除 .part，从头重新下载。
         */
        closeFile();

        QFile::remove(m_filePath + ".part");

        m_existingSize = 0;

        /*
         * 重新启动
         */
        start(m_url, m_filePath);

        reply->deleteLater();

        return;
    }

    reply->deleteLater();

    closeFile();

    /*
     * .part -> 正式文件
     */
    QFile::remove(m_filePath);

    if (!QFile::rename(
            m_filePath + ".part",
            m_filePath)) {

        emit failed(
            QStringLiteral("下载完成，但是无法重命名文件"));

        return;
    }

    emit progress(
        QFileInfo(m_filePath).size(),
        QFileInfo(m_filePath).size(),
        0);

    emit finished(m_filePath);
}

/*
 * onError - 网络错误信号
 *
 * 真正的错误处理放在 onFinished() 中，
 * 这里仅作为兼容旧版本 Qt 的入口。
 */
void Downloader::onError(
    QNetworkReply::NetworkError code)
{
    Q_UNUSED(code)

    /*
     * 真正的处理放在 finished()
     */
}

/*
 * pause - 暂停下载
 *
 * 设置 m_paused 标志，然后 abort 请求。
 * abort 后 finished() 会被调用，
 * 在 onFinished() 中检测到 m_paused 后保留 .part 文件。
 */
void Downloader::pause()
{
    if (!m_reply) {
        return;
    }

    if (m_paused) {
        return;
    }

    m_paused = true;

    /*
     * abort 后 finished() 会被调用。
     *
     * .part 文件保留。
     */
    m_reply->abort();
}

/*
 * resume - 继续下载
 *
 * 重新调用 start()，利用已有的 .part 文件实现断点续传。
 */
void Downloader::resume()
{
    if (m_reply) {
        return;
    }

    if (!m_paused) {
        return;
    }

    m_paused = false;

    m_canceling = false;

    start(m_url, m_filePath);

    emit resumed();
}

/*
 * cancel - 取消下载
 *
 * 设置 m_canceling 标志，abort 请求。
 * finished() 中检测到后发出 canceled 信号。
 */
void Downloader::cancel()
{
    if (!m_reply) {

        if (m_file.isOpen()) {
            closeFile();
        }

        return;
    }

    m_canceling = true;

    m_paused = false;

    m_reply->abort();
}

/*
 * closeFile - 关闭并刷新文件
 */
void Downloader::closeFile()
{
    if (m_file.isOpen()) {
        m_file.flush();
        m_file.close();
    }
}

/*
 * reset - 重置所有内部状态
 */
void Downloader::reset()
{
    closeFile();

    m_existingSize = 0;
    m_totalSize = 0;
    m_lastBytes = 0;
    m_speed = 0;

    m_paused = false;
    m_canceling = false;
}

/*
 * isDownloading - 是否正在下载
 */
bool Downloader::isDownloading() const
{
    return m_reply != nullptr;
}

/*
 * isPaused - 是否已暂停
 */
bool Downloader::isPaused() const
{
    return m_paused;
}
