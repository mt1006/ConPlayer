#include "../conplayer.h"

#ifndef CP_DISABLE_OPENGL

static bool mainWindowExists = false, isMainWindow = false;

static HWND createGlWindow(int stage);
static void createGlContext(HDC hdc, HGLRC* hglrc);
static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void copyRgbFrameToParts(AVFrame* frame, GlImagePart* part);
static int nextPowerOf2(int n);

GlWindow initGlWindow(GlWindowType type, int stage)
{
	GlWindow glw = { 0 };

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

	createGlContext(glw.hdc, &glw.hglrc);
	return glw;
}

void peekWindowMessages(GlWindow* glw)
{
	while (PeekMessageA(&glw->msg, NULL, 0, 0, PM_REMOVE))
	{
		DispatchMessageA(&glw->msg);
	}
}

uint8_t* getGlBitmap(GlWindow* glw, int w, int h)
{
	if (glw->bmpW != w || glw->bmpH == h)
	{
		free(glw->bitmap);
		glw->bitmap = malloc(w * h * 4);
		glw->bmpW = w;
		glw->bmpH = h;

		glf[0].renderbufferStorage(CP_GL_RENDERBUFFER, GL_RGBA, w, h);
	}

	glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, glw->bitmap);
	return glw->bitmap;
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
	glBindTexture(GL_TEXTURE_2D, 0);
}

void setPixelMatrix(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, (double)w, (double)h, 0.0);
	glTranslatef(0.375f, 0.375f, 0.0f);
}

void getImageParts(AVFrame* frame, GlImageParts* parts, int maxTextureSize)
{
	int partCountX = ((frame->width - 1) / maxTextureSize) + 1;
	int partCountY = ((frame->height - 1) / maxTextureSize) + 1;
	int partCount = partCountX * partCountY;

	if (partCount != parts->count && parts->count != 0)
	{
		for (int i = 0; i < parts->count; i++)
		{
			free(parts->parts[i].data);
			glDeleteTextures(1, parts->parts[i].texture);
		}
		free(parts->parts);
		parts->count = 0;
	}

	if (partCount == 0) { return; }

	if (parts->count == 0)
	{
		parts->count = partCount;
		parts->parts = calloc(partCount, sizeof(GlImagePart));

		for (int i = 0; i < partCount; i++)
		{
			parts->parts[i].texture = createTexture();
		}
	}

	parts->w = frame->width;
	parts->h = frame->height;

	for (int i = 0; i < partCountY; i++)
	{
		for (int j = 0; j < partCountX; j++)
		{
			GlImagePart* part = &parts->parts[(i * partCountX) + j];

			part->x = j * maxTextureSize;
			part->y = i * maxTextureSize;
			part->w = (j == partCountX - 1) ? (frame->width % maxTextureSize) : maxTextureSize;
			part->h = (i == partCountY - 1) ? (frame->height % maxTextureSize) : maxTextureSize;

			calcTextureSize(part->w, part->h, &part->texW, &part->texH, &part->texRatioW, &part->texRatioH, false);
			copyRgbFrameToParts(frame, part);
			setTexture(part->data, part->texW, part->texH, GL_RGB, part->texture);
		}
	}
}

void calcTextureSize(int w, int h, int* outW, int* outH, float* ratioW, float* ratioH, bool invertH)
{
	*outW = nextPowerOf2(w);
	*outH = nextPowerOf2(h);
	*ratioW = w / (float)(*outW);
	*ratioH = h / (float)(*outH);
	if (invertH) { *ratioH = 1.0f - (*ratioH); }
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

static void createGlContext(HDC hdc, HGLRC* hglrc)
{
	PIXELFORMATDESCRIPTOR pfd =
	{
		.nSize = sizeof(PIXELFORMATDESCRIPTOR),
		.nVersion = 1,
		.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER,
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

static void copyRgbFrameToParts(AVFrame* frame, GlImagePart* part)
{
	if (part->texW != part->dataW || part->texH != part->dataH)
	{
		free(part->data);
		part->data = calloc(part->texW * part->texH * 3, 1);
		part->dataW = part->texW;
		part->dataH = part->texH;
	}

	if (part->x + part->w > frame->width || part->y + part->h > frame->height)
	{
		error("Parts size doesn't match frame size!", "gl/glUtils.c", __LINE__);
	}

	for (int i = 0; i < part->h; i++)
	{
		for (int j = 0; j < part->w; j++)
		{
			int x = j + part->x;
			int y = i + part->y;
			part->data[(i * part->texW + j) * 3] = frame->data[0][(y * frame->linesize[0]) + (x * 3)];
			part->data[(i * part->texW + j) * 3 + 1] = frame->data[0][(y * frame->linesize[0]) + (x * 3) + 1];
			part->data[(i * part->texW + j) * 3 + 2] = frame->data[0][(y * frame->linesize[0]) + (x * 3) + 2];
		}
	}
}

static int nextPowerOf2(int n)
{
	int retVal = 1;
	while (retVal < n) { retVal <<= 1; }
	return retVal;
}

#endif