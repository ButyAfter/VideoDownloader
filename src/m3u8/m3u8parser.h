#ifndef M3U8PARSER_H
#define M3U8PARSER_H

#include <QString>
#include <QVector>
#include <QUrl>

struct M3U8Segment
{
    QString url;
    double duration = 0.0;
    int sequence = 0;
};

struct M3U8Variant
{
    QString url;
    qint64 bandwidth = 0;
    QString resolution;
    QString codecs;
};

struct M3U8Playlist
{
    bool valid = false;
    bool master = false;
    bool encrypted = false;
    QUrl baseUrl;
    QVector<M3U8Variant> variants;
    QVector<M3U8Segment> segments;
};

class M3U8Parser
{
public:
    static M3U8Playlist parse(const QByteArray &data, const QUrl &baseUrl);
};

#endif
