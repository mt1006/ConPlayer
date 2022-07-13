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
	if (ansiEnabled)
	{
		fputs("\x1B[0m", stdout);
	}
	else if (setColorMode == SCM_WINAPI)
	{
		#ifdef _WIN32
		SetConsoleTextAttribute(outputHandle,
			FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		#endif
	}
}

void setCursorPos(int x, int y)
{
	#ifdef _WIN32

	COORD cursor = { (SHORT)x,(SHORT)y };
	if (!disableCLS) { SetConsoleCursorPosition(outputHandle, cursor); }

	#else

	if (x == 0 && y == 0) { fputs("\x1B[H", stdout); }
	else { printf("\x1B[%d;%dH", x, y); }

	#endif
}

size_t getOutputArraySize(void)
{
	const int CSTD_16_CODE_LEN = 6;   // "\x1B[??m?"
	const int CSTD_256_CODE_LEN = 12; // "\x1B[38;5;???m?"
	const int CSTD_RGB_CODE_LEN = 20; // "\x1B[38;2;???;???;???m?"

	switch (colorMode)
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

uint8_t rgbToAnsi256(uint8_t r, uint8_t g, uint8_t b)
{
	// https://stackoverflow.com/questions/15682537/ansi-color-specific-rgb-sequence-bash
	if (r == g && g == b)
	{
		if (r < 8) { return 16; }
		if (r > 248) { return 231; }
		return (uint8_t)round((((double)r - 8.0) / 247.0) * 24.0) + 232;
	}

	return (uint8_t)(16.0
		+ (36.0 * round((double)r / 255.0 * 5.0))
		+ (6.0 * round((double)g / 255.0 * 5.0))
		+ round((double)b / 255.0 * 5.0));
}

int utf8ArraySize(unichar* input, int inputSize)
{
	if (USE_WCHAR)
	{
		int output = 1;
		for (int i = 0; i < inputSize; i++)
		{
			unsigned short ch = (unsigned short)input[i];
			if (ch < 0x80) { output++; }
			else if (ch < 0x800) { output += 2; }
			else { output += 3; }
		}
		return output;
	}
	else
	{
		return inputSize + 1;
	}
}

void unicharArrayToUTF8(unichar* input, char* output, int inputSize)
{
	if (USE_WCHAR)
	{
		int p = 0;
		for (int i = 0; i < inputSize; i++)
		{
			unsigned short ch = (unsigned short)input[i];
			if (ch < 0x80)
			{
				output[p] = (char)ch;
				p++;
			}
			else if (ch < 0x800)
			{
				output[p] = (ch >> 6) | 0xC0;
				output[p + 1] = (ch & 0x3F) | 0x80;
				p += 2;
			}
			else
			{
				output[p] = (ch >> 12) | 0xE0;
				output[p + 1] = ((ch >> 6) & 0x3F) | 0x80;
				output[p + 2] = (ch & 0x3F) | 0x80;
				p += 3;
			}
		}
		output[p] = '\0';
	}
	else
	{
		memcpy(output, input, inputSize + 1);
	}
}

char* toUTF8(unichar* input, int inputLen)
{
	if (USE_WCHAR)
	{
		int outputStrSize = utf8ArraySize(input, inputLen);
		char* output = (char*)malloc(outputStrSize * sizeof(char));
		unicharArrayToUTF8(input, output, inputLen);
		return output;
	}
	else
	{
		char* output = (char*)malloc((inputLen + 1) * sizeof(char));
		memcpy(output, input, (inputLen + 1));
		return output;
	}
}

void cpExit(int code)
{
	#ifndef _WIN32
	setTermios(1);
	fputs("\n", stdout);
	#endif

	setDefaultColor();
	exit(code);
}

void error(const char* description, const char* fileName, int line)
{
	puts("\nSomething went wrong...");
	printf("%s [%s:%d]\n", description, fileName, line);
	cpExit(-1);
}

#ifndef _WIN32

//https://stackoverflow.com/a/7469410/18214530
void setTermios(int deinit)
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
	static int firstCall = 1;
	
	if (firstCall)
	{
		setTermios(0);
		firstCall = 0;
	}

	return getchar();
}

void Sleep(DWORD ms)
{
	struct timespec timeSpec;
	timeSpec.tv_sec = ms / 1000;
	timeSpec.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&timeSpec, NULL);
}

#endif