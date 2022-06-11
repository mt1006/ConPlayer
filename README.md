# ConPlayer
Allows you to play videos in console.

Inspired by [mariiaan/CmdPlay](https://github.com/mariiaan/CmdPlay) with some significant improvements:
- it allows you to play video in color
- it uses libav instead of ffmpeg executable to read video and converts it on the fly, so it loads instantly
- it allows you to resize console while playing video and it keeps its correct aspect ratio
- allows you to go back and skip forward video

Plans for future:
- version for Linux
- better audio/video sync (maybe I'll switch from libao to PortAudio)
- more options

# Examples

"Bad Apple!!" in cmd

![Bad Apple!!](screenshots/bad_apple.png "Bad Apple!!")

Some random 4K test video from yt with colors in Windows Terminal (https://www.youtube.com/watch?v=xcJtL7QggTI&t=96)

![Colors](screenshots/colors.png "Colors")
Btw by using "-c" option you get by default only 16 colors. To get 256 colors use "-c" and "-cstd". Also when using colors with "-cstd" use Windows Terminal instead of cmd for better performance. You can also improve performance by using "-int" (check out list below).

# List of options
```
 [none] / -i         Just input file - audio or video.
                     Examples:
                      conpl video.mp4
 -s [w] [h]          Sets width and height of the drawn image.
  (--size)           By default size of entire window.
                     Using "-s 0 0" image size will be constant
                     (will not change with the console size change).
                     Examples:
                      conpl video.mp4 -s 120 50
 -vol [volume]       Sets audio volume. By default "0.5".
  (--volume)         Examples:
                      conpl video.mp4 -v 0.2
 -c (--colors)       Colors (by default - 16, with "-cstd" - 256).
                     On older Windows versions colors with "-cstd" may not work properly!
 -f  (--fill)        Fills entire available area, without keeping ratio.
 -int [divisor]      Uses interlacing to draw frames (works only with "-cstd").
  (--interlaced)     The larger the divisor, fewer scanlines there are per frame.
                     When divisor is equal to 1, then interlacing is disabled.
                     Examples:
                      conpl video.mp4 -int 2
 -cstd (--c-std-out) Uses C std functions instead of WinAPI.
 -inf(--information) Information about ConPlayer.
 -fi (--full-info)   Full info about ConPlayer.
 -v  (--version)     Information about ConPlayer version.
 -h / -? (--help)    Displays this help message.
 ```

# Keyboard control
```
 Space - Pause/Play
 "[" / "]" - Go back/Skip forward
 ESC - Exit
```

# Compiling
To compile it you need Visual Studio. First you need to install vcpkg and integrate it with Visual Studio. Here is how to do it: https://vcpkg.io/en/getting-started.html. Then you need to install libav (ffmpeg's library) and libao.
```
vcpkg install ffmpeg:x64-windows
vcpkg install libao:x64-windows
```
Now you can just open .sln file and everything should work (at least theoretically).
