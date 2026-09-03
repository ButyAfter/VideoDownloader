#include "m3u8parser.h"

#include <QRegularExpression>

static QString attributeValue(const QString &line, const QString &name)
{
    QRegularExpression re(name + "=([^,]*)");
    QRegularExpressionMatch m = re.match(line);
    return m.hasMatch() ? m.captured(1) : QString();
}

M3U8Playlist M3U8Parser::parse(const QByteArray &data, const QUrl &baseUrl)
{
    M3U8Playlist result;
    result.baseUrl = baseUrl;

    const QString text = QString::fromUtf8(data);
    const QStringList lines = text.split(QRegularExpression("[\\r\\n]+"),
                                         Qt::SkipEmptyParts);

    if (lines.isEmpty() || lines.first().trimmed() != "#EXTM3U")
        return result;

    result.valid = true;

    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines.at(i).trimmed();

        if (line.startsWith("#EXT-X-KEY:")) {
            result.encrypted = true;
        }

        if (line.startsWith("#EXT-X-STREAM-INF:")) {
            result.master = true;

            M3U8Variant variant;
            QString bw = attributeValue(line, "BANDWIDTH");
            variant.bandwidth = bw.toLongLong();
            variant.resolution = attributeValue(line, "RESOLUTION");
            variant.codecs = attributeValue(line, "CODECS");

            for (int j = i + 1; j < lines.size(); ++j) {
                QString next = lines.at(j).trimmed();
                if (!next.isEmpty() && !next.startsWith("#")) {
                    variant.url = baseUrl.resolved(QUrl(next)).toString();
                    i = j;
                    break;
                }
            }

            if (!variant.url.isEmpty())
                result.variants.append(variant);
        }
    }

    if (result.master)
        return result;

    double pendingDuration = 0.0;
    int sequence = 0;

    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines.at(i).trimmed();

        if (line.startsWith("#EXT-X-MEDIA-SEQUENCE:")) {
            sequence = line.section(':', 1).toInt();
        } else if (line.startsWith("#EXTINF:")) {
            pendingDuration =
                line.section(':', 1).section(',', 0, 0).toDouble();
        } else if (!line.startsWith("#") && !line.isEmpty()) {
            M3U8Segment seg;
            seg.duration = pendingDuration;
            seg.sequence = sequence++;
            seg.url = baseUrl.resolved(QUrl(line)).toString();
            result.segments.append(seg);
            pendingDuration = 0.0;
        }
    }

    return result;
}
