#include "conplayer.h"

static void helpBasicOptions(void);
static void helpAdvancedOptions(void);
static void helpColorModes(void);
static void helpKeyboard(void);

void showHelp(int basic, int advanced, int colorModes, int keyboard)
{
	puts("ConPlayer - Help\n");
	if (basic) { helpBasicOptions(); }
	if (advanced) { helpAdvancedOptions(); }
	if (colorModes) { helpColorModes(); }
	if (keyboard) { helpKeyboard(); }
}

void showInfo(void)
{
	puts(
		"ConPlayer - Information\n"
		"ConPlayer " CP_VERSION " [" CP_CPU "]\n"
		"Author: https://github.com/mt1006\n"
		"Version: " CP_VERSION "\n"
		"Architecture: " CP_CPU);
}

void showFullInfo(void)
{
	puts(
		"ConPlayer - Full info\n"
		"ConPlayer " CP_VERSION " [" CP_CPU "]\n"
		"Author: https://github.com/mt1006\n"
		"Version: " CP_VERSION "\n"
		"Architecture: " CP_CPU);
	#if defined(_MSC_VER)
	puts("Compiler: MSC\n"
		"Compiler version: " DEF_TO_STR(_MSC_VER) " [" DEF_TO_STR(_MSC_FULL_VER) "]");
	#elif defined(__GNUC__)
	puts("Compiler: GCC\n"
		"Compiler version: " __VERSION__);
	#else
	puts("Compiler: [unknown]");
	#endif
}

void showVersion(void)
{
	puts("ConPlayer " CP_VERSION " [" CP_CPU "]");
}

static void helpBasicOptions(void)
{
	puts(
		"Basic options:\n"
		" [none] / -i         Just input file - audio or video.\n"
		"                     Examples:\n"
		"                      conpl video.mp4\n"
		" -c <mode>           Sets color mode.\n"
		"  (--colors)         Default color mode is \"winapi-gray\".\n"
		"                     With \"-c\", with no mode specified, default is \"cstd-256\".\n"
		"                     To get list of all avaible color modes use \"conpl -h color-modes\"\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -c\n"
		"                      conpl video.mp4 -c winapi-16\n"
		" -vol [volume]       Sets audio volume. By default \"0.5\".\n"
		"  (--volume)         Examples:\n"
		"                      conpl video.mp4 -vol 0.2\n"
		" -s [w] [h]          Sets width and height of the drawn image.\n"
		"  (--size)           By default size of entire window.\n"
		"                     Using \"-s 0 0\" image size will be constant\n"
		"                     (will not change with the console size change).\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -s 120 50\n"
		" -f  (--fill)        Fills entire available area, without keeping ratio.\n"
		" -inf(--information) Information about ConPlayer.\n"
		" -v  (--version)     Information about ConPlayer version.\n"
		" -h <topic>          Displays help message.\n"
		"  (--help)           Topics: basic, advanced, color-modes, keyboard, full\n");
}

static void helpAdvancedOptions(void)
{
	puts(
		"Advanced options:\n"
		" -int [divisor]      Uses interlacing to draw frames.\n"
		"      <height>       The larger the divisor, fewer scanlines there are per frame.\n"
		"  (--interlaced)     Height is optional parameter and means scanline height - by default 1.\n"
		"                     When divisor is equal to 1, then interlacing is disabled.\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -int 2\n"
		"                      conpl video.mp4 -int 4 3\n"
		" -fi (--full-info)   Full info about ConPlayer.\n");
}

static void helpColorModes(void)
{
	puts(
		"Color modes:\n"
		" >winapi-gray (default without \"-c\")\n"
		" >winapi-16\n"
		" >cstd-gray\n"
		" >cstd-16\n"
		" >cstd-256 (defaults to \"-c\")\n"
		" >cstd-rgb\n");
}

static void helpKeyboard(void)
{
	puts(
		"Keyboard control:\n"
		" Space - Pause/Play\n"
		" \"[\" / \"]\" - Go back/Skip forward\n"
		" ESC - Exit\n");
}