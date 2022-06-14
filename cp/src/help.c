#include "conplayer.h"

void showHelp(void)
{
	puts(
		"ConPlayer - Help\n\n"
		"List of options:\n"
		" [none] / -i         Just input file - audio or video.\n"
		"                     Examples:\n"
		"                      conpl video.mp4\n"
		" -s [w] [h]          Sets width and height of the drawn image.\n"
		"  (--size)           By default size of entire window.\n"
		"                     Using \"-s 0 0\" image size will be constant\n"
		"                     (will not change with the console size change).\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -s 120 50\n"
		" -vol [volume]       Sets audio volume. By default \"0.5\".\n"
		"  (--volume)         Examples:\n"
		"                      conpl video.mp4 -v 0.2\n"
		" -c (--colors)       Colors (by default - 16, with \"-cstd\" - 256).\n"
		"                     On older Windows versions colors with \"-cstd\" may not work properly!\n"
		" -f  (--fill)        Fills entire available area, without keeping ratio.\n"
		" -int [divisor]      Uses interlacing to draw frames (works only with \"-cstd\").\n"
		"  (--interlaced)     The larger the divisor, fewer scanlines there are per frame.\n"
		"                     When divisor is equal to 1, then interlacing is disabled.\n"
		"                     Examples:\n"
		"                      conpl video.mp4 -int 2\n"
		" -cstd (--c-std-out) Uses C std functions instead of WinAPI.\n"
		" -inf(--information) Information about ConPlayer.\n"
		" -fi (--full-info)   Full info about ConPlayer.\n"
		" -v  (--version)     Information about ConPlayer version.\n"
		" -h / -? (--help)    Displays this help message.\n\n"
		"Keyboard control:\n"
		" Space - Pause/Play\n"
		" \"[\" / \"]\" - Go back/Skip forward\n"
		" ESC - Exit");
}

void showInformations(void)
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