#include "conplayer.h"

static HANDLE outputHandle;

static void drawWithWinAPI(CHAR_INFO* output, int fw, int fh);

void initDrawFrame(void)
{
	if (vidW == -1 || vidH == -1) { return; }
	outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	if (colorMode == CM_CSTD_16 ||
		colorMode == CM_CSTD_256 ||
		colorMode == CM_CSTD_RGB)
	{
		DWORD mode;
		GetConsoleMode(outputHandle, &mode);
		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(outputHandle, mode);
	}

	refreshSize();
}

void refreshSize(void)
{
	static int firstCall = 1;
	static int setNewSize = 0;
	static int lastW = 0, lastH = 0;

	CONSOLE_SCREEN_BUFFER_INFOEX consoleBufferInfo;
	CONSOLE_FONT_INFOEX consoleFontInfo;
	consoleBufferInfo.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	consoleFontInfo.cbSize = sizeof(CONSOLE_FONT_INFOEX);
	GetConsoleScreenBufferInfoEx(outputHandle, &consoleBufferInfo);
	GetCurrentConsoleFontEx(outputHandle, FALSE, &consoleFontInfo);
	int newConW = consoleBufferInfo.srWindow.Right - consoleBufferInfo.srWindow.Left + 1;
	int newConH = consoleBufferInfo.srWindow.Bottom - consoleBufferInfo.srWindow.Top + 1;
	if (colorMode != CM_WINAPI_GRAY && colorMode != CM_WINAPI_16) { newConW--; }

	if (newConW < 4) { newConW = 4; }
	if (newConH < 4) { newConH = 4; }

	if (consoleFontInfo.dwFontSize.X == 0 ||
		consoleFontInfo.dwFontSize.Y == 0)
	{
		consoleFontInfo.dwFontSize.X = 8;
		consoleFontInfo.dwFontSize.Y = 18;
	}

	int newW = lastW, newH = lastH;

	if (argW != -1 && argH != -1)
	{
		if (argW == 0 && argH == 0)
		{
			argW = newConW;
			argH = newConH;
		}

		if (fillArea)
		{
			if (firstCall)
			{
				newW = argW;
				newH = argH;
			}
		}
		else if (fontW != consoleFontInfo.dwFontSize.X ||
			fontH != consoleFontInfo.dwFontSize.Y)
		{
			fontW = consoleFontInfo.dwFontSize.X;
			fontH = consoleFontInfo.dwFontSize.Y;
			int fullW = argW * fontW;
			int fullH = argH * fontH;

			double vidRatio = (double)vidW / (double)vidH;
			double conRatio = (double)fullW / (double)fullH;
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
			if (conW != newConW || conH != newConH)
			{
				conW = newConW;
				conH = newConH;
				newW = newConW;
				newH = newConH;
			}
		}
		else if (conW != newConW || conH != newConH ||
			fontW != consoleFontInfo.dwFontSize.X ||
			fontH != consoleFontInfo.dwFontSize.Y)
		{
			conW = newConW;
			conH = newConH;
			fontW = consoleFontInfo.dwFontSize.X;
			fontH = consoleFontInfo.dwFontSize.Y;
			int fullW = conW * fontW;
			int fullH = conH * fontH;

			double vidRatio = (double)vidW / (double)vidH;
			double conRatio = (double)fullW / (double)fullH;
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
	if (lastFW != fw || lastFH != fh)
	{
		lastFW = fw;
		lastFH = fh;
		clearScreen(outputHandle);
	}

	if (colorMode == CM_WINAPI_GRAY || colorMode == CM_WINAPI_16)
	{
		drawWithWinAPI((CHAR_INFO*)output, fw, fh);
		return;
	}

	if (scanlineCount == 1)
	{
		COORD cursor = { 0,0 };
		SetConsoleCursorPosition(outputHandle, cursor);
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

			COORD cursor = { 0,sy };
			SetConsoleCursorPosition(outputHandle, cursor);
			fwrite((char*)output + lineOffsets[sy], 1, lineOffsets[sy + sh] - lineOffsets[sy], stdout);
		}

		scanline++;
		if (scanline == scanlineCount) { scanline = 0; }
	}
}

static void drawWithWinAPI(CHAR_INFO* output, int fw, int fh)
{
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
}