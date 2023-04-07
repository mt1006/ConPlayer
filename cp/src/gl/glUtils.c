#include "../conplayer.h"

#ifndef CP_DISABLE_OPENGL

static bool mainWindowExists = false, isMainWindow = false;

static HWND createGlWindow(int stage);
static void createGlContext(HDC hdc, HGLRC* hglrc, bool drawToBitmap);
static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static int nextPowerOf2(int n);

GlWindow initGlWindow(GlWindowType type, int stage)
{
	GlWindow glw;

	if (type == GLWT_MAIN || type == GLWT_MAIN_CHILD)
	{
		if (mainWindowExists) { error("Main window already created!", "gl/glUtils.c", __LINE__); }
		mainWindowExists = true;
		isMainWindow = true;
	}

	glw.hwnd = createGlWindow(stage);
	glw.hdc = GetDC(glw.hwnd);

	isMainWindow = false;

	if (type == GLWT_MAIN_CHILD) { SetWindowLongA(glw.hwnd, GWL_STYLE, WS_CHILD); }
	else if (type == GLWT_DUMMY) { ShowWindow(glw.hwnd, SW_HIDE); }

	createGlContext(glw.hdc, &glw.hglrc, type == GLWT_DUMMY);
	return glw;
}

void peekWindowMessages(GlWindow* glw)
{
	while (PeekMessageA(&glw->msg, NULL, 0, 0, PM_REMOVE))
	{
		DispatchMessageA(&glw->msg);
	}
}

GLuint createTexture(void)
{
	GLuint id;
	glGenTextures(1, &id);
	return id;
}

void setTexture(uint8_t* data, int w, int h, int format, GLuint id)
{
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void calcTextureSize(int w, int h, int* outW, int* outH, float* ratioW, float* ratioH)
{
	*outW = nextPowerOf2(w);
	*outH = nextPowerOf2(h);
	*ratioW = w / (float)(*outW);
	*ratioH = 1.0f - (h / (float)(*outH));
}

static HWND createGlWindow(int stage)
{
	char className[64];
	sprintf(className, "ConPlayer_glWindow_stage%d", stage);

	WNDCLASSEXA wcs = { 0 };
	wcs.cbSize = sizeof(WNDCLASSEXA);
	wcs.lpszClassName = className;
	wcs.hInstance = GetModuleHandleA(NULL);
	wcs.hIcon = LoadIconA(NULL, IDI_APPLICATION);
	wcs.hIconSm = LoadIconA(NULL, IDI_APPLICATION);
	wcs.hCursor = LoadCursorA(NULL, IDC_ARROW);
	wcs.hbrBackground = CreateSolidBrush(RGB(12, 12, 12));
	wcs.lpfnWndProc = &windowProc;
	RegisterClassExA(&wcs);

	return CreateWindowExA(WS_EX_TRANSPARENT, className, className,
		WS_BORDER, 0, 0, 320, 240, NULL, NULL, GetModuleHandleA(NULL), NULL);
}

static void createGlContext(HDC hdc, HGLRC* hglrc, bool drawToBitmap)
{
	PIXELFORMATDESCRIPTOR pfd =
	{
		.nSize = sizeof(PIXELFORMATDESCRIPTOR),
		.nVersion = 1,
		.dwFlags = PFD_SUPPORT_OPENGL | (drawToBitmap ? PFD_DRAW_TO_BITMAP : PFD_DRAW_TO_WINDOW),
		.iPixelType = PFD_TYPE_RGBA,
		.cColorBits = 32,
		.cDepthBits = 24,
		.cStencilBits = 8,
		.iLayerType = PFD_MAIN_PLANE
	};

	int pfID = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, pfID, &pfd);
	*hglrc = wglCreateContext(hdc);
}

static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (isMainWindow)
	{
		if (msg == WM_CREATE) { SetParent(hwnd, conHWND); }
		else if (msg == WM_SIZE) { ShowWindow(hwnd, SW_SHOWMAXIMIZED); }
	}
	return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static int nextPowerOf2(int n)
{
	int retVal = 1;
	while (retVal < n) { retVal <<= 1; }
	return retVal;
}

#endif