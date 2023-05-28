#include "conplayer.h"

typedef struct 
{
	int conW;
	int conH;
	double fontRatio;
} ConsoleInfo;

HANDLE outputHandle = NULL;

static void drawWithWinAPI(CHAR_INFO* output, int fw, int fh);
static void getConsoleInfo(ConsoleInfo* consoleInfo);
static void setConstColor(void);

void initDrawFrame(void)
{
	if (vidW == -1 || vidH == -1) { return; }

	#ifdef _WIN32
	getConsoleWindow();
	outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	if (settings.colorMode == CM_CSTD_16 ||
		settings.colorMode == CM_CSTD_256 ||
		settings.colorMode == CM_CSTD_RGB ||
		settings.setColorMode == SCM_CSTD_256 ||
		settings.setColorMode == SCM_CSTD_RGB)
	{
		/*DWORD mode;
		GetConsoleMode(outputHandle, &mode);
		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(outputHandle, mode);
		ansiEnabled = true;*/

		puts("Virtual terminal processing not supported on this build!");
		puts("Color mode will be changed to \"winapi-16\".");
		puts("Press any key to continue...");
		getchar();

		settings.colorMode = CM_WINAPI_16;
	}
	#endif

	refreshSize();
}

void refreshSize(void)
{
	static int firstCall = 1;
	static int setNewSize = 0;
	static int lastW = 0, lastH = 0;
	static double fontRatio = 0.0;

	ConsoleInfo consoleInfo;
	getConsoleInfo(&consoleInfo);

	int newW = lastW, newH = lastH;

	if (settings.argW != -1 && settings.argH != -1)
	{
		if (settings.argW == 0 && settings.argH == 0)
		{
			settings.argW = consoleInfo.conW;
			settings.argH = consoleInfo.conH;
		}

		if (settings.fillArea)
		{
			if (firstCall)
			{
				newW = settings.argW;
				newH = settings.argH;
			}
		}
		else if (fontRatio != consoleInfo.fontRatio)
		{
			if (settings.constFontRatio == 0.0) { fontRatio = consoleInfo.fontRatio; }
			else { fontRatio = settings.constFontRatio; }

			double vidRatio = (double)vidW / (double)vidH;
			double conRatio = ((double)settings.argW / (double)settings.argH) * fontRatio;

			if (vidRatio > conRatio)
			{
				newW = settings.argW;
				newH = (int)((conRatio / vidRatio) * settings.argH);
			}
			else
			{
				newW = (int)((vidRatio / conRatio) * settings.argW);
				newH = settings.argH;
			}
		}
	}
	else
	{
		if (settings.fillArea)
		{
			if (conW != consoleInfo.conW || conH != consoleInfo.conH)
			{
				conW = consoleInfo.conW;
				conH = consoleInfo.conH;
				newW = consoleInfo.conW;
				newH = consoleInfo.conH;
			}
		}
		else if (conW != consoleInfo.conW || conH != consoleInfo.conH ||
			fontRatio != consoleInfo.fontRatio)
		{
			conW = consoleInfo.conW;
			conH = consoleInfo.conH;
			
			if (settings.constFontRatio == 0.0) { fontRatio = consoleInfo.fontRatio; }
			else { fontRatio = settings.constFontRatio; }

			double vidRatio = (double)vidW / (double)vidH;
			double conRatio = ((double)conW / (double)conH) * fontRatio;
			if (vidRatio > conRatio)
			{
				newW = conW;
				newH = (int)((conRatio / vidRatio) * conH);
			}
			else
			{
				newW = (int)((vidRatio / conRatio) * conW);
				newH = conH;
			}
		}
	}

	if (firstCall)
	{
		w = newW;
		h = newH;
		lastW = w;
		lastH = h;
		firstCall = 0;
	}
	else
	{
		if (newW != lastW || newH != lastH)
		{
			lastW = newW;
			lastH = newH;
			setNewSize = 1;
		}
		else if (setNewSize)
		{
			w = newW;
			h = newH;
			setNewSize = 0;
		}
	}
}

void drawFrame(void* output, int* lineOffsets, int fw, int fh)
{
	static int scanline = 0;
	static int lastFW = 0, lastFH = 0;
	if ((lastFW != fw || lastFH != fh) && !settings.disableCLS)
	{
		lastFW = fw;
		lastFH = fh;
		clearScreen();
	}

	if (settings.colorMode == CM_WINAPI_GRAY || settings.colorMode == CM_WINAPI_16)
	{
		drawWithWinAPI((CHAR_INFO*)output, fw, fh);
		return;
	}

	setConstColor();

	if (settings.scanlineCount == 1)
	{
		if (!settings.disableCLS) { setCursorPos(0, 0); }
		fwrite((char*)output, 1, lineOffsets[fh], stdout);
	}
	else
	{
		for (int i = 0; i < fh; i += settings.scanlineCount * settings.scanlineHeight)
		{
			int sy = i + (scanline * settings.scanlineHeight);
			int sh = settings.scanlineHeight;
			if (sy >= fh) { break; }
			else if (sy + sh > fh) { sh = fh - sy; }

			if (!settings.disableCLS) { setCursorPos(0, sy); }
			fwrite((char*)output + lineOffsets[sy], 1, lineOffsets[sy + sh] - lineOffsets[sy], stdout);
		}

		scanline++;
		if (scanline == settings.scanlineCount) { scanline = 0; }
	}
}

