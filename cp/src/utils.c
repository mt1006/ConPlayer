#include "conplayer.h"

#define CP_MAX_OLD_TITLE_LEN 1024
#define CP_MAX_NEW_TITLE_LEN 128
#define CP_WAIT_FOR_SET_TITLE 100

#ifndef _WIN32
static void setTermios(bool deinit);
#endif

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

void getConsoleSize(int* w, int* h)
{
	#ifdef _WIN32

	CONSOLE_SCREEN_BUFFER_INFO consoleBufferInfo;
	GetConsoleScreenBufferInfo(outputHandle, &consoleBufferInfo);

	*w = consoleBufferInfo.srWindow.Right - consoleBufferInfo.srWindow.Left + 1;
	*h = consoleBufferInfo.srWindow.Bottom - consoleBufferInfo.srWindow.Top + 1;

	#else

	struct winsize winSize;
	ioctl(0, TIOCGWINSZ, &winSize);
	*w = winSize.ws_col;
	*h = winSize.ws_row;

	#endif
}

void enableANSI(void)
{
	#ifdef _WIN32

	DWORD mode;
	GetConsoleMode(outputHandle, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(outputHandle, mode);
	ansiEnabled = true;

	#endif
}

size_t getOutputArraySize(int w, int h)
{
	const int CSTD_16_CODE_LEN = 6;   // "\x1B[??m?"
	const int CSTD_256_CODE_LEN = 12; // "\x1B[38;5;???m?"
	const int CSTD_RGB_CODE_LEN = 20; // "\x1B[38;2;???;???;???m?"

	#ifndef CP_DISABLE_OPENGL
	if (settings.useFakeConsole)
	{
		return w * h * sizeof(GlConsoleChar);
	}
	#endif

	switch (settings.colorMode)
	{
	case CM_CSTD_GRAY:
		return (w + 1) * h * sizeof(char);
	case CM_CSTD_16:
		return ((w * h * CSTD_16_CODE_LEN) + h) * sizeof(char);
	case CM_CSTD_256:
		return ((w * h * CSTD_256_CODE_LEN) + h) * sizeof(char);
	case CM_CSTD_RGB:
		return ((w * h * CSTD_RGB_CODE_LEN) + h) * sizeof(char);
	case CM_WINAPI_GRAY:
	case CM_WINAPI_16:
		return w * h * sizeof(CHAR_INFO);
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

static int parseWideStringAsArgv(LPCWSTR str, char*** pargv);

int getWindowsArgv(char*** pargv)
{
	return parseWideStringAsArgv(GetCommandLineW(), pargv);
}

int parseStringAsArgv(char* str, char*** pargv)
{
	int wideLen = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	LPCWSTR wstr = (char*)malloc(wideLen * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, wideLen);
	return parseWideStringAsArgv(wstr, pargv);
}

static int parseWideStringAsArgv(LPCWSTR str, char*** pargv)
{
	int argc;
	wchar_t** argvw = CommandLineToArgvW(str, &argc);
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

int getChar(bool wasdAsArrows)
{
	int ch = _getch();

	if (wasdAsArrows)
	{
		switch (ch)
		{
		case 'w': return VK_UP;
		case 'a': return VK_LEFT;
		case 's': return VK_DOWN;
		case 'd': return VK_RIGHT;
		case 'q': return VK_PRIOR;
		case 'e': return VK_NEXT;
		case 'r': return VK_HOME;
		case 'f': return VK_END;
		}
	}
	
	if (ch == 0 || ch == 0xE0)
	{
		//https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-6.0/aa299374%28v=vs.60%29
		switch (_getch())
		{
		case 72: return VK_UP;
		case 75: return VK_LEFT;
		case 77: return VK_RIGHT;
		case 80: return VK_DOWN;
		case 73: return VK_PRIOR;
		case 81: return VK_NEXT;
		case 71: return VK_HOME;
		case 79: return VK_END;
		}
		return 0;
	}
	return ch;
}

#else

//https://stackoverflow.com/a/7469410/18214530
static void setTermios(bool deinit)
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
		tcsetattr(0, TCSANOW, &current);

		termiosInitialized = 1;
	}
}

int getChar(bool wasdAsArrows)
{
	static bool firstCall = true;

	if (firstCall)
	{
		setTermios(false);
		firstCall = false;
	}

	int ch = getchar();

	if (wasdAsArrows)
	{
		switch (ch)
		{
		case 'w': return VK_UP;
		case 'a': return VK_LEFT;
		case 's': return VK_DOWN;
		case 'd': return VK_RIGHT;
		case 'q': return VK_PRIOR;
		case 'e': return VK_NEXT;
		case 'r': return VK_HOME;
		case 'f': return VK_END;
		}
	}

	//https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h3-PC-Style-Function-Keys
	if (ch == 0x1B)
	{
		if (getchar() == '[')
		{
			switch (getchar())
			{
			case 'A': return VK_UP;
			case 'B': return VK_DOWN;
			case 'C': return VK_RIGHT;
			case 'D': return VK_LEFT;
			case '5': if (getchar() == '~') { return VK_PRIOR; }
			case '6': if (getchar() == '~') { return VK_NEXT; }
			case 'H': return VK_HOME;
			case 'F': return VK_END;
			case 'O':
				int chSS3 = getchar();
				if (chSS3 == 'H') { return VK_HOME; }
				if (chSS3 == 'F') { return VK_END; }
				if (chSS3 == 'M') { return VK_RETURN; }
			}
		}
		return 0;
	}
	else if (ch == '\n')
	{
		return VK_RETURN;
	}
	return ch;
}

FILE* _popen(const char* command, const char* type)
{
	return popen(command, type);
}

int _pclose(FILE* stream)
{
	return pclose(stream);
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