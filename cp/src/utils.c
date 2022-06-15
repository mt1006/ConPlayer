#include "conplayer.h"

double getTime(void)
{
	return (double)clock() / (double)CLOCKS_PER_SEC;
}

void clearScreen(HANDLE handle)
{
	// https://docs.microsoft.com/en-us/windows/console/clearing-the-screen
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	SMALL_RECT scrollRect;
	COORD scrollTarget;
	CHAR_INFO fill;

	GetConsoleScreenBufferInfo(handle, &csbi);

	scrollRect.Left = 0;
	scrollRect.Top = 0;
	scrollRect.Right = csbi.dwSize.X;
	scrollRect.Bottom = csbi.dwSize.Y;
	
	scrollTarget.X = 0;
	scrollTarget.Y = (SHORT)(0 - csbi.dwSize.Y);

	fill.Char.UnicodeChar = TEXT(' ');
	fill.Attributes = csbi.wAttributes;

	ScrollConsoleScreenBuffer(handle, &scrollRect, NULL, scrollTarget, &fill);

	csbi.dwCursorPosition.X = 0;
	csbi.dwCursorPosition.Y = 0;

	SetConsoleCursorPosition(handle, csbi.dwCursorPosition);
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

ColorMode colorModeFromStr(char* str)
{
	return CM_CSTD_RGB;
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
		char* output = (char*)malloc(inputLen * sizeof(char));
		memcpy(output, input, inputLen);
		return output;
	}
}

void error(const char* description, const char* fileName, int line)
{
	puts("\nSomething went wrong...");
	printf("%s [%s:%d]\n", description, fileName, line);
	exit(-1);
}