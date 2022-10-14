#include "conplayer.h"

#define CP_MAX_OLD_TITLE_LEN 1024
#define CP_MAX_NEW_TITLE_LEN 128
#define CP_WAIT_FOR_SET_TITLE 100

//drawFrame.c
extern HANDLE outputHandle;

double getTime(void)
{
	#ifdef _WIN32

	return (double)clock() / (double)CLOCKS_PER_SEC;

	#else

	struct timespec timeSpec;
	clock_gettime(CLOCK_REALTIME, &timeSpec);
	return (double)((timeSpec.tv_nsec / 1000000) + (timeSpec.tv_sec * 1000)) / 1000.0;

	#endif
}

ThreadIDType startThread(ThreadFuncPtr threadFunc, void* args)
{
	#ifdef _WIN32

	return _beginthread(threadFunc, 0, args);

	#else

	ThreadIDType retVal;
	pthread_create(&retVal, NULL, threadFunc, args);
	return retVal;

	#endif
}

void strToLower(char* str)
{
	for (int i = 0; i < strlen(str); i++)
	{
		str[i] = (char)tolower((int)str[i]);
	}
}

void getConsoleWindow(void)
{
	#ifdef _WIN32
	if (conHWND) { return; }

	RECT clientRect;
	conHWND = GetConsoleWindow();
	GetClientRect(conHWND, &clientRect);

	if (clientRect.right == 0 || clientRect.bottom == 0)
	{
		wchar_t oldConTitle[CP_MAX_OLD_TITLE_LEN];
		char newConTitle[CP_MAX_NEW_TITLE_LEN];

		GetConsoleTitleW(oldConTitle, CP_MAX_OLD_TITLE_LEN);
		sprintf(newConTitle, "ConPlayer-(%d/%d)",
			(int)clock(), (int)GetCurrentProcessId());
		SetConsoleTitleA(newConTitle);
		Sleep(CP_WAIT_FOR_SET_TITLE);
		HWND newConHWND = FindWindowA(NULL, newConTitle);
		SetConsoleTitleW(oldConTitle);

		if (newConHWND)
		{
			conHWND = newConHWND;
			wtDragBarHWND = FindWindowExW(conHWND, NULL,
				L"DRAG_BAR_WINDOW_CLASS", NULL);
		}
	}
	#endif
}

void clearScreen(void)
{
	#ifdef _WIN32

	// https://docs.microsoft.com/en-us/windows/console/clearing-the-screen
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	SMALL_RECT scrollRect;
	COORD scrollTarget;
	CHAR_INFO fill;

	GetConsoleScreenBufferInfo(outputHandle, &csbi);

	scrollRect.Left = 0;
	scrollRect.Top = 0;
	scrollRect.Right = csbi.dwSize.X;
	scrollRect.Bottom = csbi.dwSize.Y;
	
	scrollTarget.X = 0;
	scrollTarget.Y = (SHORT)(0 - csbi.dwSize.Y);

	fill.Char.UnicodeChar = TEXT(' ');
	fill.Attributes = csbi.wAttributes;

	ScrollConsoleScreenBufferA(outputHandle, &scrollRect, NULL, scrollTarget, &fill);

	csbi.dwCursorPosition.X = 0;
	csbi.dwCursorPosition.Y = 0;

	SetConsoleCursorPosition(outputHandle, csbi.dwCursorPosition);

	#else

	fputs("\x1B[H\x1B[J", stdout);

	#endif
}

void setDefaultColor(void)
{
	#ifdef _WIN32
	SetConsoleTextAttribute(outputHandle,
		FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	#else
	puts("\x1B[39");
	#endif
}

void setCursorPos(int x, int y)
{
	#ifdef _WIN32

	COORD cursor = { (SHORT)x,(SHORT)y };
	if (!settings.disableCLS) { SetConsoleCursorPosition(outputHandle, cursor); }

	#else

	if (x == 0 && y == 0) { fputs("\x1B[H", stdout); }
	else { printf("\x1B[%d;%dH", x, y); }

	#endif
}

size_t getOutputArraySize(int frameW, int frameH)
{
	const int CSTD_16_CODE_LEN = 6;   // "\x1B[??m?"
	const int CSTD_256_CODE_LEN = 12; // "\x1B[38;5;???m?"
	const int CSTD_RGB_CODE_LEN = 20; // "\x1B[38;2;???;???;???m?"

	switch (settings.colorMode)
	{
	case CM_CSTD_GRAY:
		return (frameW + 1) * frameH * sizeof(char);
	case CM_CSTD_16:
		return ((frameW * frameH * CSTD_16_CODE_LEN) + frameH) * sizeof(char);
	case CM_CSTD_256:
		return ((frameW * frameH * CSTD_256_CODE_LEN) + frameH) * sizeof(char);
	case CM_CSTD_RGB:
		return ((frameW * frameH * CSTD_RGB_CODE_LEN) + frameH) * sizeof(char);
	case CM_WINAPI_GRAY:
	case CM_WINAPI_16:
		return frameW * frameH * sizeof(CHAR_INFO);
	}
}

void cpExit(int code)
{
	#ifndef _WIN32
	setTermios(true);
	fputs("\n", stdout);
	#endif

	setDefaultColor();
	exit((int)code);
}

void error(const char* description, const char* fileName, int line)
{
	puts("\nSomething went wrong...");
	printf("%s [%s:%d]\n", description, fileName, line);
	cpExit(-1);
}

#ifdef _WIN32

int getWindowsArgv(char*** pargv)
{
	int argc;
	wchar_t** argvw = CommandLineToArgvW(GetCommandLineW(), &argc);

	char** argv = (char**)malloc(argc * sizeof(char*));

	for (int i = 0; i < argc; i++)
	{
		int utf8Len = WideCharToMultiByte(CP_UTF8, 0, argvw[i], -1, NULL, 0, NULL, NULL);
		argv[i] = (char*)malloc(utf8Len * sizeof(char));
		WideCharToMultiByte(CP_UTF8, 0, argvw[i], -1, argv[i], utf8Len, NULL, NULL);
	}

	*pargv = argv;
	LocalFree(argvw);
	return argc;
}

#else

//https://stackoverflow.com/a/7469410/18214530
void setTermios(bool deinit)
{
	static struct termios current, old;
	static int termiosInitialized = 0;

	if (deinit)
	{
		if (!termiosInitialized) { return; }
		tcsetattr(0, TCSANOW, &old);
	}
	else
	{
		if (termiosInitialized) { return; }

		tcgetattr(0, &old);
		current = old;
		current.c_lflag &= ~ICANON;
		//current.c_lflag &= ~ECHO;
		tcsetattr(0, TCSANOW, &current);

		termiosInitialized = 1;
	}
}

int _getch(void)
{
	static bool firstCall = true;
	
	if (firstCall)
	{
		setTermios(false);
		firstCall = false;
	}

	return getchar();
}

void Sleep(DWORD ms)
{
	if (ms == 0)
	{
		sched_yield();
	}
	else
	{
		struct timespec timeSpec;
		timeSpec.tv_sec = ms / 1000;
		timeSpec.tv_nsec = (ms % 1000) * 1000000;
		nanosleep(&timeSpec, NULL);
	}
}
#endif