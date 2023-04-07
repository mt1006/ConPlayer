#include "../../conplayer.h"

#ifndef CP_DISABLE_OPENGL

typedef struct
{
	char* code;
	int stage;
	ShaderType shaderType;
	ShaderPartType partType;
	char* data;
} ShaderPart;

typedef struct
{
	char* name;
	float value;
	int stage;
} UniformValue;

typedef struct
{
	char* name;
	ShaderPart shader;
} PredefinedShader;


// OpenGL 2.0 functions

// https://registry.khronos.org/OpenGL/api/GL/glext.h
#define CP_GL_FRAGMENT_SHADER     0x8B30
#define CP_GL_VERTEX_SHADER       0x8B31
#define CP_GL_COMPILE_STATUS      0x8B81
#define CP_GL_LINK_STATUS         0x8B82
#define CP_GL_INFO_LOG_LENGTH     0x8B84

typedef GLuint(APIENTRY T_glCreateShader)(GLenum type);
typedef void(APIENTRY T_glShaderSource)(GLuint shader, GLsizei count, const char* const* string, const GLint* length);
typedef void(APIENTRY T_glCompileShader)(GLuint shader);
typedef GLuint(APIENTRY T_glCreateProgram)(void);
typedef void(APIENTRY T_glAttachShader)(GLuint program, GLuint shader);
typedef void(APIENTRY T_glLinkProgram)(GLuint program);
typedef void(APIENTRY T_glUseProgram)(GLuint program);
typedef GLint(APIENTRY T_glGetUniformLocation)(GLuint program, const char* name);
typedef void(APIENTRY T_glUniform1f)(GLint location, GLfloat v0);
typedef void(APIENTRY T_glUniform2i)(GLint location, GLint v0, GLint v1);
typedef void(APIENTRY T_glGetShaderiv)(GLuint shader, GLenum pname, GLint* params);
typedef void(APIENTRY T_glGetProgramiv)(GLuint program, GLenum pname, GLint* params);
typedef void(APIENTRY T_glGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog);
typedef void(APIENTRY T_glGetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog);

static T_glCreateShader* f_glCreateShader;
static T_glShaderSource* f_glShaderSource;
static T_glCompileShader* f_glCompileShader;
static T_glCreateProgram* f_glCreateProgram;
static T_glAttachShader* f_glAttachShader;
static T_glLinkProgram* f_glLinkProgram;
static T_glUseProgram* f_glUseProgram;
static T_glGetUniformLocation* f_glGetUniformLocation;
static T_glUniform1f* f_glUniform1f;
static T_glUniform2i* f_glUniform2i;
static T_glGetShaderiv* f_glGetShaderiv;
static T_glGetProgramiv* f_glGetProgramiv;
static T_glGetShaderInfoLog* f_glGetShaderInfoLog;
static T_glGetProgramInfoLog* f_glGetProgramInfoLog;

static void loadGlFunctions(void);
static void* loadGlFunction(const char* name);

static void loadGlFunctions(void)
{
	f_glCreateShader = loadGlFunction("glCreateShader");
	f_glShaderSource = loadGlFunction("glShaderSource");
	f_glCompileShader = loadGlFunction("glCompileShader");
	f_glCreateProgram = loadGlFunction("glCreateProgram");
	f_glAttachShader = loadGlFunction("glAttachShader");
	f_glLinkProgram = loadGlFunction("glLinkProgram");
	f_glUseProgram = loadGlFunction("glUseProgram");
	f_glGetUniformLocation = loadGlFunction("glGetUniformLocation");
	f_glUniform1f = loadGlFunction("glUniform1f");
	f_glUniform2i = loadGlFunction("glUniform2i");
	f_glGetShaderiv = loadGlFunction("glGetShaderiv");
	f_glGetProgramiv = loadGlFunction("glGetProgramiv");
	f_glGetShaderInfoLog = loadGlFunction("glGetShaderInfoLog");
	f_glGetProgramInfoLog = loadGlFunction("glGetProgramInfoLog");
}

// https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions
static void* loadGlFunction(const char* name)
{
	void* p = (void*)wglGetProcAddress(name);

	if (p == NULL || (p == (void*)0x1) || (p == (void*)0x2) ||
		(p == (void*)0x3) || (p == (void*)-1))
	{
		HMODULE module = LoadLibraryA("opengl32.dll");
		if (module) { p = (void*)GetProcAddress(module, name); }
	}

	if (!p)
	{
		printf("Failed to load OpenGL function: %s\n\n", name);
		error("Failed to load OpenGL function!", "gl/glShaders.c", __LINE__);
	}
	return p;
}



