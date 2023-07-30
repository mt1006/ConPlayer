#include "../../conplayer.h"

#ifndef CP_DISABLE_OPENGL

static ShadersStructure structure =
{
	//vshBegin
	"#version 330 compatibility\n\n"

	"out vec4 cp_color;\n"
	"out vec2 cp_pixelPos;\n"
	"uniform ivec2 cp_charPos;\n"
	"vec2 cp_texCoord;\n\n"

	"uniform float uni_lerp_a = 0.0;\n"
	"uniform float uni_lerp_b = 1.0;\n\n",


	//vshMainBegin
	"void main()\n"
	"{\n"
	"	cp_color = gl_Color;\n"
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

	"in vec4 cp_color;\n"
	"in vec2 cp_pixelPos;\n"
	"uniform ivec2 cp_charPos;\n"
	"vec4 cp_texColor;\n\n"

	"uniform float uni_lerp_a = 0.0;\n"
	"uniform float uni_lerp_b = 1.0;\n\n",


	//fshMainBegin
	"void main()\n"
	"{\n"
	"	cp_texColor = texture2D(cp_colorMap, gl_TexCoord[0].xy);\n",

	
	//fshMainMiddle
	"	cp_fragColor = vec4(mix(uni_lerp_a, uni_lerp_b, cp_texColor.r) * cp_color.r,\n"
	"						mix(uni_lerp_a, uni_lerp_b, cp_texColor.g) * cp_color.g,\n"
	"						mix(uni_lerp_a, uni_lerp_b, cp_texColor.b) * cp_color.b, 1.0f);\n",


	//fshMainEnd
	"}"
};


static GLuint uniformCharPos = -1;


void shStage3_init(void)
{
	if (!shStageEnabled[2]) { return; }
	GLuint program = shLoadStageShaders(3, &structure);

	uniformCharPos = shGetUniformLocation(&glf[2], program, "cp_", "charPos");
}

void shStage3_setSize(int w, int h)
{
	if (uniformCharPos != -1)
	{
		glf[2].uniform2i(uniformCharPos, w, h);
	}
}

#endif