#include "conplayer.h"

static void helpBasicOptions(void);
static void helpAdvancedOptions(void);
static void helpColorModes(void);
static void helpScalingModes(void);
static void helpKeyboard(void);

void showHelp(bool basic, bool advanced, bool colorModes, bool scalingModes, bool keyboard)
{
	puts("ConPlayer - Help\n");
	if (basic) { helpBasicOptions(); }
	if (advanced) { helpAdvancedOptions(); }
	if (colorModes) { helpColorModes(); }
	if (scalingModes) { helpScalingModes(); }
	if (keyboard) { helpKeyboard(); }
}

void showInfo(void)
{
	puts(
		"ConPlayer - Information\n"
		"ConPlayer " CP_VERSION " [" CP_CPU "/" CP_OS "]\n"
		"Author: https://github.com/mt1006\n"
		"Version: " CP_VERSION "\n"
		"Architecture: " CP_CPU "\n"
		"Platform: " CP_OS);
}

void showFullInfo(void)
{
	puts(
		"ConPlayer - Full info\n"
		"ConPlayer " CP_VERSION " [" CP_CPU "/" CP_OS "]\n"
		"Author: https://github.com/mt1006\n"
		"Version: " CP_VERSION "\n"
		"Architecture: " CP_CPU "\n"
		"Platform: " CP_OS);

	#if defined(_MSC_VER) 
	printf("Compiler: MSC %d [%d]\n", _MSC_VER, _MSC_FULL_VER);
	#elif defined(__GNUC__)
	puts("Compiler: GCC " __VERSION__);
	#else
	puts("Compiler: [unknown]");
	#endif
	
	printf("Libav: Libav %s [%s]", av_version_info(), avutil_configuration());
}

void showVersion(void)
{
	puts("ConPlayer " CP_VERSION " [" CP_CPU "/" CP_OS "]");
}

void showNoArgsInfo(void)
{
	showVersion();
	puts("\nNo arguments were given!\n"
		"To start playing give the video filename as the first argument.\n"
		"To get list of basic options use \"conpl -h\".\n"
		"To get list of all options use \"conpl -h full\".");
}

static void helpBasicOptions(void)
{
	puts(
		"Basic options:\n"
		" [none] / -i         Just input file - audio or video.\n"
		"                     Examples:\n"
		"                      conpl video.mp4\n"
		" -c [mode]           Sets color mode. By default \"cstd-256\".\n"
		"  (--colors)         To get list of all avaible color modes use \"conpl -h color-modes\".\n"
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
		"  (--help)           Topics: basic, advanced, color-modes, scaling-modes, keyboard, full\n");
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
		" -sch                Uses single character to draw image and sets it's color to original\n"
		"  (--single-char)    instead of recalculated. Requires colors!\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -c cstd-rgb -sch\n"
		" -r [val]            Randomly increases or decreases pixel brightness by a random value\n"
		"  (--rand)           between 0 and val/2. When \"single char\" mode is enabled or \"@\" sign\n"
		"                     is placed before the number, brightness is decreased by a random value\n"
		"                     between 0 and val.\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -r 20\n"
		"                      conpl video.mp4 -r @40\n"
		"                      conpl video.mp4 -c cstd-rgb -sch -r 56\n"
		" -sm [mode]          Sets scaling mode. Default scaling mode is \"bicubic\".\n"
		"  (--scaling-mode)   To get list of all avaible scaling modes use \"conpl -h scaling-modes\".\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -sm nearest\n"
		" -fr [ratio]         Sets constant font ratio (x/y).\n"
		"  (--font-ratio)     On Windows by default font ratio is obtained using WinAPI.\n"
		"                     On Linux or if it's not posible, it's set to 8/18 (about 0.44).\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -ft 0.5\n"
		" -sy [mode]          Sets synchronization mode.\n"
		"  (--sync)           Available modes:\n"
		"                     \"disabled\" - prints the output as fast as possible.\n"
		"                     \"draw-all\" - synchronization enabled, but tries to draw all frames.\n"
		"                     \"enabled\" - synchronization enabled, skips frames if necessary [default].\n"
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
		" -da(--disable-audio)Disables audio.\n"
		" -dk (--disable-keys)Disables keyboard control.\n"
		" -avl (--libav-logs) Enables printing Libav logs. Helpful with FFmpeg filters problems.\n"
		" -fi (--full-info)   Full info about ConPlayer.\n");
}

static void helpColorModes(void)
{
	puts(
		"Color modes:\n"
		" >winapi-gray [Windows-only]\n"
		" >winapi-16 [Windows-only]\n"
		" >cstd-gray\n"
		" >cstd-16\n"
		" >cstd-256 [default]\n"
		" >cstd-rgb\n");
}

static void helpScalingModes(void)
{
	puts(
		"Scaling modes:\n"
		" >nearest\n"
		" >fast-bilinear\n"
		" >bilinear\n"
		" >bicubic [default]\n");
}

static void helpKeyboard(void)
{
	puts(
		"Keyboard control:\n"
		" Space      Pause/Play\n"
		" \"[\" / \"]\"  Go back / Skip forward (10 second)\n"
		" \"-\" / \"+\"  Go back / Skip forward (30 second)\n"
		" \"L\" / \"O\"  Turn down/up the volume\n"
		" \"M\"        Mute the audio\n"
		" ESC        Exit\n");
}