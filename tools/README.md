把第三方命令行工具放在这里：

- yt-dlp.exe
- ffmpeg.exe
- ffprobe.exe（可选）

程序优先寻找程序目录下的 tools/，其次寻找系统 PATH。

YouTube 下载模块通过 QProcess 调用 yt-dlp。