static void drawWithWinAPI(CHAR_INFO* output, int fw, int fh)
{
	#ifdef _WIN32
	static int scanline = 0;

	if (settings.scanlineCount == 1)
	{
		COORD charBufSize = { fw,fh };
		COORD startCharPos = { 0,0 };
		SMALL_RECT writeRect = { 0,0,fw,fh };
		WriteConsoleOutputA(outputHandle, output, charBufSize, startCharPos, &writeRect);
	}
	else
	{
		for (int i = 0; i < fh; i += settings.scanlineCount * settings.scanlineHeight)
		{
			int sy = i + (scanline * settings.scanlineHeight);
			int sh = settings.scanlineHeight;
			if (sy >= fh) { break; }
			else if (sy + sh > fh) { sh = fh - sy; }

			COORD charBufSize = { fw,fh };
			COORD startCharPos = { 0,sy };
			SMALL_RECT writeRect = { 0,sy,fw,sy + sh };
			WriteConsoleOutputA(outputHandle, output, charBufSize, startCharPos, &writeRect);
		}

		scanline++;
		if (scanline == settings.scanlineCount) { scanline = 0; }
	}
	#endif
}

static void getConsoleInfo(ConsoleInfo* consoleInfo)
{
	const double DEFAULT_FONT_RATIO = 8.0 / 18.0;

	int fullConW, fullConH;
	double fontRatio;

	#ifdef _WIN32

	CONSOLE_SCREEN_BUFFER_INFO consoleBufferInfo;
	GetConsoleScreenBufferInfo(outputHandle, &consoleBufferInfo);

	fullConW = consoleBufferInfo.srWindow.Right - consoleBufferInfo.srWindow.Left + 1;
	fullConH = consoleBufferInfo.srWindow.Bottom - consoleBufferInfo.srWindow.Top + 1;

	RECT clientRect = { 0 };
	GetClientRect(conHWND, &clientRect);

	if (clientRect.bottom == 0 || fullConW == 0 || fullConH == 0)
	{
		CONSOLE_FONT_INFO consoleFontInfo;
		GetCurrentConsoleFont(outputHandle, FALSE, &consoleFontInfo);

		if (consoleFontInfo.dwFontSize.X == 0 ||
			consoleFontInfo.dwFontSize.Y == 0)
		{
			fontRatio = DEFAULT_FONT_RATIO;
		}
		else
		{
			fontRatio = (double)consoleFontInfo.dwFontSize.X /
				(double)consoleFontInfo.dwFontSize.Y;
		}
	}
	else
	{
		if (wtDragBarHWND && IsWindowVisible(wtDragBarHWND))
		{
			RECT wtDragBarRect;
			GetClientRect(wtDragBarHWND, &wtDragBarRect);
			clientRect.bottom -= wtDragBarRect.bottom;
		}

		fontRatio = ((double)clientRect.right / (double)fullConW) /
			((double)clientRect.bottom / (double)fullConH);
	}

	#else

	struct winsize winSize;
	ioctl(0, TIOCGWINSZ, &winSize);
	fullConW = winSize.ws_col;
	fullConH = winSize.ws_row;
	fontRatio = DEFAULT_FONT_RATIO;

	#endif

	if (fullConW < 4) { fullConW = 4; }
	if (fullConH < 4) { fullConH = 4; }

	if (settings.colorMode != CM_WINAPI_GRAY && settings.colorMode != CM_WINAPI_16) { fullConW--; }

	consoleInfo->conW = fullConW;
	consoleInfo->conH = fullConH;
	consoleInfo->fontRatio = fontRatio;
}

static void setConstColor(void)
{
	switch (settings.setColorMode)
	{
	case SCM_WINAPI:
		#ifdef _WIN32
		SetConsoleTextAttribute(outputHandle, (WORD)settings.setColorVal1);
		#endif
		break;

	case SCM_CSTD_256:
		printf("\x1B[38;5;%dm", settings.setColorVal1);
		if (settings.setColorVal2 != -1)
		{
			printf("\x1B[48;5;%dm", settings.setColorVal2);
		}
		break;

	case SCM_CSTD_RGB:
		printf("\x1B[38;2;%d;%d;%dm",
			(settings.setColorVal1 & 0xFF0000) >> 16,
			(settings.setColorVal1 & 0x00FF00) >> 8,
			settings.setColorVal1 & 0x0000FF);
		if (settings.setColorVal2 != -1)
		{
			printf("\x1B[48;2;%d;%d;%dm",
				(settings.setColorVal2 & 0xFF0000) >> 16,
				(settings.setColorVal2 & 0x00FF00) >> 8,
				settings.setColorVal2 & 0x0000FF);
		}
		break;
	}
}