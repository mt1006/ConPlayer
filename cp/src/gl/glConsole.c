#include "../conplayer.h"

#ifndef CP_DISABLE_OPENGL

typedef struct
{
	GLuint texture;
	float* vertexArray;
	float* colorArray;
	int count;
	int lastUsed;
	bool available;
} GlCharType;

const int CHARACTER_COUNT = 256;

volatile float volGlCharW = 0.0f, volGlCharH = 0.0f;

static float glCharW = 0.0f, glCharH = 0.0f;
static HWND conhostWindow, childWindow;
static HDC hdc = NULL, textDC = NULL;
static PIXELFORMATDESCRIPTOR pfd = { 0 };
static int pfID;
static HGLRC hglrc;
static BITMAPINFO bmpInfo = { 0 };
static HBRUSH bgBrush;
static GlCharType* charset;
static float* texCoordArray = NULL;
static int textureW = 0, textureH = 0;
static float ratioW = 0.0f, ratioH = 0.0f;
static HFONT textFont = NULL;
static MSG msg;
static bool charsetLoaded = false;
static int fontW, fontH;
static volatile bool reqRefreshFont = false;

static HWND getConhostWindow(void);
static void initWindow(void);
static void initOpenGL(void);
static void initCharset(void);
static int CALLBACK enumFontsProc(ENUMLOGFONT* lpelf, NEWTEXTMETRIC* lpntm, DWORD FontType, LPARAM lParam);
static int CALLBACK enumFontFamProc(ENUMLOGFONT* lpelf, NEWTEXTMETRIC* lpntm, DWORD FontType, LPARAM lParam);
static void loadCharset(void);
static void loadCharsetSize(void);
static void createCharacter(char* text, GLuint textureID);
static void setMatrix(void);
static void refreshArrays(int w, int h);
static void makeArraysAvailable(GlCharType* charType, int w, int h);
static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static int nextPowerOf2(int n);

void initOpenGlConsole(void)
{
	getConsoleWindow();
	conhostWindow = getConhostWindow();
	if (!conhostWindow) { error("The current console process is not \"conhost.exe\"!", "gl/glConsole.c", __LINE__); }
	hdc = GetDC(conhostWindow);

	initWindow();
	initOpenGL();
	initCharset();
}

void refreshFont(void)
{
	static int oldFontW = -1, oldFontH = -1;

	CONSOLE_SCREEN_BUFFER_INFOEX consoleBufferInfo;
	consoleBufferInfo.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
	GetConsoleScreenBufferInfoEx(outputHandle, &consoleBufferInfo);

	int realConW = consoleBufferInfo.srWindow.Right - consoleBufferInfo.srWindow.Left + 1;
	int realConH = consoleBufferInfo.srWindow.Bottom - consoleBufferInfo.srWindow.Top + 1;

	RECT clientRect = { 0 };
	GetClientRect(conHWND, &clientRect);

	fontW = (int)round((double)clientRect.right / (double)realConW);
	fontH = (int)round((double)clientRect.bottom / (double)realConH);

	if (fontW != oldFontW || fontH != oldFontH)
	{
		EnumFontFamiliesA(hdc, "Terminal", &enumFontFamProc, 0);
		if (glCharW == 0.0f) { loadCharsetSize(); }
		reqRefreshFont = true;

		oldFontW = fontW;
		oldFontH = fontH;
	}
}

