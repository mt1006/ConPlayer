#include "conplayer.h"

// https://devblogs.microsoft.com/commandline/updating-the-windows-console-colors/
static const uint8_t CMD_COLORS_16[16][3] =
{
	{12,12,12},{0,55,218},{19,161,14},{58,150,221},
	{197,15,31},{136,23,152},{193,156,0},{204,204,204},
	{118,118,118},{59,120,255},{22,198,12},{97,214,214},
	{231,72,86},{180,0,158},{249,241,165},{242,242,242}
};

static void processImage(Frame* frame, int x, int y, int w, int h, uint8_t* output, int* outputLineOffsets);
static void processForWinAPI(Frame* frame);
static void processForGlConsole(Frame* frame);
static uint8_t procColor(uint8_t* r, uint8_t* g, uint8_t* b);
static void procRand(uint8_t* val);
static uint8_t findNearestColor16(uint8_t r, uint8_t g, uint8_t b);
static uint8_t rgbToAnsi256(uint8_t r, uint8_t g, uint8_t b);
static void rgbFromAnsi256(uint8_t ansi, uint8_t* r, uint8_t* g, uint8_t* b);

void processFrame(Frame* frame)
{
	if (settings.useFakeConsole)
	{
		processForGlConsole(frame);
	}
	else if (settings.colorMode == CM_WINAPI_GRAY ||
		settings.colorMode == CM_WINAPI_16)
	{
		processForWinAPI(frame);
	}
	else
	{
		processImage(frame, 0, 0, frame->w, frame->h,
			(uint8_t*)frame->output, frame->outputLineOffsets);
	}

}

static void processImage(Frame* frame, int x, int y, int w, int h, uint8_t* output, int* outputLineOffsets)
{
	if (settings.colorMode == CM_CSTD_16 ||
		settings.colorMode == CM_CSTD_256 ||
		settings.colorMode == CM_CSTD_RGB)
	{
		outputLineOffsets[0] = 0;
		int yPos = y;

		for (int i = 0; i < h; i++)
		{
			uint8_t oldColor = -1;
			uint8_t oldR = -1, oldG = -1, oldB = -1;
			int isFirstChar = 1;

			int offset = outputLineOffsets[i];
			int xPos = x;

			for (int j = 0; j < w; j++)
			{
				uint8_t valR = frame->videoFrame[(xPos * 3) + (yPos * frame->videoLinesize)];
				uint8_t valG = frame->videoFrame[(xPos * 3) + (yPos * frame->videoLinesize) + 1];
				uint8_t valB = frame->videoFrame[(xPos * 3) + (yPos * frame->videoLinesize) + 2];

				uint8_t val, color;

				if (settings.colorProcMode == CPM_NONE) { val = 255; }
				else { val = procColor(&valR, &valG, &valB); }

				if (settings.brightnessRand) { procRand(&val); }

				switch (settings.colorMode)
				{
				case CM_CSTD_16:
					color = findNearestColor16(valR, valG, valB);
					color = (color & 0b1010) | ((color & 4) >> 2) | ((color & 1) << 2);
					if (color > 7) { color += 82; }
					else { color += 30; }

					if (color == oldColor && !isFirstChar)
					{
						output[offset] = settings.charset[(val * settings.charsetSize) / 256];
						offset++;
						break;
					}
					oldColor = color;

					output[offset] = '\x1B';
					output[offset + 1] = '[';
					output[offset + 2] = (char)(((color / 10) % 10) + 0x30);
					output[offset + 3] = (char)((color % 10) + 0x30);
					output[offset + 4] = 'm';
					output[offset + 5] = settings.charset[(val * settings.charsetSize) / 256];

					offset += 6;
					break;

				case CM_CSTD_256:
					color = rgbToAnsi256(valR, valG, valB);

					if (color == oldColor && !isFirstChar)
					{
						output[offset] = settings.charset[(val * settings.charsetSize) / 256];
						offset++;
						break;
					}
					oldColor = color;

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
					output[offset + 11] = settings.charset[(val * settings.charsetSize) / 256];

					offset += 12;
					break;

				case CM_CSTD_RGB:
					if (valR == oldR && valG == oldG && valB == oldB && !isFirstChar)
					{
						output[offset] = settings.charset[(val * settings.charsetSize) / 256];
						offset++;
						break;
					}
					oldR = valR;
					oldG = valG;
					oldB = valB;

					output[offset] = '\x1B';
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
					output[offset + 19] = settings.charset[(val * settings.charsetSize) / 256];

					offset += 20;
					break;
				}

				isFirstChar = 0;
				xPos++;
			}

			output[offset] = '\n';
			outputLineOffsets[i + 1] = offset + 1;
			yPos++;
		}

		if (!settings.disableCLS)
		{
			output[outputLineOffsets[h] - 1] = '\0';
			outputLineOffsets[h]--;
		}
	}
	else
	{
		int fullW = w + 1;
		outputLineOffsets[0] = 0;
		int yPos = y;

		for (int i = 0; i < h; i++)
		{
			int xPos = x;

			for (int j = 0; j < w; j++)
			{
				uint8_t val = frame->videoFrame[(yPos * frame->videoLinesize) + xPos];
				if (settings.brightnessRand) { procRand(&val); }
				output[(i * fullW) + j] = settings.charset[(val * settings.charsetSize) / 256];
				xPos++;
			}

			output[(i * fullW) + w] = '\n';
			outputLineOffsets[i + 1] = ((i + 1) * fullW);
			yPos++;
		}

		if (!settings.disableCLS)
		{
			output[((h - 1) * fullW) + w] = '\0';
			outputLineOffsets[h]--;
		}
	}
}

