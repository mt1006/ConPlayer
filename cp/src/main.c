#include "conplayer.h"

const int QUEUE_SIZE = 64;

int w = -1, h = -1;
int conW = -1, conH = -1;
int fontW = -1, fontH = -1;
int vidW = -1, vidH = -1;
int argW = -1, argH = -1;
int fillArea = 0;
ColorMode colorMode = CM_WINAPI_GRAY;
int interlacing = 1;
double volume = 0.5;
double fps;
int decodeEnd = 0;

void load(char* inputFile)
{
	Stream* audioStream;
	if (!inputFile) { error("Invalid syntax!", "main.c", __LINE__); };

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

	load(argumentParser(uc_argc - 1, uc_argv + 1));
	return 0;
}