void drawWithOpenGL(GlConsoleChar* output, int w, int h)
{
	const double BG_COLOR_R = 12.0 / 255.0;
	const double BG_COLOR_G = 12.0 / 255.0;
	const double BG_COLOR_B = 12.0 / 255.0;

	static bool drawCalled = false;
	static int scanline = 0;
	
	if (!drawCalled)
	{
		wglMakeCurrent(hdc, hglrc);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		initShaders();
		drawCalled = true;
	}

	if (reqRefreshFont)
	{
		loadCharset();
		reqRefreshFont = false;
	}

	setMatrix();
	refreshArrays(w, h);

	if (!settings.disableCLS && settings.scanlineCount == 1)
	{
		glClearColor(BG_COLOR_R, BG_COLOR_G, BG_COLOR_B, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	for (int i = 0; i < CHARACTER_COUNT; i++)
	{
		charset[i].count = 0;
	}

	float y = 0.0f;
	for (int i = 0; i < h; i++)
	{
		if (settings.scanlineCount != 1)
		{
			if (((i % (settings.scanlineCount * settings.scanlineHeight))
					/ settings.scanlineHeight) != scanline)
			{
				y += glCharH;
				continue;
			}
			else if (!settings.disableCLS)
			{
				glBindTexture(GL_TEXTURE_2D, 0);
				glColor3d(BG_COLOR_R, BG_COLOR_G, BG_COLOR_B);

				glBegin(GL_QUADS);
				glVertex2f(0.0f, y);
				glVertex2f(glCharW * w, y);
				glVertex2f(glCharW * w, y + glCharH);
				glVertex2f(0.0f, y + glCharH);
				glEnd();
			}
		}

		float x = 0.0f;
		for (int j = 0; j < w; j++)
		{
			GlConsoleChar character = output[i * w + j];
			GlCharType* charType = &charset[character.ch];
			makeArraysAvailable(charType, w, h);

			int pos = charType->count;

			charType->vertexArray[pos * 8] = x;
			charType->vertexArray[(pos * 8) + 1] = y;
			charType->vertexArray[(pos * 8) + 2] = x + glCharW;
			charType->vertexArray[(pos * 8) + 3] = y;
			charType->vertexArray[(pos * 8) + 4] = x + glCharW;
			charType->vertexArray[(pos * 8) + 5] = y + glCharH;
			charType->vertexArray[(pos * 8) + 6] = x;
			charType->vertexArray[(pos * 8) + 7] = y + glCharH;

			float r = (float)character.r / 255.0f;
			float g = (float)character.g / 255.0f;
			float b = (float)character.b / 255.0f;

			charType->colorArray[pos * 12] = r;
			charType->colorArray[(pos * 12) + 1] = g;
			charType->colorArray[(pos * 12) + 2] = b;
			charType->colorArray[(pos * 12) + 3] = r;
			charType->colorArray[(pos * 12) + 4] = g;
			charType->colorArray[(pos * 12) + 5] = b;
			charType->colorArray[(pos * 12) + 6] = r;
			charType->colorArray[(pos * 12) + 7] = g;
			charType->colorArray[(pos * 12) + 8] = b;
			charType->colorArray[(pos * 12) + 9] = r;
			charType->colorArray[(pos * 12) + 10] = g;
			charType->colorArray[(pos * 12) + 11] = b;

			charType->count++;
			charType->lastUsed = 0;
			x += glCharW;
		}
		y += glCharH;
	}

	scanline++;
	if (scanline == settings.scanlineCount) { scanline = 0; }

	for (int i = 0; i < CHARACTER_COUNT; i++)
	{
		if (!charset[i].count) { continue; }
		glBindTexture(GL_TEXTURE_2D, charset[i].texture);
		glVertexPointer(2, GL_FLOAT, 0, charset[i].vertexArray);
		glColorPointer(3, GL_FLOAT, 0, charset[i].colorArray);
		glTexCoordPointer(2, GL_FLOAT, 0, texCoordArray);
		glDrawArrays(GL_QUADS, 0, charset[i].count * 4);
	}

	glFlush();
}

void peekMessages(void)
{
	while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
	{
		DispatchMessageA(&msg);
	}
}

static HWND getConhostWindow(void)
{
	HWND consoleHWND = GetConsoleWindow();

	if (consoleHWND)
	{
		char className[32];
		GetClassNameA(consoleHWND, className, 32);

		if (!strcmp(className, "ConsoleWindowClass")) { return consoleHWND; }
		else { return NULL; }
	}

	return NULL;
}

static void initWindow(void)
{
	static const char* WND_NAME = "ConPlayer_glWindow";

	WNDCLASSEXA wcs = { 0 };
	wcs.cbSize = sizeof(WNDCLASSEXA);
	wcs.lpszClassName = WND_NAME;
	wcs.hInstance = GetModuleHandleA(NULL);
	wcs.hIcon = LoadIconA(NULL, IDI_APPLICATION);
	wcs.hIconSm = LoadIconA(NULL, IDI_APPLICATION);
	wcs.hCursor = LoadCursorA(NULL, IDC_ARROW);
	wcs.hbrBackground = CreateSolidBrush(RGB(12, 12, 12));
	wcs.lpfnWndProc = &windowProc;
	RegisterClassExA(&wcs);

	childWindow = CreateWindowExA(WS_EX_TRANSPARENT, WND_NAME,
		WND_NAME, WS_BORDER, 0, 0, 400, 300,
		NULL, NULL, GetModuleHandleA(NULL), NULL);

	SetWindowLongA(childWindow, GWL_STYLE, WS_CHILD);

	while (!hdc && GetMessageA(&msg, NULL, 0, 0))
	{
		DispatchMessageA(&msg);
	}
}

static void initOpenGL(void)
{
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	pfID = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, pfID, &pfd);
	hglrc = wglCreateContext(hdc);
}

static void initCharset(void)
{
	textDC = CreateCompatibleDC(hdc);
	charset = (GlCharType*)calloc(CHARACTER_COUNT, sizeof(GlCharType));

	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 32;
	bmpInfo.bmiHeader.biCompression = BI_RGB;

	bgBrush = CreateSolidBrush(RGB(12, 12, 12));

	EnumFontFamiliesA(hdc, NULL, &enumFontsProc, 0);
}

static int CALLBACK enumFontsProc(ENUMLOGFONT* lpelf, NEWTEXTMETRIC* lpntm, DWORD FontType, LPARAM lParam)
{
	char aaa[256];

	sprintf(aaa, "%s - %d x %d (%d)\n", lpelf->elfLogFont.lfFaceName, lpelf->elfLogFont.lfWidth,
		lpelf->elfLogFont.lfHeight, lpelf->elfLogFont.lfPitchAndFamily);

	OutputDebugStringA(aaa);
	return TRUE;
}

static int CALLBACK enumFontFamProc(ENUMLOGFONT* lpelf, NEWTEXTMETRIC* lpntm, DWORD FontType, LPARAM lParam)
{
	if (lpelf->elfLogFont.lfWidth == fontW &&
		lpelf->elfLogFont.lfHeight == fontH)
	{
		textFont = CreateFontIndirectA(lpelf);
	}
	return TRUE;
}

static void loadCharset(void)
{
	if (charsetLoaded)
	{
		glDeleteTextures(CHARACTER_COUNT, charset);
	}

	GLuint* textures = malloc(CHARACTER_COUNT * sizeof(GLuint));
	glGenTextures(CHARACTER_COUNT, textures);

	for (int i = 0; i < CHARACTER_COUNT; i++)
	{
		charset[i].texture = textures[i];
	}
	free(textures);

	char charToUse[] = "-";

	SetBkColor(textDC, RGB(12, 12, 12));
	SetTextColor(textDC, RGB(255, 255, 255));

	SelectObject(textDC, textFont);

	glCharW = 0.0f;
	glCharH = 0.0f;

	for (int i = 1; i < CHARACTER_COUNT; i++)
	{
		SIZE charSize;
		bool recalcTextureSize = false;
		charToUse[0] = (char)i;

		GetTextExtentPoint32A(textDC, charToUse, 1, &charSize);

		if ((float)charSize.cx > glCharW)
		{
			glCharW = (float)charSize.cx;
			recalcTextureSize = true;
		}

		if ((float)charSize.cy > glCharH)
		{
			glCharH = (float)charSize.cy;
			recalcTextureSize = true;
		}

		if (recalcTextureSize)
		{
			textureW = nextPowerOf2((int)glCharW);
			textureH = nextPowerOf2((int)glCharH);
			ratioW = glCharW / (float)textureW;
			ratioH = 1.0f - (glCharH / (float)textureH);
		}
	}

	for (int i = 1; i < CHARACTER_COUNT; i++)
	{
		charToUse[0] = (char)i;
		createCharacter(charToUse, charset[i].texture);
	}

	volGlCharW = glCharW;
	volGlCharH = glCharH;

	glBindTexture(GL_TEXTURE_2D, 0);
	charsetLoaded = true;
}

static void loadCharsetSize(void)
{
	char charToUse[] = "-";

	SetBkColor(textDC, RGB(12, 12, 12));
	SetTextColor(textDC, RGB(255, 255, 255));

	SelectObject(textDC, textFont);

	glCharW = 0.0f;
	glCharH = 0.0f;

	for (int i = 1; i < CHARACTER_COUNT; i++)
	{
		SIZE charSize;
		charToUse[0] = (char)i;

		GetTextExtentPoint32A(textDC, charToUse, 1, &charSize);

		if ((float)charSize.cx > glCharW)
		{
			glCharW = (float)charSize.cx;
		}

		if ((float)charSize.cy > glCharH)
		{
			glCharH = (float)charSize.cy;
		}
	}

	for (int i = 1; i < CHARACTER_COUNT; i++)
	{
		charToUse[0] = (char)i;
		createCharacter(charToUse, charset[i].texture);
	}

	volGlCharW = glCharW;
	volGlCharH = glCharH;
}

static void createCharacter(char* text, GLuint textureID)
{
	uint8_t* pixels = NULL;
	RECT textRect = { 0 };

	textRect.right = (LONG)glCharW;
	textRect.bottom = (LONG)glCharH;
	bmpInfo.bmiHeader.biWidth = textureW;
	bmpInfo.bmiHeader.biHeight = textureH;
	bmpInfo.bmiHeader.biSizeImage = textureW * textureH * 4;

	HBITMAP bitmap = CreateDIBSection(hdc, &bmpInfo, DIB_RGB_COLORS, &pixels, NULL, 0);
	DeleteObject(SelectObject(textDC, bitmap));

	FillRect(textDC, &textRect, bgBrush);
	DrawTextA(textDC, text, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	for (int i = 0; i < textureW * textureH; i++)
	{
		if (pixels[i * 4] != 12) { pixels[i * 4 + 3] = 255; }
		else { pixels[i * 4 + 3] = 0; }
	}

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureW, textureH,
		0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

static void setMatrix(void)
{
	static RECT oldConsoleRect = { 0 };
	RECT consoleRect;

	GetClientRect(conHWND, &consoleRect);

	if (consoleRect.right != oldConsoleRect.right ||
		consoleRect.bottom != oldConsoleRect.bottom)
	{
		SetWindowPos(childWindow, HWND_TOP, 0, 0,
			consoleRect.right, consoleRect.bottom, SWP_SHOWWINDOW);

		glViewport(0, 0, consoleRect.right, consoleRect.bottom);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0f, (double)consoleRect.right, (double)consoleRect.bottom, 0.0f);
		glTranslatef(0.375f, 0.375f, 0.0f);

		shSetSize(consoleRect.right, consoleRect.bottom);

		oldConsoleRect = consoleRect;
	}
}

static void refreshArrays(int w, int h)
{
	const int LAST_USED_COUNTER = 256;

	static int oldW = -1;
	static int oldH = -1;
	static float oldRatioW = -1.0f;
	static float oldRatioH = -1.0f;

	if (w != oldW || h != oldH)
	{
		for (int i = 0; i < CHARACTER_COUNT; i++)
		{
			if (charset[i].available)
			{
				free(charset[i].vertexArray);
				free(charset[i].colorArray);
				charset[i].vertexArray = malloc(w * h * 4 * 2 * sizeof(float));
				charset[i].colorArray = malloc(w * h * 4 * 3 * sizeof(float));
			}
		}

		free(texCoordArray);
		texCoordArray = malloc(w * h * 4 * 2 * sizeof(float));

		for (int i = 0; i < w * h; i++)
		{
			texCoordArray[i * 8] = 0.0f;
			texCoordArray[(i * 8) + 1] = 1.0f;
			texCoordArray[(i * 8) + 2] = ratioW;
			texCoordArray[(i * 8) + 3] = 1.0f;
			texCoordArray[(i * 8) + 4] = ratioW;
			texCoordArray[(i * 8) + 5] = ratioH;
			texCoordArray[(i * 8) + 6] = 0.0f;
			texCoordArray[(i * 8) + 7] = ratioH;
		}

		oldW = w;
		oldH = h;
		oldRatioW = ratioW;
		oldRatioH = ratioH;
	}
	else if (ratioW != oldRatioW || ratioH != oldRatioH)
	{
		for (int i = 0; i < w * h; i++)
		{
			texCoordArray[(i * 8) + 2] = ratioW;
			texCoordArray[(i * 8) + 4] = ratioW;
			texCoordArray[(i * 8) + 5] = ratioH;
			texCoordArray[(i * 8) + 7] = ratioH;
		}

		oldRatioW = ratioW;
		oldRatioH = ratioH;
	}

	for (int i = 0; i < CHARACTER_COUNT; i++)
	{
		if (charset[i].available && charset[i].lastUsed > LAST_USED_COUNTER)
		{
			free(charset[i].vertexArray);
			free(charset[i].colorArray);
			charset[i].available = false;
		}
		charset[i].lastUsed++;
	}
}

static void makeArraysAvailable(GlCharType* charType, int w, int h)
{
	if (!charType->available)
	{
		charType->vertexArray = malloc(w * h * 4 * 2 * sizeof(float));
		charType->colorArray = malloc(w * h * 4 * 3 * sizeof(float));
		charType->available = true;
	}
}

static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_CREATE)
	{
		SetParent(hwnd, conHWND);
		hdc = GetDC(hwnd);
	}
	else if (msg == WM_SIZE)
	{
		ShowWindow(hwnd, SW_SHOWMAXIMIZED);
	}

	return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static int nextPowerOf2(int n)
{
	int retVal = 1;
	while (retVal < n) { retVal <<= 1; }
	return retVal;
}

#else

volatile float volGlCharW = 0.0f, volGlCharH = 0.0f;

void initOpenGlConsole(void) {}
void refreshFont(void) {}
void drawWithOpenGL(GlConsoleChar* output, int w, int h) {}
void peekMessages(void) {}

#endif