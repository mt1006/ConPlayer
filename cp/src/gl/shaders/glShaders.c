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


GlFunctions glf[4];


static void loadGlFunctions(int stage);
static void* loadGlFunction(const char* name);


static void loadGlFunctions(int stage)
{
	GlFunctions* f = &glf[stage - 1];
	f->createShader = loadGlFunction("glCreateShader");
	f->shaderSource = loadGlFunction("glShaderSource");
	f->compileShader = loadGlFunction("glCompileShader");
	f->createProgram = loadGlFunction("glCreateProgram");
	f->attachShader = loadGlFunction("glAttachShader");
	f->linkProgram = loadGlFunction("glLinkProgram");
	f->useProgram = loadGlFunction("glUseProgram");
	f->getUniformLocation = loadGlFunction("glGetUniformLocation");
	f->uniform1f = loadGlFunction("glUniform1f");
	f->uniform2i = loadGlFunction("glUniform2i");
	f->getShaderiv = loadGlFunction("glGetShaderiv");
	f->getProgramiv = loadGlFunction("glGetProgramiv");
	f->getShaderInfoLog = loadGlFunction("glGetShaderInfoLog");
	f->getProgramInfoLog = loadGlFunction("glGetProgramInfoLog");

	f->genFramebuffers = loadGlFunction("glGenFramebuffers");
	f->bindFramebuffer = loadGlFunction("glBindFramebuffer");
	f->genRenderbuffers = loadGlFunction("glGenRenderbuffers");
	f->bindRenderbuffer = loadGlFunction("glBindRenderbuffer");
	f->renderbufferStorage = loadGlFunction("glRenderbufferStorage");
	f->framebufferRenderbuffer = loadGlFunction("glFramebufferRenderbuffer");
}

