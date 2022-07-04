#include "conplayer.h"

typedef struct 
{
	int conW;
	int conH;
	double fontRatio;
} ConsoleInfo;

HANDLE outputHandle = NULL;
static HWND conHWND = NULL;

static void drawWithWinAPI(CHAR_INFO* output, int fw, int fh);
static void getConsoleInfo(ConsoleInfo* consoleInfo);

void initDrawFrame(void)
{
	if (vidW == -1 || vidH == -1) { return; }

	#ifdef _WIN32
	conHWND = GetConsoleWindow();
	outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	if (colorMode == CM_CSTD_16 ||
		colorMode == CM_CSTD_256 ||
		colorMode == CM_CSTD_RGB ||
		setColorMode == SCM_CSTD_256 ||
		setColorMode == SCM_CSTD_RGB)
	{
		DWORD mode;
		GetConsoleMode(outputHandle, &mode);
		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(outputHandle, mode);
	}
	#endif

	switch (setColorMode)
	{
	case SCM_WINAPI:
		#ifdef _WIN32
		SetConsoleTextAttribute(outputHandle, (WORD)setColorVal);
		#endif
		break;
	
	case SCM_CSTD_256:
		printf("\x1B[38;5;%dm", setColorVal);
		break;

	case SCM_CSTD_RGB:
		printf("\x1B[38;2;%d;%d;%dm",
			(setColorVal & 0xFF0000) >> 16,
			(setColorVal & 0x00FF00) >> 8,
			setColorVal & 0x0000FF);
		break;
	}

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

	if (argW != -1 && argH != -1)
	{
		if (argW == 0 && argH == 0)
		{
			argW = consoleInfo.conW;
			argH = consoleInfo.conH;
		}

		if (fillArea)
		{
			if (firstCall)
			{
				newW = argW;
				newH = argH;
			}
		}
		else if (fontRatio != consoleInfo.fontRatio)
		{
			if (constFontRatio == 0.0) { fontRatio = consoleInfo.fontRatio; }
			else { fontRatio = constFontRatio; }

			double vidRatio = (double)vidW / (double)vidH;
			double conRatio = ((double)argW / (double)argH) * fontRatio;

			if (vidRatio > conRatio)
			{
				newW = argW;
				newH = (int)((conRatio / vidRatio) * argH);
			}
			else
			{
				newW = (int)((vidRatio / conRatio) * argW);
				newH = argH;
			}
		}
	}
	else
	{
		if (fillArea)
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
			
			if (constFontRatio == 0.0) { fontRatio = consoleInfo.fontRatio; }
			else { fontRatio = constFontRatio; }

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
	if ((lastFW != fw || lastFH != fh) && !disableCLS)
	{
		lastFW = fw;
		lastFH = fh;
		clearScreen();
	}

	if (colorMode == CM_WINAPI_GRAY || colorMode == CM_WINAPI_16)
	{
		drawWithWinAPI((CHAR_INFO*)output, fw, fh);
		return;
	}

	if (scanlineCount == 1)
	{
		if (!disableCLS) { setCursorPos(0, 0); }
		fwrite((char*)output, 1, lineOffsets[fh], stdout);
	}
	else
	{
		for (int i = 0; i < fh; i += scanlineCount * scanlineHeight)
		{
			int sy = i + (scanline * scanlineHeight);
			int sh = scanlineHeight;
			if (sy >= fh) { break; }
			else if (sy + sh > fh) { sh = fh - sy; }

			if (!disableCLS) { setCursorPos(0, sy); }
			fwrite((char*)output + lineOffsets[sy], 1, lineOffsets[sy + sh] - lineOffsets[sy], stdout);
		}

		scanline++;
		if (scanline == scanlineCount) { scanline = 0; }
	}
}

static void drawWithWinAPI(CHAR_INFO* output, int fw, int fh)
{
	#ifdef _WIN32
	static int scanline = 0;

	if (scanlineCount == 1)
	{
		COORD charBufSize = { fw,fh };
		COORD startCharPos = { 0,0 };
		SMALL_RECT writeRect = { 0,0,fw,fh };
		WriteConsoleOutputA(outputHandle, output, charBufSize, startCharPos, &writeRect);
	}
	else
	{
		for (int i = 0; i < fh; i += scanlineCount * scanlineHeight)
		{
			int sy = i + (scanline * scanlineHeight);
			int sh = scanlineHeight;
			if (sy >= fh) { break; }
			else if (sy + sh > fh) { sh = fh - sy; }

			COORD charBufSize = { fw,fh };
			COORD startCharPos = { 0,sy };
			SMALL_RECT writeRect = { 0,sy,fw,sy + sh };
			WriteConsoleOutputA(outputHandle, output, charBufSize, startCharPos, &writeRect);
		}

		scanline++;
		if (scanline == scanlineCount) { scanline = 0; }
	}
	#endif
}

static void getConsoleInfo(ConsoleInfo* consoleInfo)
{
	const double DEFAULT_FONT_RATIO = 8.0 / 18.0;
	const int USE_GET_CURRENT_CONSOLE_FONT = 0;

	int fullConW, fullConH;
	double fontRatio;

	#ifdef _WIN32

	CONSOLE_SCREEN_BUFFER_INFOEX consoleBufferInfo;
	consoleBufferInfo.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	GetConsoleScreenBufferInfoEx(outputHandle, &consoleBufferInfo);

	fullConW = consoleBufferInfo.srWindow.Right - consoleBufferInfo.srWindow.Left + 1;
	fullConH = consoleBufferInfo.srWindow.Bottom - consoleBufferInfo.srWindow.Top + 1;

	RECT clientRect = { 0 };
	if (!USE_GET_CURRENT_CONSOLE_FONT) { GetClientRect(conHWND, &clientRect); }

	if (clientRect.bottom == 0 || fullConW == 0 || fullConH == 0)
	{
		CONSOLE_FONT_INFOEX consoleFontInfo;
		consoleFontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
		GetCurrentConsoleFontEx(outputHandle, FALSE, &consoleFontInfo);

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

	if (colorMode != CM_WINAPI_GRAY && colorMode != CM_WINAPI_16) { fullConW--; }

	consoleInfo->conW = fullConW;
	consoleInfo->conH = fullConH;
	consoleInfo->fontRatio = fontRatio;
}