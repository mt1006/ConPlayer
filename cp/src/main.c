#include "conplayer.h"

const int QUEUE_SIZE = 64;

HWND conHWND = NULL, wtDragBarHWND = NULL;
int w = -1, h = -1;
int conW = -1, conH = -1;
int vidW = -1, vidH = -1;
double fps = 0.0;
bool ansiEnabled = false;
bool decodeEnd = false;

Settings settings =
{
	.argW = -1, .argH = -1,
	.fillArea = false,
	.colorMode = CM_CSTD_256,
	.scanlineCount = 1, .scanlineHeight = 1,
	.volume = 0.5,
	.charset = NULL,
	.charsetSize = 0,
	.setColorMode = SCM_DISABLED,
	.setColorVal1 = 0, .setColorVal2 = 0,
	.constFontRatio = 0.0,
	.brightnessRand = 0,
	.scalingMode = SWS_BICUBIC,
	.syncMode = SM_ENABLED,
	.videoFilters = NULL,
	.scaledVideoFilters = NULL,
	.audioFilters = NULL,
	.disableKeyboard = false,
	.disableCLS = false,
	.disableAudio = false,
	.singleCharMode = false,
	.libavLogs = false
};

void load(char* inputFile)
{
	if (settings.libavLogs) { av_log_set_level(AV_LOG_VERBOSE); }
	else { av_log_set_level(AV_LOG_QUIET); }

	Stream* audioStream;

	puts("Loading...");

	initDecodeFrame(inputFile, &audioStream);
	initDrawFrame();
	initQueue();
	if (!settings.disableAudio) { initAudio(audioStream); }

	beginThreads();
	readFrames();
}

int main(int argc, char** argv)
{
	#ifdef _WIN32
	argc = getWindowsArgv(&argv);
	#endif

	if (argc < 2)
	{
		showNoArgsInfo();
		return -1;
	}

	char* inputFile = argumentParser(argc - 1, argv + 1);
	if (inputFile) { load(inputFile); }

	cpExit(0);
	return 0;
}