# ConPlayer
Allows you to play videos in console.

Inspired by [mariiaan/CmdPlay](https://github.com/mariiaan/CmdPlay) with significant improvements:
- plays videos in color
- uses libav instead of ffmpeg executable to read video and converts it on the fly, so it loads instantly
- allows you to resize console while playing video and it keeps its correct aspect ratio
- you can go back and skip forward video
- it has multiple advanced options like enabling interlacing, changing charset or applying FFmpeg filters

[Download Version 1.5](https://github.com/mt1006/ConPlayer/releases/tag/ConPlayer-1.5)

Demonstration: https://www.youtube.com/watch?v=nbUKhalJATk

# Examples

### "Bad Apple!!" in cmd

![Bad Apple!!](screenshots/bad_apple.png "Bad Apple!!")

### Also "Bad Apple!!" in cmd but with FFmpeg filters

`conpl ba.mp4 -vf "curves=m=0/0 .8/.8 1/0" -svf "noise=alls=8:allf=t,rgbashift=rh=-2:bh=2"`

![Bad Apple!!](screenshots/bad_apple2.png "Bad Apple!!")

### Some random 4K test video with colors in Windows Terminal (https://www.youtube.com/watch?v=xcJtL7QggTI&t=96)

![Colors](screenshots/colors.png "Colors")

### "Bad Apple!!" in "fake console" mode with Windows console raster font in Wine

`wine conpl.exe ba.mp4 -fc -gls :win:type normal :font Terminal 8 12`

It requires of course ConPlayer binaries for Windows. To use "Terminal" you need to also copy Windows 10 or 11 fonts (C:\\Windows\\Fonts) to corresponding location in Wine. It's also possible to use `:enum-fonts` and `:enum-font (font name)` flags to list available fonts.

![wine](screenshots/bad_apple3.png "Wine")

# Options

## Basic options
```
 [none] / -i         Input file - audio or video.
                     Put "$" before link to extract stream URL with yt-dlp (if in path).
                     Examples:
                      conpl video.mp4
                      conpl $https://www.youtube.com/watch?v=FtutLA63Cp8
 -c [mode]           Sets color mode. By default "cstd-256".
  (--colors)         To get list of all available modes use "conpl -h modes".
                     Examples:
                      conpl video.mp4 -c winapi-16
 -vol [volume]       Sets audio volume. By default "0.5".
  (--volume)         Examples:
                      conpl video.mp4 -vol 0.2
 -s [w] [h]          Sets width and height of the drawn image.
  (--size)           By default size of entire window.
                     Using "-s 0 0" image size will be constant.
                     (will not change with the console size change).
                     Examples:
                      conpl video.mp4 -s 120 50
 -f  (--fill)        Fills entire available area, without keeping ratio.
 -inf(--information) Information about ConPlayer.
 -v  (--version)     Information about ConPlayer version.
 -h <topic>          Displays help message.
  (--help)           Topics: basic, advanced, modes, keyboard, full
 ```

## Advanced options
```
 -int [divisor]      Uses interlacing to draw frames.
      <height>       The larger the divisor, fewer scanlines there are per frame.
  (--interlaced)     Height is optional parameter and means scanline height - by default 1.
                     When divisor is equal to 1, then interlacing is disabled.
                     Note: in winapi mode instead of improve performance, it may reduce it.
                     Examples:
                      conpl video.mp4 -int 2
                      conpl video.mp4 -int 4 3
 -cp [mode]          Sets color processing mode
  (--color-proc)     To get list of all available modes use "conpl -h modes".
                     Examples:
                      conpl video.mp4 -c cstd-rgb -cp none
 -sc [value]         Sets constant color in grayscale mode.
  (--set-color)      Only number - sets text attribute using WinAPI.
                     "@color" - sets color from ANSI 256 palette (only with cstd-gray).
                     "#RRGGBB" - sets RGB color (only with cstd-gray).
                     With "@" or "#" you can give second argument as background color.
                     Examples:
                      conpl video.mp4 -c winapi-gray -sc 4
                      conpl video.mp4 -c cstd-gray -sc @93
                      conpl video.mp4 -c cstd-gray -sc #FF00FF
                      conpl video.mp4 -c cstd-gray -sc #FF00FF FF0000
 -cs [charset]       Sets character set used for drawing frames.
  (--charset)        Takes name of file with charset or name of predefined charset.
                     Predefined charsets: #long, #short, #2, #blocks, #outline, #bold-outline.
                     Default charset is "#long".
                     Examples:
                      conpl video.mp4 -ch #blocks
                      conpl video.mp4 -ch my_charset.txt
 -r [val]            Randomly increases or decreases pixel brightness by a random value
  (--rand)           between 0 and val/2. When color processing mode is set to "none" or
                     "@" sign is placed before the number, brightness is decreased by
                     a random value between 0 and val.
                     Examples:
                      conpl video.mp4 -r 20
                      conpl video.mp4 -r @40
                      conpl video.mp4 -c cstd-rgb -cp char-only -r 56
 -sm [mode]          Sets scaling mode. Default scaling mode is "bicubic".
  (--scaling-mode)   To get list of all available modes use "conpl -h modes".
                     Examples:
                      conpl video.mp4 -sm nearest
 -fr [ratio]         Sets constant font ratio (x/y).
  (--font-ratio)     On Windows by default font ratio is obtained using WinAPI.
                     On Linux or if it's not posible, it's set to 8/18 (about 0.44).
                     Examples:
                      conpl video.mp4 -ft 0.5
 -sy [mode]          Sets synchronization mode.
  (--sync)           To get list of all available modes use "conpl -h modes".
                     Examples:
                      conpl video.mp4 -sy draw-all
 -vf [filter]        Applies FFmpeg filters to the video.
  (--video-filters)  FFmpeg filters documentation: https://www.ffmpeg.org/ffmpeg-filters.html
                     Examples:
                      conpl video.mp4 -vf "colorchannelmixer=gg=0:bb=0"
                      conpl video.mp4 -vf "split[a][t];[t]crop=iw:ih/2:0:0,vflip[b];[a][b]overlay=0:H/2"
 -svf [filter]       Applies FFmpeg filters to the video after scaling.
  (--scaled-video-   Examples:
     filters)         conpl video.mp4 -svf "rgbashift=rh=-2:bh=2"
                      conpl video.mp4 -svf "noise=alls=8:allf=t"
 -af [filter]        Applies FFmpeg filters to the audio.
  (--audio-filters)  Examples:
                      conpl video.mp4 -af "aecho=0.8:0.88:60:0.4"
                      conpl video.mp4 -af "volume=80.0"
 -dcls               Disables clearing screen every new frame.
   (--disable-cls)   Uses new line instead. Useful for printing output to file.
                     Works properly only in "cstd" color mode and it breaks interlacing.
                     Examples:
                      conpl video.mp4 -c cstd-gray -s 80 30 -fr 0.5 -sy disabled -dcls > output.txt
 -fc                 Creates child window on top of the console that looks like console but
  (--fake-console)   renders text much faster using OpenGL. Currently works only on Windows
                     and may be unstable! Recommended to use with raster font.
 -gls                Settings for "fake console" mode. Experimental!
  (--opengl-settings)Examples:
                      conpl video.mp4 -c cstd-rgb -cp char-only -r 60 -fc -gls :s3:fsh:add predef dither :s3:set lerp_a 0.5
 -xmh                Sets maximum video for video extracted from URL. By default 480.
  (--extractor-max-  If such video format isn't available it will search for worst quality video.
     height)         When set to 0 it will search only for format with best quality video.
                     Examples:
                      conpl $https://www.youtube.com/watch?v=FtutLA63Cp8 -xmh 144
 -xp                 Sets extractor command prefix. Doesn't work with "-xmh" option.
  (--extractor-      By default: "yt-dlp -f "bv*[height<=%%d]+ba/bv*[height<=%%d]/wv*+ba/w" --no-warnings --get-url",
   prefix)           where %%d is maximum video height, 480 by default.
                     When maximum video height is set to 0: "yt-dlp --no-warnings --get-url".
                     Examples:
                      conpl $https://www.youtube.com/watch?v=FtutLA63Cp8 -xp "yt-dlpppx --no-warnings --get-url"
 -xs                 Sets extractor command suffix. By default: "2>&1".
  (--extractor-      Examples:
   suffix)            conpl $https://www.youtube.com/watch?v=FtutLA63Cp8 -xs ""
 -pl (--preload)     Loads and unload entire input file (in hope that system will cache it into RAM).
 -da(--disable-audio)Disables audio.
 -dk (--disable-keys)Disables keyboard control.
 -avl (--libav-logs) Enables printing Libav logs. Helpful with FFmpeg filters problems.
 -fi (--full-info)   Full info about ConPlayer.
```

## Color modes
```
 >winapi-gray [Windows-only]
 >winapi-16 [Windows-only]
 >cstd-gray
 >cstd-16
 >cstd-256 [default]
 >cstd-rgb
```

## Color processing modes
```
 >none - keeps original color and uses single character.
 >char-only - uses character according to luminance but doesn't change color.
 >both - uses character according to luminance and changes color so that
         its largest component is equal to 255. [default]
```

## Scaling modes
```
 >nearest
 >fast-bilinear
 >bilinear
 >bicubic [default]
```

## Synchronization modes
```
 >disabled - prints the output as fast as possible.
 >draw-all - synchronization enabled, but tries to draw all frames.
 >enabled - synchronization enabled, skips frames if necessary. [default]
```

## Keyboard control
```
 Space       Pause/Play
 L/R arrows  Go back / Skip forward (10 second)
 -/+ keys    Go back / Skip forward (30 second)
 U/D arrows  Turn down/up the volume
 M           Mute the audio
 ESC/Q       Exit
```

# Compiling

## Windows

To compile ConPlayer on Windows you first need to have the Visual Studio. Then install vcpkg and integrate it with Visual Studio. Then you need to install libav (FFmpeg's library) and libao.

Installation and configuration of vcpkg:
```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && bootstrap-vcpkg.bat -disableMetrics
vcpkg integrate install
```

Installation of required libraries (on x86-64):
```
vcpkg install ffmpeg[gpl,freetype,fontconfig,fribidi,ass,opencl,dav1d]:x64-windows
vcpkg install libao:x64-windows
```

Now you can just open .sln file and everything should work (at least in theory).

## Linux (dpkg)

Installation of required packages:
```
sudo apt install build-essential libavcodec-dev libavformat-dev libavfilter-dev libswscale-dev libao-dev
```

Use `make` to compile.

## Linux (pacman)

Installation of required packages:
```
sudo pacman -S --needed base-devel ffmpeg libao
```

Use `make` to compile.

## Termux

Full list of steps to build and run:
```
pkg upgrade
pkg install git build-essential ffmpeg libao
git clone https://github.com/mt1006/ConPlayer.git
cd ConPlayer
make
./conpl
```