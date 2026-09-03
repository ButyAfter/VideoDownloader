#ifndef URLRESOLVER_H
#define URLRESOLVER_H

#include <QString>

class UrlResolver
{
public:
    enum Type {
        Direct,
        M3U8,
        YouTube,
        Unknown
    };

    static Type resolve(const QString &url);
    static bool isYouTube(const QString &url);
    static bool looksLikeM3U8(const QString &url);
};

#endif
