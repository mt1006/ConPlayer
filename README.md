# ConPlayer
Allows you to play videos in console.

Inspired by [mariiaan/CmdPlay](https://github.com/mariiaan/CmdPlay) with some significant improvements:
- it allows you to play video in color
- it uses libav instead of ffmpeg executable to read video and converts it on the fly, so it loads instantly
- it allows you to resize console while playing video and it keeps its correct aspect ratio
- allows you to go back and skip forward video

[Download Version 1.2](https://github.com/mt1006/ConPlayer/releases/tag/ConPlayer-1.2)

Demonstration: https://www.youtube.com/watch?v=nbUKhalJATk

# Examples

"Bad Apple!!" in cmd

![Bad Apple!!](screenshots/bad_apple.png "Bad Apple!!")

Some random 4K test video from yt with colors in Windows Terminal (https://www.youtube.com/watch?v=xcJtL7QggTI&t=96)

![Colors](screenshots/colors.png "Colors")
To get colors add "-c" option. Also when playing with colors use Windows Terminal instead of cmd for better performance. You can also improve performance by using interlacing ("-int"), to see full documentation use "-h full".

# List of basic options
```
 [none] / -i         Just input file - audio or video.
                     Examples:
                      conpl video.mp4
 -c <mode>           Sets color mode.
  (--colors)         Default color mode is "winapi-gray".
                     With "-c", with no mode specified, default is "cstd-256".
                     To get list of all avaible color modes use "conpl -h color-modes"
                     Examples:
                      conpl video.mp4 -c
                      conpl video.mp4 -c winapi-16
 -vol [volume]       Sets audio volume. By default "0.5".
  (--volume)         Examples:
                      conpl video.mp4 -vol 0.2
 -s [w] [h]          Sets width and height of the drawn image.
  (--size)           By default size of entire window.
                     Using "-s 0 0" image size will be constant
                     (will not change with the console size change).
                     Examples:
                      conpl video.mp4 -s 120 50
 -f  (--fill)        Fills entire available area, without keeping ratio.
 -inf(--information) Information about ConPlayer.
 -v  (--version)     Information about ConPlayer version.
 -h <topic>          Displays help message.
  (--help)           Topics: basic, advanced, color-modes, keyboard, full 
  
[To see full help use "conpl -h full"]
 ```

# Keyboard control
```
 Space - Pause/Play
 "[" / "]" - Go back/Skip forward
 "L" / "O" - Turn down/up the volume
 ESC - Exit
```

# Compiling

## Windows
To compile it on Windows you need Visual Studio. First, install vcpkg and integrate it with Visual Studio. Here is how to do it: https://vcpkg.io/en/getting-started.html. Then you need to install libav (ffmpeg's library) and libao.
```
vcpkg install ffmpeg:x64-windows
vcpkg install portaudio:x64-windows
```
Now you can just open .sln file and everything should work (at least theoretically).\

## Linux
To compile it on Linux you need GCC. You also need ffmpeg (libav) and libao libraries. If you're using apt package manager, you can install them with these commands:
```
sudo apt-get install libavcodec-dev
sudo apt-get install libavformat-dev
sudo apt-get install libswscale-dev
*    sudo apt-get install libao-dev
```
Then you can just use ```make```.