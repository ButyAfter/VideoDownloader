#include "urlresolver.h"

#include <QUrl>

bool UrlResolver::isYouTube(const QString &url)
{
    QUrl u(url);
    QString host = u.host().toLower();

    return host == "youtube.com" ||
           host.endsWith(".youtube.com") ||
           host == "www.youtube-nocookie.com" ||
           host == "youtu.be";
}

bool UrlResolver::looksLikeM3U8(const QString &url)
{
    QString lower = url.toLower();
    return lower.contains(".m3u8") ||
           lower.contains("application/vnd.apple.mpegurl");
}

UrlResolver::Type UrlResolver::resolve(const QString &url)
{
    if (isYouTube(url))
        return YouTube;

    if (looksLikeM3U8(url))
        return M3U8;

    QUrl u(url);
    if (u.isValid() && !u.scheme().isEmpty())
        return Direct;

    return Unknown;
}