static void processForWinAPI(Frame* frame)
{
	#ifdef _WIN32

	CHAR_INFO* output = (CHAR_INFO*)frame->output;
	int w = frame->w;
	int h = frame->h;

	if (settings.colorMode == CM_WINAPI_16)
	{
		for (int i = 0; i < h; i++)
		{
			for (int j = 0; j < w; j++)
			{
				uint8_t valR = frame->videoFrame[(j * 3) + (i * frame->videoLinesize)];
				uint8_t valG = frame->videoFrame[(j * 3) + (i * frame->videoLinesize) + 1];
				uint8_t valB = frame->videoFrame[(j * 3) + (i * frame->videoLinesize) + 2];

				uint8_t val;
				
				if (settings.colorProcMode == CPM_NONE) { val = 255; }
				else { val = procColor(&valR, &valG, &valB); }

				if (settings.brightnessRand) { procRand(&val); }

				output[(i * w) + j].Char.AsciiChar = settings.charset[(val * settings.charsetSize) / 256];
				output[(i * w) + j].Attributes = findNearestColor16(valR, valG, valB);
			}
		}
	}
	else
	{
		for (int i = 0; i < h; i++)
		{
			for (int j = 0; j < w; j++)
			{
				uint8_t val = frame->videoFrame[j + i * frame->videoLinesize];
				if (settings.brightnessRand) { procRand(&val); }
				output[(i * w) + j].Char.AsciiChar = settings.charset[(val * settings.charsetSize) / 256];
				
				if (settings.setColorMode == SCM_WINAPI)
				{
					output[(i * w) + j].Attributes = settings.setColorVal1;
				}
				else
				{
					output[(i * w) + j].Attributes =
						FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
				}
			}
		}
	}

	#endif
}

