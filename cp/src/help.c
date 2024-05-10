#include "conplayer.h"

const char* INFO_MESSAGE =
"ConPlayer " CP_VERSION " [" CP_CPU "/" CP_OS "]\n"
"Author: https://github.com/mt1006\n"
"Version: " CP_VERSION "\n"
"Architecture: " CP_CPU "\n"
"Platform: " CP_OS;

static void helpBasicOptions(void);
static void helpAdvancedOptions(void);
static void helpModes(void);
static void helpKeyboard(void);

void showHelp(bool basic, bool advanced, bool modes, bool keyboard)
{
	puts("ConPlayer - Help\n");
	if (basic) { helpBasicOptions(); }
	if (advanced) { helpAdvancedOptions(); }
	if (modes) { helpModes(); }
	if (keyboard) { helpKeyboard(); }
}

void showInfo(void)
{
	puts(INFO_MESSAGE);
}

void showFullInfo(void)
{
	puts(
		"ConPlayer " CP_VERSION " [" CP_CPU "/" CP_OS "]\n"
		"Author: https://github.com/mt1006\n"
		"Version: " CP_VERSION "\n"
		"Architecture: " CP_CPU "\n"
		"Platform: " CP_OS);

	#if defined(_MSC_VER) 
	printf("Compiler: MSC %d [%d]\n", _MSC_VER, _MSC_FULL_VER);
	#elif defined(__clang__)
	puts("Compiler: Clang " __clang_version__);
	#elif defined(__GNUC__)
	puts("Compiler: GCC " __VERSION__);
	#else
	puts("Compiler: [unknown]");
	#endif
	
	printf("Libav: Libav %s [%s]", av_version_info(), avutil_configuration());
}

void showVersion(void)
{
	puts(CP_VERSION_STRING);
}

static void helpBasicOptions(void)
{
	puts(
		"Basic options:\n"
		" [none] / -i         Input file - audio or video.\n"
		"                     Put \"$\" before link to extract stream URL with yt-dlp (if in path).\n"
		"                     Examples:\n"
		"                      conpl video.mp4\n"
		"                      conpl $https://www.youtube.com/watch?v=FtutLA63Cp8\n"
		" -c [mode]           Sets color mode. By default \"cstd-256\".\n"
		"  (--colors)         To get list of all available modes use \"conpl -h modes\".\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -c winapi-16\n"
		" -vol [volume]       Sets audio volume. By default \"0.5\".\n"
		"  (--volume)         Examples:\n"
		"                      conpl video.mp4 -vol 0.2\n"
		" -s [w] [h]          Sets width and height of the drawn image.\n"
		"  (--size)           By default size of entire window.\n"
		"                     Using \"-s 0 0\" image size will be constant.\n"
		"                     (will not change with the console size change).\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -s 120 50\n"
		" -f  (--fill)        Fills entire available area, without keeping ratio.\n"
		" -inf(--information) Information about ConPlayer.\n"
		" -v  (--version)     Information about ConPlayer version.\n"
		" -h <topic>          Displays help message.\n"
		"  (--help)           Topics: basic, advanced, modes, keyboard, full\n");
}