// Shader code

const char* vshBegin[3] =
{
	NULL,


	NULL,


	"#version 330 compatibility\n\n"

	"out vec4 cp_color;\n"
	"out vec2 cp_pixelPos;\n"
	"uniform ivec2 cp_charPos;\n"
	"vec2 cp_texCoord;\n\n"

	"uniform float uni_lerp_a = 0.0;\n"
	"uniform float uni_lerp_b = 1.0;\n\n"
};

const char* vshMainBegin[3] =
{
	NULL,

	NULL,


	"void main()\n"
	"{\n"
	"	cp_color = gl_Color;\n"
	"	gl_Position = ftransform();\n"
	"	cp_pixelPos = vec2((gl_Position.x + 1.0) / 2.0, ((-gl_Position.y + 1.0) / 2.0));\n"
	"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"
	"	cp_texCoord = gl_TexCoord[0].xy;\n\n"
};

const char* vshMainEnd[3] =
{
	NULL,


	NULL,


	"	gl_Position.x = (cp_pixelPos.x * 2.0) - 1.0;\n"
	"	gl_Position.y = -((cp_pixelPos.y * 2.0) - 1.0);\n"
	"	gl_TexCoord[0].xy = cp_texCoord;\n"
	"}"
};

const char* fshBegin[3] =
{
	NULL,

	NULL,


	"#version 330 compatibility\n\n"

	"uniform sampler2D cp_colorMap;\n\n"
	"out vec4 cp_fragColor;\n"

	"in vec4 cp_color;\n"
	"in vec2 cp_pixelPos;\n"
	"uniform ivec2 cp_charPos;\n"
	"vec4 cp_texColor;\n\n"

	"uniform float uni_lerp_a = 0.0;\n"
	"uniform float uni_lerp_b = 1.0;\n\n"
};

const char* fshMainBegin[3] =
{
	NULL,


	NULL,


	"void main()\n"
	"{\n"
	"	cp_texColor = texture2D(cp_colorMap, gl_TexCoord[0].xy);\n"
};

const char* fshMainMiddle[3] =
{
	NULL,


	NULL,


	"	cp_fragColor = vec4(mix(uni_lerp_a, uni_lerp_b, cp_texColor.r) * cp_color.r,\n"
	"						mix(uni_lerp_a, uni_lerp_b, cp_texColor.g) * cp_color.g,\n"
	"						mix(uni_lerp_a, uni_lerp_b, cp_texColor.b) * cp_color.b, 1.0f);\n"
};

const char* fshMainEnd[3] =
{
	NULL,


	NULL,


	"}"
};

const PredefinedShader PREDEFS[] =
{
	{"dither", {
		"uniform float uni_dither_str = 0.6;\n"
		"uniform float uni_dither_rc = 2.0;\n"
		"uniform float uni_dither_gc = 2.0;\n"
		"uniform float uni_dither_bc = 2.0;\n\n"
		
		"const int cp_bayer8[8 * 8] = int[](\n"
		"	0,  32, 8,  40, 2,  34, 10, 42,\n"
		"	48, 16, 56, 24, 50, 18, 58, 26,\n"
		"	12, 44, 4,  36, 14, 46, 6,  38,\n"
		"	60, 28, 52, 20, 62, 30, 54, 22,\n"
		"	3,  35, 11, 43, 1,  33, 9,  41,\n"
		"	51, 19, 59, 27, 49, 17, 57, 25,\n"
		"	15, 47, 7,  39, 13, 45, 5,  37,\n"
		"	63, 31, 55, 23, 61, 29, 53, 21 );\n\n"
		
		"float cp_getBayer8(int x, int y)\n"
		"{\n"
		"	return float(cp_bayer8[(x % 8) + (y % 8) * 8]) * (1.0 / 64.0) - 0.5;\n"
		"}\n\n"
		
		"vec3 cp_dither(vec3 color)\n"
		"{\n"
		"	int x = int(cp_pixelPos.x * float(cp_charPos.x));\n"
		"	int y = int(cp_pixelPos.y * float(cp_charPos.y));\n"
		"	color += cp_getBayer8(x, y) * uni_dither_str;\n"
		"	color.r = floor((uni_dither_rc - 1.0) * color.r + 0.5) / (uni_dither_rc - 1.0f);\n"
		"	color.g = floor((uni_dither_gc - 1.0) * color.g + 0.5) / (uni_dither_gc - 1.0f);\n"
		"	color.b = floor((uni_dither_bc - 1.0) * color.b + 0.5) / (uni_dither_bc - 1.0f);\n"
		"	return color;\n"
		"}\n\n"
		
		"void cp_applyDither()\n"
		"{\n"
		"	cp_fragColor = vec4(cp_dither(cp_fragColor.rgb), 1.0);"
		"}\n\n",
		0, ST_FRAGMENT_SHADER_POST, SPT_COMPLEX, "cp_applyDither"}},

	{"mandelbrot", {
		"float cp_mandelbrot(float real, float imag, int limit)\n"
		"{\n"
		"	float zReal = real;\n"
		"	float zImag = imag;\n"
		"	for (int i = 0; i < limit; i++)\n"
		"	{\n"
		"		float r2 = zReal * zReal;\n"
		"		float i2 = zImag * zImag;\n"
		"		if (r2 + i2 > 4.0) { return float(i)/float(limit); }\n"
		"		zImag = 2.0 * zReal * zImag + imag;\n"
		"		zReal = r2 - i2 + real;\n"
		"	}\n"
		"	return 1.0f;\n"
		"}\n\n",
		0, ST_FRAGMENT_SHADER, SPT_UTILS, NULL}}
};

