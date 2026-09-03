# AGENTS.md

## Build & Run

- **Build system:** qmake (`VideoDownloader.pro`)
- **Qt modules required:** `core gui network widgets`
- **Build:** `qmake VideoDownloader.pro && make` (or open in Qt Creator and build)
- **Run:** `./VideoDownloader` (output in `build/` or project root)

## Architecture

Simple single-window Qt5 app (C++11):

- `main.cpp` — entrypoint, sets Microsoft YaHei font, launches `MainWindow`
- `mainwindow.h/cpp` — GUI: URL input, save path, start/pause/cancel buttons, progress bar, log
- `downloader.h/cpp` — `QNetworkAccessManager`-based HTTP downloader with resume support (Range header), speed calculation

`Downloader` signals (`progress`, `started`, `finished`, `failed`, `paused`, `resumed`, `canceled`) drive all UI updates. No MVC separation — direct signal/slot wiring in `MainWindow`.

## Conventions

- Header guards: `#ifndef CLASSNAME_H` / `#define CLASSNAME_H`
- Qt string literals: `QStringLiteral()` for Chinese UI text
- C++11 (`CONFIG += c++11` in `.pro`)
- No tests, no CI, no linting configured
