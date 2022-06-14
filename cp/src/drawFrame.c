#include "conplayer.h"

//char* symbolSet = " .-+*?#M&%@";
char* symbolSet = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B$@";

// https://devblogs.microsoft.com/commandline/updating-the-windows-console-colors/
static const uint8_t WIN_CMD_COLORS[16][3] =
{
	{12,12,12},{0,55,218},{19,161,14},{58,150,221},
	{197,15,31},{136,23,152},{193,156,0},{204,204,204},
	{118,118,118},{59,120,255},{22,198,12},{97,214,214},
	{231,72,86},{180,0,158},{249,241,165},{242,242,242}
};

static int symbolSetSize;
static HANDLE outputHandle;

static void processForWinAPI(Frame* frame);
static void drawWithWinAPI(CHAR_INFO* output, int fw, int fh);
static uint8_t procColor(uint8_t* r, uint8_t* g, uint8_t* b);

void initConsole(void)
{
	if (vidW == -1 || vidH == -1) { return; }
	symbolSetSize = (int)strlen(symbolSet);
	outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	if (useCStdOut && withColors)
	{
		DWORD mode;
		GetConsoleMode(outputHandle, &mode);
		mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(outputHandle, mode);
	}

	refreshSize();
}

void processFrame(Frame* frame)
{
	if (!useCStdOut)
	{
		processForWinAPI(frame);
		return;
	}

	uint8_t* output = (uint8_t*)frame->output;
	if (withColors == 1)
	{
		int fullW = (frame->frameW * ANSI_CODE_LEN) + 1;
		for (int i = 0; i < frame->frameH; i++)
		{
			for (int j = 0; j < frame->frameW; j++)
			{
				uint8_t valR = frame->videoFrame[(j * 3) + (i * frame->videoLinesize)];
				uint8_t valG = frame->videoFrame[(j * 3) + (i * frame->videoLinesize) + 1];
				uint8_t valB = frame->videoFrame[(j * 3) + (i * frame->videoLinesize) + 2];

				uint8_t val = procColor(&valR, &valG, &valB);
				uint8_t color = rgbToAnsi256(valR, valG, valB);

				int offset = (i * fullW) + (j * ANSI_CODE_LEN);
				output[offset] = '\x1B';
				output[offset + 1] = '[';
				output[offset + 2] = '3';
				output[offset + 3] = '8';
				output[offset + 4] = ';';
				output[offset + 5] = '5';
				output[offset + 6] = ';';
				output[offset + 7] = (char)(((color / 100) % 10) + 0x30);
				output[offset + 8] = (char)(((color / 10) % 10) + 0x30);
				output[offset + 9] = (char)((color % 10) + 0x30);
				output[offset + 10] = 'm';
				output[offset + 11] = symbolSet[(val * symbolSetSize) / 256];

				/*output[offset] = '\x1B';
				output[offset + 1] = '[';
				output[offset + 2] = '3';
				output[offset + 3] = '8';
				output[offset + 4] = ';';
				output[offset + 5] = '2';
				output[offset + 6] = ';';
				output[offset + 7] = (char)(((valR / 100) % 10) + 0x30);
				output[offset + 8] = (char)(((valR / 10) % 10) + 0x30);
				output[offset + 9] = (char)((valR % 10) + 0x30);
				output[offset + 10] = ';';
				output[offset + 11] = (char)(((valG / 100) % 10) + 0x30);
				output[offset + 12] = (char)(((valG / 10) % 10) + 0x30);
				output[offset + 13] = (char)((valG % 10) + 0x30);
				output[offset + 14] = ';';
				output[offset + 15] = (char)(((valB / 100) % 10) + 0x30);
				output[offset + 16] = (char)(((valB / 10) % 10) + 0x30);
				output[offset + 17] = (char)((valB % 10) + 0x30);
				output[offset + 18] = 'm';
				output[offset + 19] = symbolSet[(val * symbolSetSize) / 256];*/
			}
			output[(i * fullW) + (frame->frameW * ANSI_CODE_LEN)] = '\n';
		}
		output[((frame->frameH - 1) * fullW) + (frame->frameW * ANSI_CODE_LEN)] = '\0';
	}
	else
	{
		int fullW = frame->frameW + 1;
		for (int i = 0; i < frame->frameH; i++)
		{
			for (int j = 0; j < frame->frameW; j++)
			{
				uint8_t val = frame->videoFrame[(i * frame->videoLinesize) + j];
				output[(i * fullW) + j] = symbolSet[(val * symbolSetSize) / 256];
			}
			output[(i * fullW) + frame->frameW] = '\n';
		}
		output[((frame->frameH - 1) * fullW) + frame->frameW] = '\0';
	}
}

