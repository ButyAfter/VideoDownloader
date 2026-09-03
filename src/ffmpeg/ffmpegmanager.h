#ifndef FFMPEGMANAGER_H
#define FFMPEGMANAGER_H

#include <QString>

class FFmpegManager
{
public:
    FFmpegManager();

    bool concat(const QString &concatFile, const QString &outputFile);
    QString executable() const;
    QString lastError() const;

private:
    QString m_lastError;
};

#endif