static void processForGlConsole(Frame* frame)
{
	#ifndef CP_DISABLE_OPENGL
	GlConsoleChar* output = (GlConsoleChar*)frame->output;
	int w = frame->w;
	int h = frame->h;

	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			uint8_t val, valR, valG, valB;

			if (settings.colorMode == CM_WINAPI_GRAY || settings.colorMode == CM_CSTD_GRAY)
			{
				val = frame->videoFrame[(i * frame->videoLinesize) + j];
				valR = CMD_COLORS_16[7][0];
				valG = CMD_COLORS_16[7][1];
				valB = CMD_COLORS_16[7][2];
			}
			else if (settings.colorMode == CM_WINAPI_16 || settings.colorMode == CM_CSTD_16)
			{
				valR = frame->videoFrame[(j * 3) + (i * frame->videoLinesize)];
				valG = frame->videoFrame[(j * 3) + (i * frame->videoLinesize) + 1];
				valB = frame->videoFrame[(j * 3) + (i * frame->videoLinesize) + 2];

				if (settings.colorProcMode == CPM_NONE) { val = 255; }
				else { val = procColor(&valR, &valG, &valB); }

				uint8_t color = findNearestColor16(valR, valG, valB);

				valR = CMD_COLORS_16[color][0];
				valG = CMD_COLORS_16[color][1];
				valB = CMD_COLORS_16[color][2];
			}
			else
			{
				valR = frame->videoFrame[(j * 3) + (i * frame->videoLinesize)];
				valG = frame->videoFrame[(j * 3) + (i * frame->videoLinesize) + 1];
				valB = frame->videoFrame[(j * 3) + (i * frame->videoLinesize) + 2];

				if (settings.colorProcMode == CPM_NONE) { val = 255; }
				else { val = procColor(&valR, &valG, &valB); }

				if (settings.colorMode == CM_CSTD_256)
				{
					rgbFromAnsi256(rgbToAnsi256(valR, valG, valB), &valR, &valG, &valB);
				}
			}

			if (settings.brightnessRand) { procRand(&val); }

			//double pos = ((double)val / 255.0) * (settings.charsetSize - 1);
			//double dec = pos - floor(pos);
			//int charPos;

			//if ((rand() % 1000) < (int)(dec * 1000.0)) { charPos = (int)ceil(pos); }
			//else { charPos = (int)floor(pos); }
			// 
			//if (charPos >= settings.charsetSize) { error("vsddsfsfd", "fdfds", 0); }
			//output[(i * w) + j].ch = settings.charset[charPos];

			output[(i * w) + j].ch = settings.charset[(val * settings.charsetSize) / 256];
			output[(i * w) + j].r = valR;
			output[(i * w) + j].g = valG;
			output[(i * w) + j].b = valB;
		}
	}
	#endif
}

static uint8_t procColor(uint8_t* r, uint8_t* g, uint8_t* b)
{
	uint8_t valR = *r, valG = *g, valB = *b;

	if (settings.colorProcMode == CPM_BOTH)
	{
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
	}

	return (uint8_t)((double)valR * 0.299 + (double)valG * 0.587 + (double)valB * 0.114);
}

static void procRand(uint8_t* val)
{
	if (settings.colorProcMode == CPM_NONE)
	{
		*val -= rand() % (settings.brightnessRand + 1);
	}
	else
	{
		if (settings.brightnessRand > 0)
		{
			int tempVal = (int)(*val) +
				(rand() % (settings.brightnessRand + 1)) -
				(settings.brightnessRand / 2);

			if (tempVal >= 255) { *val = 255; }
			else if (tempVal <= 0) { *val = 0; }
			else { *val = (uint8_t)tempVal; }
		}
		else
		{
			int tempVal = (int)(*val) -
				(rand() % (-settings.brightnessRand + 1));

			if (tempVal <= 0) { *val = 0; }
			else { *val = (uint8_t)tempVal; }
		}
	}
}

static uint8_t findNearestColor16(uint8_t r, uint8_t g, uint8_t b)
{
	int min = INT_MAX;
	int minPos = 0;
	for (int i = 0; i < 16; i++)
	{
		int diff = (r - CMD_COLORS_16[i][0]) * (r - CMD_COLORS_16[i][0]) +
			(g - CMD_COLORS_16[i][1]) * (g - CMD_COLORS_16[i][1]) +
			(b - CMD_COLORS_16[i][2]) * (b - CMD_COLORS_16[i][2]);
		if (diff < min)
		{
			min = diff;
			minPos = i;
		}
	}
	return (uint8_t)minPos;
}

static uint8_t rgbToAnsi256(uint8_t r, uint8_t g, uint8_t b)
{
	// https://stackoverflow.com/a/26665998
	if (r == g && g == b)
	{
		if (r < 8) { return 16; }
		if (r > 248) { return 231; }
		return (uint8_t)round((((double)r - 8.0) / 247.0) * 24.0) + 232;
	}

	return (uint8_t)(16.0
		+ (36.0 * round((double)r / 51.0))  // 51 = 255/5
		+ (6.0 * round((double)g / 51.0))
		+ round((double)b / 51.0));
}

static void rgbFromAnsi256(uint8_t ansi, uint8_t* r, uint8_t* g, uint8_t* b)
{
	if (ansi < 16) { error("ANSI code smaller than 16!", "processFrame.c", __LINE__); }
	
	if (ansi > 232)
	{
		uint8_t val = (uint8_t)((double)(ansi - 232) * (240.0 / 23.0)) + 8;
	}
	else
	{
		*r = ((ansi - 16) / 36) * 51;
		*g = (((ansi - 16) % 36) / 6) * 51;
		*b = ((ansi - 16) % 6) * 51;
	}
}