const int PREDEF_COUNT = sizeof(PREDEFS) / sizeof(PredefinedShader);



// Shader functions

static GLuint program = 0;

static bool initialize = false;
static bool vshInit[3] = { false, false, false };
static bool fshInit[3] = { false, false, false };

static ShaderPart* shaderParts = NULL;
static int shaderPartCount = 0;
static UniformValue* uniformValues = NULL;
static int uniformValueCount = 0;

static GLuint uniformCharPos = -1;


static char* buildShader(int stage, ShaderType type);
static void appendString(char** str, int* len, char* strToAppend);
static bool areSameType(ShaderType type1, ShaderType type2);
static GLint getUniformLocation(GLuint program, const char* prefix, const char* name);
static void checkShaderError(GLuint shader, const char* stage, const char* type);
static void checkProgramError(GLuint program, const char* stage);


void initShaders(void)
{
	if (!initialize) { return; }

	loadGlFunctions();

	program = f_glCreateProgram();

	if (vshInit[2] || fshInit[2])
	{
		char* vshCode = buildShader(3, ST_VERTEX_SHADER);
		char* fshCode = buildShader(3, ST_FRAGMENT_SHADER);
		printf(fshCode);

		GLuint vertexShader = f_glCreateShader(CP_GL_VERTEX_SHADER);
		f_glShaderSource(vertexShader, 1, &vshCode, NULL);
		f_glCompileShader(vertexShader);
		checkShaderError(vertexShader, "s3", "vsh");

		GLuint fragmentShader = f_glCreateShader(CP_GL_FRAGMENT_SHADER);
		f_glShaderSource(fragmentShader, 1, &fshCode, NULL);
		f_glCompileShader(fragmentShader);
		checkShaderError(fragmentShader, "s3", "fsh");

		f_glAttachShader(program, vertexShader);
		f_glAttachShader(program, fragmentShader);

		f_glLinkProgram(program);
		checkProgramError(program, "s3");

		free(fshCode);
		free(vshCode);
	}

	uniformCharPos = getUniformLocation(program, "cp_", "charPos");

	f_glUseProgram(program);

	//TODO: check stage
	for (int i = 0; i < uniformValueCount; i++)
	{
		f_glUniform1f(getUniformLocation(program, "uni_", uniformValues[i].name), uniformValues[i].value);
	}
}

