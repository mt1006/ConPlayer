#include "../../conplayer.h"

#ifndef CP_DISABLE_OPENGL

static ShadersStructure structure =
{
	//vshBegin
	"#version 330 compatibility\n\n"

	"out vec2 cp_pixelPos;\n"
	"vec2 cp_texCoord;\n\n",


	//vshMainBegin
	"void main()\n"
	"{\n"
	"	gl_Position = ftransform();\n"
	"	cp_pixelPos = vec2((gl_Position.x + 1.0) / 2.0, ((-gl_Position.y + 1.0) / 2.0));\n"
	"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"
	"	cp_texCoord = gl_TexCoord[0].xy;\n\n",


	//vshMainEnd
	"	gl_Position.x = (cp_pixelPos.x * 2.0) - 1.0;\n"
	"	gl_Position.y = -((cp_pixelPos.y * 2.0) - 1.0);\n"
	"	gl_TexCoord[0].xy = cp_texCoord;\n"
	"}",


	//fshBegin
	"#version 330 compatibility\n\n"

	"uniform sampler2D cp_colorMap;\n\n"
	"out vec4 cp_fragColor;\n"
	"in vec2 cp_pixelPos;\n\n",


	//fshMainBegin
	"void main()\n"
	"{\n"
	"	cp_fragColor = texture2D(cp_colorMap, gl_TexCoord[0].xy);\n",


	//fshMainMiddle
	NULL,


	//fshMainEnd
	"}"
};


static GlWindow glw;
static GlImageParts parts;

void shStage1_init(void)
{
	if (!shStageEnabled[0]) { return; }

	glw = initGlWindow(GLWT_DUMMY, 1);
	wglMakeCurrent(glw.hdc, glw.hglrc);
	HGLRC aaa = wglGetCurrentContext();
	void* p = (void*)wglGetProcAddress("glCompileShader");
	glEnable(GL_TEXTURE_2D);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &glw.maxTextureSize);

	shLoadStageShaders(1, &structure);
	
	GLuint fbo, rbo;
	glf[0].genFramebuffers(1, &fbo);
	glf[0].bindFramebuffer(CP_GL_FRAMEBUFFER, fbo);

	glf[0].genRenderbuffers(1, &rbo);
	glf[0].bindRenderbuffer(CP_GL_RENDERBUFFER, rbo);
	glf[0].framebufferRenderbuffer(CP_GL_FRAMEBUFFER, CP_GL_COLOR_ATTACHMENT0, CP_GL_RENDERBUFFER, rbo);
}

void shStage1_apply(AVFrame* videoFrame)
{
	if (!shStageEnabled[0]) { return; }

	static uint8_t* array = NULL;
	static int oldArrayW = -1, oldArrayH = -1;

	int frameW = videoFrame->width, frameH = videoFrame->height;
	int frameLinesize = videoFrame->linesize[0];
	uint8_t* frameData = videoFrame->data[0];

	uint8_t* bitmap = getGlBitmap(&glw, frameW, frameH);
	glViewport(0, 0, frameW, frameH);

	getImageParts(videoFrame, &parts, glw.maxTextureSize);

	for (int i = 0; i < parts.count; i++)
	{
		GlImagePart* part = &parts.parts[i];
		glBindTexture(GL_TEXTURE_2D, part->texture);
		glColor3f(1.0f, 1.0f, 1.0f);

		float x = (float)(((double)part->x / (double)parts.w) * 2.0 - 1.0);
		float y = -(float)(((double)part->y / (double)parts.h) * 2.0 - 1.0);
		float w = (float)(((double)part->w / (double)parts.w) * 2.0);
		float h = (float)(((double)part->h / (double)parts.h) * 2.0);

		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, part->texRatioH);
		glVertex2f(x, y - h);
		glTexCoord2f(part->texRatioW, part->texRatioH);
		glVertex2f(x + w, y - h);
		glTexCoord2f(part->texRatioW, 0.0f);
		glVertex2f(x + w, y);
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(x, y);
		glEnd();

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glFlush();

	for (int i = 0; i < frameH; i++)
	{
		int bmpI = frameH - i - 1;
		for (int j = 0; j < frameW; j++)
		{
			frameData[(i * frameLinesize) + (j * 3)] = bitmap[(j + bmpI * frameW) * 4];
			frameData[(i * frameLinesize) + (j * 3) + 1] = bitmap[(j + bmpI * frameW) * 4 + 1];
			frameData[(i * frameLinesize) + (j * 3) + 2] = bitmap[(j + bmpI * frameW) * 4 + 2];
		}
	}
}

#endif