#include "../../conplayer.h"

#ifndef CP_DISABLE_OPENGL

static const char* VSH_BEGIN =
"#version 330 compatibility\n\n"

"out vec2 cp_pixelPos;\n"
"vec2 cp_texCoord;\n\n";


static const char* VSH_MAIN_BEGIN =
"void main()\n"
"{\n"
"	gl_Position = ftransform();\n"
"	cp_pixelPos = vec2((gl_Position.x + 1.0) / 2.0, ((-gl_Position.y + 1.0) / 2.0));\n"
"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"	cp_texCoord = gl_TexCoord[0].xy;\n\n";


static const char* VSH_MAIN_END =
"	gl_Position.x = (cp_pixelPos.x * 2.0) - 1.0;\n"
"	gl_Position.y = -((cp_pixelPos.y * 2.0) - 1.0);\n"
"	gl_TexCoord[0].xy = cp_texCoord;\n"
"}";


static const char* FSH_BEGIN =
"#version 330 compatibility\n\n"

"uniform sampler2D cp_colorMap;\n\n"
"out vec4 cp_fragColor;\n"
"in vec2 cp_pixelPos;\n\n";


static const char* FSH_MAIN_BEGIN =
"void main()\n"
"{\n"
"	cp_fragColor = texture2D(cp_colorMap, gl_TexCoord[0].xy);\n";


static const char* FSH_MAIN_END =
"}";


bool shStage1_enabled = false;

static GlWindow glw;
static GLuint texture;


static uint8_t* getBitmap(HDC hdc, AVFrame* videoFrame);


void shStage1_init(void)
{
	glw = initGlWindow(GLWT_DUMMY, 1);
	texture = createTexture();
	shStage1_enabled = true;
}

void shStage1_apply(AVFrame* videoFrame)
{
	uint8_t* bitmap = getBitmap(glw.hdc, videoFrame);
	//setTexture(videoFrame->buf, )
}

static uint8_t* getBitmap(HDC hdc, AVFrame* videoFrame)
{
	static int oldW = -1, oldH = -1;
	static uint8_t oldBitmap = NULL;

	int w = videoFrame->width, h = videoFrame->height;

	if (oldW == w && oldH == h) { return oldBitmap; }
	if (!w || !h) { return NULL; }

	unsigned char* bmpArray;
	BITMAPINFO bmpInfo;
	RECT tempRect = { 0,0,w,h };

	memset(&bmpInfo, 0, sizeof(BITMAPINFO));
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 32;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	bmpInfo.bmiHeader.biWidth = w;
	bmpInfo.bmiHeader.biHeight = h;
	bmpInfo.bmiHeader.biSizeImage = w * h * 3;

	HBITMAP bitmap = CreateDIBSection(hdc, &bmpInfo,
		DIB_RGB_COLORS, (void**)&bmpArray, 0, 0);
	HGDIOBJ oldObj = SelectObject(hdc, bitmap);
	if (oldObj) { DeleteObject(oldObj); }

	oldW = w;
	oldH = h;
	oldBitmap = bmpArray;

	return bmpArray;
}

#else

void shStage1_init(void) {}
void shStage1_apply(AVFrame* videoFrame) {}

#endif