void drawFrame(void* output, int fw, int fh)
{
	static int part = 0;

	if (!useCStdOut)
	{
		static int lastFW = 0, lastFH = 0;
		if (lastFW != fw || lastFH != fh)
		{
			lastFW = fw;
			lastFH = fh;
			clearScreen(outputHandle);
		}

		drawWithWinAPI((CHAR_INFO*)output, fw, fh);
		return;
	}

	int charLen;
	if (withColors) { charLen = ANSI_CODE_LEN; }
	else { charLen = 1; }

	if (interlacing == 1)
	{
		COORD cursor = { 0,0 };
		SetConsoleCursorPosition(outputHandle, cursor);
		fwrite((char*)output, 1, ((fw * charLen) + 1) * fh, stdout);
	}
	else
	{
		int partSize = interlacing;
		int partCount = interlacing;

		for (int i = 0; i < fh; i += partSize)
		{
			int ph = partSize;
			if (partSize > (fh - i)) { ph = fh - i; }

			int y = ((ph / partCount) * part) + i;
			int h = 0;

			if (part == partCount - 1) { h = (ph + i) - y; }
			else { h = ph / partCount; }

			int toWrite = ((fw * charLen) + 1) * h;
			if (y + h == fh) { toWrite--; }

			COORD cursor = { 0,y };
			SetConsoleCursorPosition(outputHandle, cursor);
			fwrite((char*)output + (((fw * charLen) + 1) * y), 1, toWrite, stdout);
		}

		part++;
		if (part == partCount) { part = 0; }
	}
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
	if (useCStdOut) { newConW--; }

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

static void processForWinAPI(Frame* frame)
{
	CHAR_INFO* output = (CHAR_INFO*)frame->output;
	if (withColors == 1)
	{
		for (int i = 0; i < frame->frameH; i++)
		{
			for (int j = 0; j < frame->frameW; j++)
			{
				uint8_t valR = frame->videoFrame[(j * 3) + (i * frame->videoLinesize)];
				uint8_t valG = frame->videoFrame[(j * 3) + (i * frame->videoLinesize) + 1];
				uint8_t valB = frame->videoFrame[(j * 3) + (i * frame->videoLinesize) + 2];
				
				uint8_t val = procColor(&valR, &valG, &valB);
				
				output[(i * frame->frameW) + j].Char.AsciiChar = symbolSet[(val * symbolSetSize) / 256];
				int min = INT_MAX;
				int minPos = 0;
				for (int i = 0; i < 16; i++)
				{
					int diff = (valR - WIN_CMD_COLORS[i][0]) * (valR - WIN_CMD_COLORS[i][0]) +
						(valG - WIN_CMD_COLORS[i][1]) * (valG - WIN_CMD_COLORS[i][1]) +
						(valB - WIN_CMD_COLORS[i][2]) * (valB - WIN_CMD_COLORS[i][2]);
					if (diff < min)
					{
						min = diff;
						minPos = i;
					}
				}
				output[(i * frame->frameW) + j].Attributes = minPos;
			}
		}
	}
	else
	{
		for (int i = 0; i < frame->frameH; i++)
		{
			for (int j = 0; j < frame->frameW; j++)
			{
				uint8_t val = frame->videoFrame[j + i * frame->videoLinesize];
				output[(i * frame->frameW) + j].Char.AsciiChar = symbolSet[(val * symbolSetSize) / 256];
				output[(i * frame->frameW) + j].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
			}
		}
	}
}

static void drawWithWinAPI(CHAR_INFO* output, int fw, int fh)
{
	COORD charBufSize = { fw,fh };
	COORD startCharPos = { 0,0 };
	SMALL_RECT writeRect = { 0,0,fw,fh };
	WriteConsoleOutputA(outputHandle, output, charBufSize, startCharPos, &writeRect);
}

static uint8_t procColor(uint8_t* r, uint8_t* g, uint8_t* b)
{
	uint8_t valR = *r, valG = *g, valB = *b;

	if (valR >= valG && valR >= valB)
	{
		*r = 255;
		*g = (uint8_t)(255.0f * ((float)valG / (float)valR));
		*b = (uint8_t)(255.0f * ((float)valB / (float)valR));
	}
	else if (valG >= valR && valG >= valB)
	{
		*r = (uint8_t)(255.0f * ((float)valR / (float)valG));
		*g = 255;
		*b = (uint8_t)(255.0f * ((float)valB / (float)valG));
	}
	else
	{
		*r = (uint8_t)(255.0f * ((float)valR / (float)valB));
		*g = (uint8_t)(255.0f * ((float)valG / (float)valB));
		*b = 255;
	}

	return (uint8_t)((double)valR * 0.299 + (double)valG * 0.587 + (double)valB * 0.114);
}