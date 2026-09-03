# VideoDownloader

Qt 5.12.3 + MinGW 7.3.0 的模块化视频下载器基础工程。

## 当前功能

- 普通 HTTP/HTTPS 文件下载
- `.part` 临时文件
- HTTP Range 断点续传
- 暂停 / 继续 / 取消
- M3U8 Master Playlist
- M3U8 Media Playlist
- 相对 URL 自动解析
- TS / fMP4 分片下载
- FFmpeg 合并/封装
- YouTube URL 通过 yt-dlp 调用外部程序下载
- 模块化目录结构

## 目录

```text
VideoDownloader/
├── VideoDownloader.pro
├── README.md
├── tools/
│   └── README.md
├── src/
│   ├── core/
│   ├── http/
│   ├── m3u8/
│   ├── ffmpeg/
│   ├── youtube/
│   └── resolver/
└── ui/
```

## 编译

使用 Qt Creator 打开：

```text
VideoDownloader.pro
```

选择 Qt 5.12.3 MinGW 64-bit Kit。

## 外部工具

如果要下载 YouTube，请把：

```text
yt-dlp.exe
ffmpeg.exe
```

放到：

```text
VideoDownloader/tools/
```

程序也会自动尝试从系统 PATH 中寻找。

## 说明

本项目只处理你有权下载的内容，不实现 DRM 破解、登录保护绕过或访问控制绕过。

M3U8 加密流中的 `#EXT-X-KEY` 当前会直接拒绝，避免把受保护内容解密逻辑混进基础下载器。



## 测试

```
https://youtu.be/avpxJsZZJUM?si=_WsqI1EYJa1_ssNh
yt-dlp.exe --list-formats "https://youtu.be/avpxJsZZJUM?si=_WsqI1EYJa1_ssNh"

yt-dlp.exe -f "bv*+ba/b" --merge-output-format mp4 "https://youtu.be/avpxJsZZJUM?si=_WsqI1EYJa1_ssNh"
```