void shAddShader(int stage, ShaderType type, ShaderPartType partType, char* shader, ShaderSource source, char* data)
{
	initialize = true;
	if (stage != 0)
	{
		if (type == ST_VERTEX_SHADER)
		{
			vshInit[stage - 1] = true;
		}
		else if (type == ST_FRAGMENT_SHADER ||
				 type == ST_FRAGMENT_SHADER_PRE ||
				 type == ST_FRAGMENT_SHADER_POST)
		{
			fshInit[stage - 1] = true;
		}
	}

	shaderPartCount++;
	shaderParts = realloc(shaderParts, shaderPartCount * sizeof(ShaderPart));

	if (source == SS_PREDEF)
	{
		int predefPos = -1;
		for (int i = 0; i < PREDEF_COUNT; i++)
		{
			if (!strcmp(PREDEFS[i].name, shader))
			{
				predefPos = i;
				break;
			}
		}

		if (predefPos == -1) { error("Failed to find predefined shader!", "gl/glShaders.c", __LINE__); }
		shaderParts[shaderPartCount - 1] = PREDEFS[predefPos].shader;

		if (PREDEFS[predefPos].shader.stage == 0)
		{
			shaderParts[shaderPartCount - 1].stage = stage;
		}
		else if (PREDEFS[predefPos].shader.stage != stage)
		{
			error("Unable to attach shader to this stage!", "gl/glShaders.c", __LINE__);
		}

		if (PREDEFS[predefPos].shader.shaderType == ST_VERTEX_SHADER && type != ST_VERTEX_SHADER)
		{
			error("Not corresponding shader type!", "gl/glShaders.c", __LINE__);
		}

		if (partType == SPT_UTILS) { shaderParts[shaderPartCount - 1].partType = SPT_UTILS; }
	}
	else
	{
		ShaderPart* newShaderPart = &shaderParts[shaderPartCount - 1];
		newShaderPart->stage = stage;
		newShaderPart->shaderType = type;
		newShaderPart->partType = partType;
		newShaderPart->data = data;

		if (source == SS_FILE)
		{
			FILE* file = fopen(shader, "r");
			if (!file) { error("Failed to open shader file!", "gl/glShaders.c", __LINE__); }

			fseek(file, 0, SEEK_END);
			int len = ftell(file);
			fseek(file, 0, SEEK_SET);

			char* code = (char*)calloc(len, 1);
			fread(code, sizeof(char), len, file);
			fclose(file);

			newShaderPart->code = code;
		}
		else
		{
			newShaderPart->code = strdup(shader);
		}
	}
}

void shAddUniform(int stage, char* name, float val)
{
	uniformValueCount++;
	uniformValues = realloc(uniformValues, uniformValueCount * sizeof(UniformValue));

	UniformValue* newUniformValue = &uniformValues[uniformValueCount - 1];
	newUniformValue->name = strdup(name);
	newUniformValue->value = val;
	newUniformValue->stage = stage;
}



void shSetSize(int w, int h)
{
	if (uniformCharPos != -1)
	{
		f_glUniform2i(uniformCharPos, w, h);
	}
}

static char* buildShader(int stage, ShaderType type)
{
	bool found = false;
	ShaderPart* fullShader = NULL;

	for (int i = 0; i < shaderPartCount; i++)
	{
		if (shaderParts[i].stage == stage && areSameType(shaderParts[i].shaderType, type))
		{
			if ((shaderParts[i].partType == SPT_FULL && found) || fullShader)
			{
				error("Trying to build full shader with other partial shaders!", "gl/glShaders.c", __LINE__);
			}

			if (shaderParts[i].partType == SPT_FULL) { fullShader = &shaderParts[i]; }
			found = true;
		}
	}

	if (fullShader) { return strdup(fullShader->code); }

	char* str = strdup("");
	int len = 0;

	if (type == ST_VERTEX_SHADER) { appendString(&str, &len, vshBegin[stage - 1]); }
	else { appendString(&str, &len, fshBegin[stage - 1]); }

	for (int i = 0; i < shaderPartCount; i++)
	{
		if (shaderParts[i].stage == stage && areSameType(shaderParts[i].shaderType, type) &&
			(shaderParts[i].partType == SPT_COMPLEX || shaderParts[i].partType == SPT_UTILS))
		{
			appendString(&str, &len, shaderParts[i].code);
			appendString(&str, &len, "\n\n");
		}
	}

	if (type == ST_VERTEX_SHADER)
	{
		appendString(&str, &len, vshMainBegin[stage - 1]);

		for (int i = 0; i < shaderPartCount; i++)
		{
			if (shaderParts[i].stage == stage && shaderParts[i].shaderType == ST_VERTEX_SHADER)
			{
				if (shaderParts[i].partType == SPT_SIMPLE)
				{
					appendString(&str, &len, shaderParts[i].code);
					appendString(&str, &len, "\n\n");
				}
				else if (shaderParts[i].partType == SPT_COMPLEX)
				{
					appendString(&str, &len, shaderParts[i].data);
					appendString(&str, &len, "();\n\n");
				}
			}
		}

		appendString(&str, &len, vshMainEnd[stage - 1]);
	}
	else
	{
		appendString(&str, &len, fshMainBegin[stage - 1]);

		for (int i = 0; i < shaderPartCount; i++)
		{
			if (shaderParts[i].stage == stage && shaderParts[i].shaderType == ST_FRAGMENT_SHADER_PRE)
			{
				if (shaderParts[i].partType == SPT_SIMPLE)
				{
					appendString(&str, &len, shaderParts[i].code);
					appendString(&str, &len, "\n\n");
				}
				else if (shaderParts[i].partType == SPT_COMPLEX)
				{
					appendString(&str, &len, shaderParts[i].data);
					appendString(&str, &len, "();\n\n");
				}
			}
		}

		appendString(&str, &len, fshMainMiddle[stage - 1]);

		for (int i = 0; i < shaderPartCount; i++)
		{
			if (shaderParts[i].stage == stage &&
				(shaderParts[i].shaderType == ST_FRAGMENT_SHADER_POST ||
					shaderParts[i].shaderType == ST_FRAGMENT_SHADER))
			{
				if (shaderParts[i].partType == SPT_SIMPLE)
				{
					appendString(&str, &len, shaderParts[i].code);
					appendString(&str, &len, "\n\n");
				}
				else if (shaderParts[i].partType == SPT_COMPLEX)
				{
					appendString(&str, &len, shaderParts[i].data);
					appendString(&str, &len, "();\n\n");
				}
			}
		}

		appendString(&str, &len, fshMainEnd[stage - 1]);
	}

	return str;
}

