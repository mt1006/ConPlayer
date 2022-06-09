#include "conplayer.h"

const int QUEUE_SIZE = 64;
const int ANSI_CODE_LEN = 12;

int w = -1, h = -1;
int conW = -1, conH = -1;
int fontW = -1, fontH = -1;
int vidW = -1, vidH = -1;
int argW = -1, argH = -1;
int fillArea = 0, useCStdOut = 0, withColors = 0;
int interlacing = 1;
double volume = 0.5;
double fps;
int decodeEnd = 0;

char* inputFile = NULL;
int usedInputOption = 0;
int volumeSet = 0;
int interlacingSet = 0;

Option getOption(char* str, int strLen)
{
	if (strLen > 1 && str[0] == '-')
	{
		if (!strcmp(str, "-i") || !strcmp(str, "--input")) { return OP_INPUT; }
		else if (!strcmp(str, "-s") || !strcmp(str, "--size")) { return OP_SIZE; }
		else if (!strcmp(str, "-vol") || !strcmp(str, "--volume")) { return OP_VOLUME; }
		else if (!strcmp(str, "-f") || !strcmp(str, "--fill")) { return OP_FILL; }
		else if (!strcmp(str, "-int") || !strcmp(str, "--interlaced")) { return OP_INTERLACING; }
		else if (!strcmp(str, "-cstd") || !strcmp(str, "--c-std-out")) { return OP_USE_CSTD_OUT; }
		else if (!strcmp(str, "-c") || !strcmp(str, "--colors")) { return OP_WITH_COLORS; }
		else if (!strcmp(str, "-inf") || !strcmp(str, "--information")) { return OP_INFORMATION; }
		else if (!strcmp(str, "-fi") || !strcmp(str, "--full-info")) { return OP_FULL_INFO; }
		else if (!strcmp(str, "-v") || !strcmp(str, "--version")) { return OP_GET_VERSION; }
		else if (!strcmp(str, "-h") || !strcmp(str, "-?") || !strcmp(str, "--help")) { return OP_HELP; }
		else { error("Invalid syntax!", "main.c", __LINE__); }
	}
	else if (!strLen) { error("Invalid syntax!", "main.c", __LINE__); }

	return OP_NONE;
}

Option argumentParser(int argc, unichar** argv)
{
	Option operation = OP_NONE;
	Option expectedInput = OP_INPUT;

	if (argc < 2) { error("Invalid syntax!", "main.c", __LINE__); }
	for (int i = 1; i < argc; i++)
	{
		int argLen = (int)uc_strlen(argv[i]);
		char* arg = toUTF8(argv[i], argLen);
		Option option = getOption(arg, argLen);
		if (option == OP_NONE)
		{
			if (expectedInput == OP_INPUT && !inputFile)
			{
				inputFile = toUTF8(argv[i], argLen);
				expectedInput = OP_NONE;
			}
			else if (expectedInput == OP_SIZE && h == -1)
			{
				if (argW == -1)
				{
					argW = atoi(arg);
					if (argW < 4 && argW != 0) { error("Invalid size!", "main.c", __LINE__); }
				}
				else
				{
					argH = atoi(arg);
					if (argH < 4 && argW != 0) { error("Invalid size!", "main.c", __LINE__); }
					expectedInput = OP_NONE;
				}
			}
			else if (expectedInput == OP_VOLUME)
			{
				volume = atof(arg);
				if (volume < 0.0) { error("Invalid volume!", "main.c", __LINE__); }
				expectedInput = OP_NONE;
			}
			else if (expectedInput == OP_INTERLACING)
			{
				interlacing = atoi(arg);
				if (interlacing < 1) { error("Invalid interlacing!", "main.c", __LINE__); }
				expectedInput = OP_NONE;
			}
			else
			{
				error("Invalid syntax!", "main.c", __LINE__);
			}
		}
		else
		{
			if (expectedInput != OP_NONE && (expectedInput != OP_INPUT || usedInputOption))
			{
				error("Invalid syntax!", "main.c", __LINE__);
			}

			if (option == OP_HELP || option == OP_INFORMATION ||
				option == OP_FULL_INFO || option == OP_GET_VERSION)
			{
				if (argc != 2) { error("Invalid syntax!", "main.c", __LINE__); }
				operation = option;
			}
			else if (option == OP_FILL)
			{
				fillArea = 1;
			}
			else if (option == OP_USE_CSTD_OUT)
			{
				useCStdOut = 1;
			}
			else if (option == OP_WITH_COLORS)
			{
				withColors = 1;
			}
			else if (option == OP_SIZE)
			{
				expectedInput = OP_SIZE;
			}
			else if (option == OP_VOLUME && !volumeSet)
			{
				expectedInput = OP_VOLUME;
				volumeSet = 1;
			}
			else if (option == OP_INTERLACING && !interlacingSet)
			{
				expectedInput = OP_INTERLACING;
				interlacingSet = 1;
			}
			else if (option == OP_INPUT && !usedInputOption)
			{
				if (inputFile) { error("Invalid syntax!", "main.c", __LINE__); }
				expectedInput = OP_INPUT;
				usedInputOption = 1;
			}
			else
			{
				error("Invalid syntax!", "main.c", __LINE__);
			}
		}
		if (USE_WCHAR) { free(arg); }
	}
	if (expectedInput != OP_NONE && expectedInput != OP_INPUT) { error("Invalid syntax!", "main.c", __LINE__); }
	if (operation == OP_NONE) { operation = OP_INPUT; }

	return operation;
}

void load()
{
	Stream* audioStream;
	if (!inputFile) { error("Invalid syntax!", "main.c", __LINE__); };

	puts("Loading...");

	initAV(inputFile, &audioStream);
	initConsole();
	initQueue();
	initAudio(audioStream);

	beginThreads();
	readFrames();
}

void executeOperation(Option operation)
{
	switch (operation)
	{
	case OP_INPUT:
		load();
		break;
	case OP_INFORMATION:
		showInformations();
		break;
	case OP_FULL_INFO:
		showFullInfo();
		break;
	case OP_GET_VERSION:
		showVersion();
		break;
	case OP_HELP:
		showHelp();
		break;
	default:
		error("Invalid syntax!", "main.c", __LINE__);
	}
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
		programError("main.c", __LINE__);
		#endif
	}
	else
	{
		uc_argc = argc;
		uc_argv = (unichar**)argv;
	}

	Option operation = argumentParser(uc_argc, uc_argv);
	executeOperation(operation);
	return 0;
}