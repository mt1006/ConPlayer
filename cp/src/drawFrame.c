#include "conplayer.h"

typedef struct 
{
	int w;
	int h;
	double fontRatio;
} ConsoleInfo;

HANDLE outputHandle = NULL;

static void drawWithWinAPI(CHAR_INFO* output, int w, int h);
static void getConsoleInfo(ConsoleInfo* consoleInfo);
static void setConstColor(void);

void initDrawFrame(void)
{
	if (vidW == -1 || vidH == -1) { return; }

	#ifdef _WIN32
	if (settings.useFakeConsole) { SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE); }

	getConsoleWindow();
	outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	if (settings.colorMode == CM_CSTD_16 ||
		settings.colorMode == CM_CSTD_256 ||
		settings.colorMode == CM_CSTD_RGB ||
		settings.setColorMode == SCM_CSTD_256 ||
		settings.setColorMode == SCM_CSTD_RGB)
	{
		enableANSI();
	}
	#endif

	#ifndef CP_DISABLE_OPENGL
	if (settings.useFakeConsole) { refreshFont(); }
	#endif

	refreshSize();
}

void refreshSize(void)
{
	static bool firstCall = true;
	static bool setNewSize = false;
	static int oldW = 0, oldH = 0;
	static double fontRatio = 0.0;
	static ConsoleInfo oldConsoleInfo = { -1,-1,-1.0 };

	ConsoleInfo consoleInfo;
	getConsoleInfo(&consoleInfo);

	int newW = oldW, newH = oldH;

	if (settings.argW != -1 && settings.argH != -1)
	{
		if (settings.argW == 0 && settings.argH == 0)
		{
			settings.argW = consoleInfo.w;
			settings.argH = consoleInfo.h;
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
			if (consoleInfo.w != oldConsoleInfo.w ||
				consoleInfo.h != oldConsoleInfo.h)
			{
				newW = consoleInfo.w;
				newH = consoleInfo.h;
			}
		}
		else if (consoleInfo.w != oldConsoleInfo.w ||
			consoleInfo.h != oldConsoleInfo.h ||
			fontRatio != consoleInfo.fontRatio)
		{	
			if (settings.constFontRatio == 0.0) { fontRatio = consoleInfo.fontRatio; }
			else { fontRatio = settings.constFontRatio; }

			double vidRatio = (double)vidW / (double)vidH;
			double conRatio = ((double)consoleInfo.w / (double)consoleInfo.h) * fontRatio;

			if (vidRatio > conRatio)
			{
				newW = consoleInfo.w;
				newH = (int)((conRatio / vidRatio) * consoleInfo.h);
			}
			else
			{
				newW = (int)((vidRatio / conRatio) * consoleInfo.w);
				newH = consoleInfo.h;
			}
		}
	}

	if (firstCall)
	{
		conW = newW;
		conH = newH;
		oldW = conW;
		oldH = conH;
		firstCall = false;
	}
	else
	{
		if (newW != oldW || newH != oldH)
		{
			oldW = newW;
			oldH = newH;
			setNewSize = true;
		}
		else if (setNewSize)
		{
			conW = newW;
			conH = newH;
			setNewSize = true;
		}
	}

	oldConsoleInfo = consoleInfo;
}

void drawFrame(void* output, int* lineOffsets, int w, int h)
{
	static int scanline = 0;
	static int lastW = -1, lastH = -1;

	#ifndef CP_DISABLE_OPENGL
	if (settings.useFakeConsole)
	{
		drawWithOpenGL((GlConsoleChar*)output, w, h);
		return;
	}
	#endif

	if ((lastW != w || lastH != h) && !settings.disableCLS)
	{
		lastW = w;
		lastH = h;
		clearScreen();
	}

	if (settings.colorMode == CM_WINAPI_GRAY || settings.colorMode == CM_WINAPI_16)
	{
		drawWithWinAPI((CHAR_INFO*)output, w, h);
		return;
	}

	setConstColor();

	if (settings.scanlineCount == 1)
	{
		if (!settings.disableCLS) { setCursorPos(0, 0); }
		fwrite(output, 1, lineOffsets[h], stdout);
	}
	else
	{
		for (int i = 0; i < h; i += settings.scanlineCount * settings.scanlineHeight)
		{
			int sy = i + (scanline * settings.scanlineHeight);
			int sh = settings.scanlineHeight;
			if (sy >= h) { break; }
			else if (sy + sh > h) { sh = h - sy; }

			if (!settings.disableCLS) { setCursorPos(0, sy); }
			fwrite((char*)output + lineOffsets[sy], 1, lineOffsets[sy + sh] - lineOffsets[sy], stdout);
		}

		scanline++;
		if (scanline == settings.scanlineCount) { scanline = 0; }
	}
}

static void drawWithWinAPI(CHAR_INFO* output, int w, int h)
{
	#ifdef _WIN32

	static int scanline = 0;

	if (settings.scanlineCount == 1)
	{
		COORD charBufSize = { w,h };
		COORD startCharPos = { 0,0 };
		SMALL_RECT writeRect = { 0,0,w,h };
		WriteConsoleOutputA(outputHandle, output, charBufSize, startCharPos, &writeRect);
	}
	else
	{
		for (int i = 0; i < h; i += settings.scanlineCount * settings.scanlineHeight)
		{
			int sy = i + (scanline * settings.scanlineHeight);
			int sh = settings.scanlineHeight;
			if (sy >= h) { break; }
			else if (sy + sh > h) { sh = h - sy; }

			COORD charBufSize = { w,h };
			COORD startCharPos = { 0,sy };
			SMALL_RECT writeRect = { 0,sy,w,sy + sh };
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

	int fullW, fullH;
	double fontRatio;

	#ifdef _WIN32

	if (settings.useFakeConsole)
	{
		#ifndef CP_DISABLE_OPENGL
		while (volGlCharW == 0.0f) { Sleep(0); }

		RECT clientRect = { 0 };
		GetClientRect(glConsoleHWND, &clientRect);

		fullW = (int)round((double)clientRect.right / (double)volGlCharW);
		fullH = (int)round((double)clientRect.bottom / (double)volGlCharH);
		fontRatio = (double)volGlCharW / (double)volGlCharH;
		#endif
	}
	else
	{
		getConsoleSize(&fullW, &fullH);

		RECT clientRect = { 0 };
		GetClientRect(conHWND, &clientRect);

		if (clientRect.bottom == 0 || fullW == 0 || fullH == 0)
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

			fontRatio = ((double)clientRect.right / (double)fullW) /
				((double)clientRect.bottom / (double)fullH);
		}
	}

	#else

	getConsoleSize(&fullW, &fullH);
	fontRatio = DEFAULT_FONT_RATIO;

	#endif

	if (fullW < 4) { fullW = 4; }
	if (fullH < 4) { fullH = 4; }

	if (settings.colorMode != CM_WINAPI_GRAY && settings.colorMode != CM_WINAPI_16) { fullW--; }

	consoleInfo->w = fullW;
	consoleInfo->h = fullH;
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