static void appendString(char** str, int* len, char* strToAppend)
{
	*len = *len + strlen(strToAppend);
	*str = realloc(*str, (*len) + 1);
	strcat(*str, strToAppend);
}

static bool areSameType(ShaderType type1, ShaderType type2)
{
	if (type1 == ST_FRAGMENT_SHADER_POST) { type1 = ST_FRAGMENT_SHADER; }
	if (type1 == ST_FRAGMENT_SHADER_PRE) { type1 = ST_FRAGMENT_SHADER; }

	if (type2 == ST_FRAGMENT_SHADER_POST) { type2 = ST_FRAGMENT_SHADER; }
	if (type2 == ST_FRAGMENT_SHADER_PRE) { type2 = ST_FRAGMENT_SHADER; }

	return type1 == type2;
}

static GLint getUniformLocation(GLuint program, const char* prefix, const char* name)
{
	int len = strlen(prefix) + strlen(name) + 1;
	char* fullName = (char*)malloc(len * sizeof(char));
	strcpy(fullName, prefix);
	strcat(fullName, name);

	GLint location = f_glGetUniformLocation(program, fullName);
	// If not used, uniform can be removed so it isn't checking for error (-1)

	free(fullName);
	return location;
}

static void checkShaderError(GLuint shader, const char* stage, const char* type)
{
	GLint isCompiled = 0;
	f_glGetShaderiv(shader, CP_GL_COMPILE_STATUS, &isCompiled);

	if (isCompiled == GL_FALSE)
	{
		GLint len = 0;
		f_glGetShaderiv(shader, CP_GL_INFO_LOG_LENGTH, &len);

		if (len != 0)
		{
			char* infoLog = (char*)malloc(len * sizeof(char));
			f_glGetShaderInfoLog(shader, len, &len, infoLog);
			printf("\nShader compilation error [%s:%s]:\n", stage, type);
			puts(infoLog, stdout);
		}

		error("Failed to compile shader!", "gl/glShaders.c", __LINE__);
	}
}

static void checkProgramError(GLuint program, const char* stage)
{
	GLint isLinked = 0;
	f_glGetProgramiv(program, CP_GL_LINK_STATUS, &isLinked);

	if (isLinked == GL_FALSE)
	{
		GLint len = 0;
		f_glGetProgramiv(program, CP_GL_INFO_LOG_LENGTH, &len);

		if (len != 0)
		{
			char* infoLog = (char*)malloc(len * sizeof(char));
			f_glGetProgramInfoLog(program, len, &len, infoLog);
			printf("\nProgram linking error [%s]:\n", stage);
			puts(infoLog, stdout);
		}

		error("Failed to link program!", "gl/glShaders.c", __LINE__);
	}
}

#else

void initShaders(void) {}
void shAddShader(int stage, ShaderType type, ShaderPartType partType, char* shader, ShaderSource source, char* data) {}
void shAddUniform(int stage, char* name, float val) {}
void shSetSize(int w, int h) {}

#endif