// https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions
static void* loadGlFunction(const char* name)
{
	void* p = (void*)wglGetProcAddress(name);
	DWORD aaa = GetLastError();

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



// Predefs code 

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

bool shStageEnabled[4] = { false, false, false, false };

static ShaderPart* shaderParts = NULL;
static int shaderPartCount = 0;
static UniformValue* uniformValues = NULL;
static int uniformValueCount = 0;


static char* buildShader(int stage, ShaderType type, ShadersStructure* structure);
static void appendPart(char** str, int* len, ShaderPart* part, bool inMain);
static void appendString(char** str, int* len, const char* strToAppend);
static bool areSameType(ShaderType type1, ShaderType type2);
static void checkShaderError(GlFunctions* f, GLuint shader, int stage, const char* type);
static void checkProgramError(GlFunctions* f, GLuint program, int stage);


GLuint shLoadStageShaders(int stage, ShadersStructure* structure)
{
	loadGlFunctions(stage);
	GlFunctions* f = &glf[stage - 1];

	GLuint program = f->createProgram();

	char* vshCode = buildShader(stage, ST_VERTEX_SHADER, structure);
	char* fshCode = buildShader(stage, ST_FRAGMENT_SHADER, structure);
	printf(fshCode);

	GLuint vertexShader = f->createShader(CP_GL_VERTEX_SHADER);
	f->shaderSource(vertexShader, 1, &vshCode, NULL);
	f->compileShader(vertexShader);
	checkShaderError(f, vertexShader, stage, "vsh");

	GLuint fragmentShader = f->createShader(CP_GL_FRAGMENT_SHADER);
	f->shaderSource(fragmentShader, 1, &fshCode, NULL);
	f->compileShader(fragmentShader);
	checkShaderError(f, fragmentShader, stage, "fsh");

	f->attachShader(program, vertexShader);
	f->attachShader(program, fragmentShader);

	f->linkProgram(program);
	checkProgramError(f, program, stage);

	free(fshCode);
	free(vshCode);

	f->useProgram(program);

	for (int i = 0; i < uniformValueCount; i++)
	{
		if (uniformValues[i].stage == stage)
		{
			f->uniform1f(shGetUniformLocation(f, program, "uni_", uniformValues[i].name), uniformValues[i].value);
		}
	}
	return program;
}

void shAddShader(int stage, ShaderType type, ShaderPartType partType, char* shader, ShaderSource source, char* data)
{
	shStageEnabled[stage - 1] = true;

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

GLint shGetUniformLocation(GlFunctions* f, GLuint program, const char* prefix, const char* name)
{
	int len = strlen(prefix) + strlen(name) + 1;
	char* fullName = (char*)malloc(len);
	strcpy(fullName, prefix);
	strcat(fullName, name);

	GLint location = f->getUniformLocation(program, fullName);
	// If not used, uniform can be removed so it isn't checking for error (-1)

	free(fullName);
	return location;
}

static char* buildShader(int stage, ShaderType type, ShadersStructure* structure)
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

	if (type == ST_VERTEX_SHADER) { appendString(&str, &len, structure->vshBegin); }
	else { appendString(&str, &len, structure->fshBegin); }

	for (int i = 0; i < shaderPartCount; i++)
	{
		if (shaderParts[i].stage == stage && areSameType(shaderParts[i].shaderType, type))
		{
			appendPart(&str, &len, &shaderParts[i], false);
		}
	}

	if (type == ST_VERTEX_SHADER)
	{
		appendString(&str, &len, structure->vshMainBegin);

		for (int i = 0; i < shaderPartCount; i++)
		{
			if (shaderParts[i].stage == stage && shaderParts[i].shaderType == ST_VERTEX_SHADER)
			{
				appendPart(&str, &len, &shaderParts[i], true);
			}
		}

		appendString(&str, &len, structure->vshMainEnd);
	}
	else
	{
		appendString(&str, &len, structure->fshMainBegin);

		for (int i = 0; i < shaderPartCount; i++)
		{
			if (shaderParts[i].stage == stage && shaderParts[i].shaderType == ST_FRAGMENT_SHADER)
			{
				appendPart(&str, &len, &shaderParts[i], true);
			}
		}

		if (structure->fshMainMiddle) { appendString(&str, &len, structure->fshMainMiddle); }

		for (int i = 0; i < shaderPartCount; i++)
		{
			if (shaderParts[i].stage == stage && shaderParts[i].shaderType == ST_FRAGMENT_SHADER_POST)
			{
				appendPart(&str, &len, &shaderParts[i], true);
			}
		}

		appendString(&str, &len, structure->fshMainEnd);
	}

	return str;
}

static void appendPart(char** str, int* len, ShaderPart* part, bool inMain)
{
	if (inMain)
	{
		if (part->partType == SPT_SIMPLE)
		{
			appendString(str, len, part->code);
			appendString(str, len, "\n\n");
		}
		else if (part->partType == SPT_COMPLEX)
		{
			appendString(str, len, part->data);
			appendString(str, len, "();\n\n");
		}
	}
	else if (part->partType == SPT_COMPLEX || part->partType == SPT_UTILS)
	{
		appendString(str, len, part->code);
		appendString(str, len, "\n\n");
	}
}

static void appendString(char** str, int* len, const char* strToAppend)
{
	*len = *len + strlen(strToAppend);
	*str = realloc(*str, (*len) + 1);
	strcat(*str, strToAppend);
}

static bool areSameType(ShaderType type1, ShaderType type2)
{
	if (type1 == ST_FRAGMENT_SHADER_POST) { type1 = ST_FRAGMENT_SHADER; }
	if (type2 == ST_FRAGMENT_SHADER_POST) { type2 = ST_FRAGMENT_SHADER; }
	return type1 == type2;
}

static void checkShaderError(GlFunctions* f, GLuint shader, int stage, const char* type)
{
	GLint isCompiled = 0;
	f->getShaderiv(shader, CP_GL_COMPILE_STATUS, &isCompiled);

	if (isCompiled == GL_FALSE)
	{
		GLint len = 0;
		f->getShaderiv(shader, CP_GL_INFO_LOG_LENGTH, &len);

		if (len != 0)
		{
			char* infoLog = (char*)malloc(len * sizeof(char));
			f->getShaderInfoLog(shader, len, &len, infoLog);
			printf("\nShader compilation error [s%d:%s]:\n", stage, type);
			fputs(infoLog, stdout);
		}

		error("Failed to compile shader!", "gl/glShaders.c", __LINE__);
	}
}

static void checkProgramError(GlFunctions* f, GLuint program, int stage)
{
	GLint isLinked = 0;
	f->getProgramiv(program, CP_GL_LINK_STATUS, &isLinked);

	if (isLinked == GL_FALSE)
	{
		GLint len = 0;
		f->getProgramiv(program, CP_GL_INFO_LOG_LENGTH, &len);

		if (len != 0)
		{
			char* infoLog = (char*)malloc(len * sizeof(char));
			f->getProgramInfoLog(program, len, &len, infoLog);
			printf("\nProgram linking error [s%d]:\n", stage);
			fputs(infoLog, stdout);
		}

		error("Failed to link program!", "gl/glShaders.c", __LINE__);
	}
}

#endif