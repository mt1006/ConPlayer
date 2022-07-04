#include "conplayer.h"

const int QUEUE_SIZE = 64;

int w = -1, h = -1;
int conW = -1, conH = -1;
int vidW = -1, vidH = -1;
int argW = -1, argH = -1;
int fillArea = 0;
ColorMode colorMode = CP_DEFAULT_COLOR_MODE;
int scanlineCount = 1, scanlineHeight = 1;
double volume = 0.5;
double fps;
int decodeEnd = 0;
char* charset = NULL;
int charsetSize = 0;
double constFontRatio = 0.0;
int disableKeyboard = 0, disableSync = 0, disableCLS = 0;
int ansiEnabled = 0;
SetColorMode setColorMode = SCM_DISABLED;
int setColorVal = 0;

void load(char* inputFile)
{
	Stream* audioStream;

	puts("Loading...");

	initAV(inputFile, &audioStream);
	initDrawFrame();
	initProcessFrame();
	initQueue();
	initAudio(audioStream);

	beginThreads();
	readFrames();
}

int main(int argc, char** argv)
{
	#ifndef _DEBUG
	av_log_set_level(AV_LOG_QUIET);
	#endif

	int uc_argc;
	unichar** uc_argv;
	if (USE_WCHAR)
	{
		#ifdef _WIN32
		uc_argv = CommandLineToArgvW(GetCommandLineW(), &uc_argc);
		#else
		error("USE_WCHAR defined but it isn't Windows!", "main.c", __LINE__);
		#endif
	}
	else
	{
		uc_argc = argc;
		uc_argv = (unichar**)argv;
	}

	char* inputFile = argumentParser(uc_argc - 1, uc_argv + 1);
	if (inputFile) { load(inputFile); }

	cpExit(0);
	return 0;
}