static void helpAdvancedOptions(void)
{
	puts(
		"Advanced options:\n"
		" -int [divisor]      Uses interlacing to draw frames.\n"
		"      <height>       The larger the divisor, fewer scanlines there are per frame.\n"
		"  (--interlaced)     Height is optional parameter and means scanline height - by default 1.\n"
		"                     When divisor is equal to 1, then interlacing is disabled.\n"
		"                     Note: in winapi mode instead of improve performance, it may reduce it.\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -int 2\n"
		"                      conpl video.mp4 -int 4 3\n"
		" -cp [mode]          Sets color processing mode\n"
		"  (--color-proc)     To get list of all available modes use \"conpl -h modes\".\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -c cstd-rgb -cp none\n"
		" -sc [value]         Sets constant color in grayscale mode.\n"
		"  (--set-color)      Only number - sets text attribute using WinAPI.\n"
		"                     \"@color\" - sets color from ANSI 256 palette (only with cstd-gray).\n"
		"                     \"#RRGGBB\" - sets RGB color (only with cstd-gray).\n"
		"                     With \"@\" or \"#\" you can give second argument as background color.\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -c winapi-gray -sc 4\n"
		"                      conpl video.mp4 -c cstd-gray -sc @93\n"
		"                      conpl video.mp4 -c cstd-gray -sc #FF00FF\n"
		"                      conpl video.mp4 -c cstd-gray -sc #FF00FF FF0000\n"
		" -cs [charset]       Sets character set used for drawing frames.\n"
		"  (--charset)        Takes name of file with charset or name of predefined charset.\n"
		"                     Predefined charsets: #long, #short, #2, #blocks, #outline, #bold-outline.\n"
		"                     Default charset is \"#long\".\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -ch #blocks\n"
		"                      conpl video.mp4 -ch my_charset.txt\n"
		" -r [val]            Randomly increases or decreases pixel brightness by a random value\n"
		"  (--rand)           between 0 and val/2. When color processing mode is set to \"none\" or\n"
		"                     \"@\" sign is placed before the number, brightness is decreased by\n"
		"                     a random value between 0 and val.\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -r 20\n"
		"                      conpl video.mp4 -r @40\n"
		"                      conpl video.mp4 -c cstd-rgb -cp char-only -r 56\n"
		" -sm [mode]          Sets scaling mode. Default scaling mode is \"bicubic\".\n"
		"  (--scaling-mode)   To get list of all available modes use \"conpl -h modes\".\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -sm nearest\n"
		" -fr [ratio]         Sets constant font ratio (x/y).\n"
		"  (--font-ratio)     On Windows by default font ratio is obtained using WinAPI.\n"
		"                     On Linux or if it's not posible, it's set to 8/18 (about 0.44).\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -ft 0.5\n"
		" -sy [mode]          Sets synchronization mode.\n"
		"  (--sync)           To get list of all available modes use \"conpl -h modes\".\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -sy draw-all\n"
		" -vf [filter]        Applies FFmpeg filters to the video.\n"
		"  (--video-filters)  FFmpeg filters documentation: https://www.ffmpeg.org/ffmpeg-filters.html\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -vf \"colorchannelmixer=gg=0:bb=0\"\n"
		"                      conpl video.mp4 -vf \"split[a][t];[t]crop=iw:ih/2:0:0,vflip[b];[a][b]overlay=0:H/2\"\n"
		" -svf [filter]       Applies FFmpeg filters to the video after scaling.\n"
		"  (--scaled-video-   Examples:\n"
		"     filters)         conpl video.mp4 -svf \"rgbashift=rh=-2:bh=2\"\n"
		"                      conpl video.mp4 -svf \"noise=alls=8:allf=t\"\n"
		" -af [filter]        Applies FFmpeg filters to the audio.\n"
		"  (--audio-filters)  Examples:\n"
		"                      conpl video.mp4 -af \"aecho=0.8:0.88:60:0.4\"\n"
		"                      conpl video.mp4 -af \"volume=80.0\"\n"
		" -dcls               Disables clearing screen every new frame.\n"
		"   (--disable-cls)   Uses new line instead. Useful for printing output to file.\n"
		"                     Works properly only in \"cstd\" color mode and it breaks interlacing.\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -c cstd-gray -s 80 30 -fr 0.5 -sy disabled -dcls > output.txt\n"
		" -fc                 Creates child window on top of the console that looks like console but\n"
		"  (--fake-console)   renders text much faster using OpenGL. Currently works only on Windows\n"
		"                     and may be unstable! Recommended to use with raster font.\n"
		" -gls                Settings for \"fake console\" mode. Experimental!\n"
		"  (--opengl-settings)Examples:\n"
		"                      conpl video.mp4 -c cstd-rgb -cp char-only -r 60 -fc -gls :s3:fsh:add predef dither :s3:set lerp_a 0.5\n"
		" -xmh                Sets maximum video for video extracted from URL. By default 480.\n"
		"  (--extractor-max-  If such video format isn't available it will search for worst quality video.\n"
		"     height)         When set to 0 it will search only for format with best quality video.\n"
		"                     Examples:\n"
		"                      conpl $https://www.youtube.com/watch?v=FtutLA63Cp8 -xmh 144\n"
		" -xp                 Sets extractor command prefix. Doesn't work with \"-xmh\" option.\n"
		"  (--extractor-      By default: \"yt-dlp -f \"bv*[height<=%%d]+ba/bv*[height<=%%d]/wv*+ba/w\" --no-warnings --get-url\",\n"
		"   prefix)           where %%d is maximum video height, 480 by default.\n"
		"                     When maximum video height is set to 0: \"yt-dlp --no-warnings --get-url\".\n"
		"                     Examples:\n"
		"                      conpl $https://www.youtube.com/watch?v=FtutLA63Cp8 -xp \"yt-dlpppx --no-warnings --get-url\"\n"
		" -xs                 Sets extractor command suffix. By default: \"2>&1\".\n"
		"  (--extractor-      Examples:\n"
		"   suffix)            conpl $https://www.youtube.com/watch?v=FtutLA63Cp8 -xs \"\"\n"
		" -pl (--preload)     Loads and unload entire input file (in hope that system will cache it into RAM).\n"
		" -da(--disable-audio)Disables audio.\n"
		" -dk (--disable-keys)Disables keyboard control.\n"
		" -avl (--libav-logs) Enables printing Libav logs. Helpful with FFmpeg filters problems.\n"
		" -fi (--full-info)   Full info about ConPlayer.\n");
}

static void helpModes(void)
{
	puts(
		"Color modes:\n"
		" >winapi-gray [Windows-only]\n"
		" >winapi-16 [Windows-only]\n"
		" >cstd-gray\n"
		" >cstd-16\n"
		" >cstd-256 [default]\n"
		" >cstd-rgb\n");

	puts(
		"Color processing modes:\n"
		" >none - keeps original color and uses single character.\n"
		" >char-only - uses character according to luminance but doesn't change color.\n"
		" >both - uses character according to luminance and changes color so that\n"
		"         its largest component is equal to 255. [default]\n");

	puts(
		"Scaling modes:\n"
		" >nearest\n"
		" >fast-bilinear\n"
		" >bilinear\n"
		" >bicubic [default]\n");

	puts(
		"Synchronization modes:\n"
		" >disabled - prints the output as fast as possible.\n"
		" >draw-all - synchronization enabled, but tries to draw all frames.\n"
		" >enabled - synchronization enabled, skips frames if necessary. [default]\n");
}

static void helpKeyboard(void)
{
	puts(
		"Keyboard control:\n"
		" Space       Pause/Play\n"
		" L/R arrows  Go back / Skip forward (10 second)\n"
		" -/+ keys    Go back / Skip forward (30 second)\n"
		" U/D arrows  Turn down/up the volume\n"
		" M           Mute the audio\n"
		" ESC/Q       Exit\n");
}