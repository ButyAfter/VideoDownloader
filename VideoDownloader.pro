QT += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
TEMPLATE = app
TARGET = VideoDownloader

SOURCES += \
    src/main.cpp \
    src/core/downloadtask.cpp \
    src/core/downloadmanager.cpp \
    src/http/httpdownloader.cpp \
    src/m3u8/m3u8parser.cpp \
    src/m3u8/m3u8downloader.cpp \
    src/ffmpeg/ffmpegmanager.cpp \
    src/youtube/youtubedownloader.cpp \
    src/resolver/urlresolver.cpp \
    ui/mainwindow.cpp

HEADERS += \
    src/core/downloadtask.h \
    src/core/downloadmanager.h \
    src/http/httpdownloader.h \
    src/m3u8/m3u8parser.h \
    src/m3u8/m3u8downloader.h \
    src/ffmpeg/ffmpegmanager.h \
    src/youtube/youtubedownloader.h \
    src/resolver/urlresolver.h \
    ui/mainwindow.h

INCLUDEPATH += \
    src \
    src/core \
    src/http \
    src/m3u8 \
    src/ffmpeg \
    src/youtube \
    src/resolver \
    ui

win32:CONFIG += console
