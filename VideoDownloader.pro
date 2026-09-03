QT += core gui network widgets

CONFIG += c++11

TARGET = VideoDownloader
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    downloader.cpp

HEADERS += \
    mainwindow.h \
    downloader.h