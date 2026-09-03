#ifndef DOWNLOADTASK_H
#define DOWNLOADTASK_H

#include <QString>

struct DownloadTask
{
    enum Type {
        Direct,
        M3U8,
        YouTube
    };

    Type type = Direct;
    QString url;
    QString outputPath;
};

#endif
