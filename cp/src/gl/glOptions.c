#include "../conplayer.h"

#ifndef CP_DISABLE_OPENGL

static int opInit(void);
static int opStage(int argc, char** argv, int stage);
static int opStageShader(int argc, char** argv, int stage, ShaderType type);
static int opStageShaderAdd(int argc, char** argv, int stage, ShaderType type, bool isFile);
static int opStageSet(int argc, char** argv, int stage);
static int opWindow(int argc, char** argv);
static int opWindowType(int argc, char** argv);
static int opFont(int argc, char** argv);
static int opEnumFonts(int argc, char** argv);
static int opEnumFont(int argc, char** argv);
static void checkArgs(int argc, char** argv, int required, int line);
static void invalidInput(char* description, char* input, int line);
static void notEnoughArguments(char** argv, int line);
static void unknownOption(char** argv, int line);

int parseGlOptions(int argc, char** argv)
{
	int orgArgc = argc;
	while (argc)
	{
		char* str = argv[0];
		if (str[0] != ':') { break; }

		int retVal = 0;
		if (!strcmp(str, ":init")) { retVal = opInit(); }
		else if (!strncmp(str, ":s1", 3)) { retVal = opStage(argc, argv, 1); }
		else if (!strncmp(str, ":s2", 3)) { retVal = opStage(argc, argv, 2); }
		else if (!strncmp(str, ":s3", 3)) { retVal = opStage(argc, argv, 3); }
		else if (!strncmp(str, ":win", 4)) { retVal = opWindow(argc, argv); }
		else if (!strncmp(str, ":font", 5)) { retVal = opFont(argc, argv); }
		else if (!strncmp(str, ":enum-fonts", 11)) { retVal = opEnumFonts(argc, argv); }
		else if (!strncmp(str, ":enum-font", 10)) { retVal = opEnumFont(argc, argv); }
		else { unknownOption(argv, __LINE__); }

		argc -= retVal + 1;
		argv += retVal + 1;
	}
	return orgArgc - argc;
}

static int opInit()
{
	//TODO: init
	return 0;
}

static int opStage(int argc, char** argv, int stage)
{
	char* str = argv[0] + 3;

	if (!strncmp(str, ":vsh", 4)) { return opStageShader(argc, argv, stage, ST_VERTEX_SHADER); }
	else if (!strncmp(str, ":fsh", 4)) { return opStageShader(argc, argv, stage, ST_FRAGMENT_SHADER); }
	else if (!strcmp(str, ":set")) { return opStageSet(argc, argv, stage); }
	else { unknownOption(argv, __LINE__); }

	return 0;
}

static int opStageShader(int argc, char** argv, int stage, ShaderType type)
{
	char* str = argv[0] + 7;

	if (!strncmp(str, ":add-file", 9)) { return opStageShaderAdd(argc, argv, stage, type, true); }
	else if (!strncmp(str, ":add", 4)) { return opStageShaderAdd(argc, argv, stage, type, false); }
	else { unknownOption(argv, __LINE__); }

	return 0;
}

static int opStageShaderAdd(int argc, char** argv, int stage, ShaderType type, bool isFile)
{
	checkArgs(argc, argv, 2, __LINE__);

	char* str = argv[1];
	ShaderSource source = isFile ? SS_FILE : SS_ARGUMENT;

	if (type == ST_FRAGMENT_SHADER)
	{
		if (!strcmp(str, "simple"))
		{
			shAddShader(stage, ST_FRAGMENT_SHADER, SPT_SIMPLE, argv[2], source, NULL);
			return 2;
		}
		else if (!strcmp(str, "simple:post"))
		{
			shAddShader(stage, ST_FRAGMENT_SHADER_POST, SPT_SIMPLE, argv[2], source, NULL);
			return 2;
		}
		else if (!strcmp(str, "complex"))
		{
			checkArgs(argc, argv, 3, __LINE__);
			shAddShader(stage, ST_FRAGMENT_SHADER, SPT_COMPLEX, argv[3], source, argv[2]);
			return 3;
		}
		else if (!strcmp(str, "complex:post"))
		{
			checkArgs(argc, argv, 3, __LINE__);
			shAddShader(stage, ST_FRAGMENT_SHADER_POST, SPT_COMPLEX, argv[3], source, argv[2]);
			return 3;
		}
	}
	else
	{
		if (!strcmp(str, "complex:post") || !strcmp(str, "complex:post"))
		{
			invalidInput("Specifying subtype is not allowed for vertex shaders", str, __LINE__);
		}

		if (!strcmp(str, "simple"))
		{
			shAddShader(stage, type, SPT_SIMPLE, argv[2], source, NULL);
			return 2;
		}
		else if (!strcmp(str, "complex"))
		{
			checkArgs(argc, argv, 3, __LINE__);
			shAddShader(stage, type, SPT_COMPLEX, argv[3], source, argv[2]);
			return 3;
		}
	}

	if (!strcmp(str, "utils"))
	{
		shAddShader(stage, type, SPT_UTILS, argv[2], source, NULL);
		return 2;
	}
	else if (!strcmp(str, "full"))
	{
		shAddShader(stage, type, SPT_FULL, argv[2], source, NULL);
		return 2;
	}
	else if (!strcmp(str, "predef"))
	{
		if (isFile) { invalidInput("It isn't possible to load predefined shader from file", NULL, __LINE__); }
		shAddShader(stage, type, SPT_NONE, argv[2], SS_PREDEF, NULL);
		return 2;
	}
	else if (!strcmp(str, "predef:utils"))
	{
		if (isFile) { invalidInput("It isn't possible to load predefined shader from file", NULL, __LINE__); }
		shAddShader(stage, type, SPT_UTILS, argv[2], SS_PREDEF, NULL);
		return 2;
	}

	invalidInput("Unknown shader type", str, __LINE__);
	return 0;
}

static int opStageSet(int argc, char** argv, int stage)
{
	checkArgs(argc, argv, 2, __LINE__);
	shAddUniform(stage, argv[1], (float)atof(argv[2]));
	return 2;
}

static int opWindow(int argc, char** argv)
{
	char* str = argv[0] + 4;

	if (!strncmp(str, ":type", 5)) { return opWindowType(argc, argv); }
	else { unknownOption(argv, __LINE__); }

	return 0;
}

static int opWindowType(int argc, char** argv)
{
	checkArgs(argc, argv, 1, __LINE__);
	char* str = argv[1];

	if (!strcmp(str, "default")) { glWindowSetType = GLWT_DUMMY; }
	else if (!strcmp(str, "child")) { glWindowSetType = GLWT_MAIN_CHILD; }
	else if (!strcmp(str, "normal")) { glWindowSetType = GLWT_MAIN; }
	else { unknownOption(argv, __LINE__); }

	return 1;
}

static int opFont(int argc, char** argv)
{
	checkArgs(argc, argv, 3, __LINE__);

	int wideLen = MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, NULL, 0);
	LPCWSTR wstr = (char*)malloc(wideLen * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, argv[1], -1, wstr, wideLen);

	glCustomFontName = wstr;
	glCustomFontW = atoi(argv[2]);
	glCustomFontH = atoi(argv[3]);
	return 3;
}

static int opEnumFonts(int argc, char** argv)
{
	if (glEnumFonts) { error("Font enumeration already enabled!", "glOptions.c", __LINE__); }
	glEnumFonts = true;
	return 0;
}

static int opEnumFont(int argc, char** argv)
{
	checkArgs(argc, argv, 1, __LINE__);
	if (glEnumFonts) { error("Font enumeration already enabled!", "glOptions.c", __LINE__); }

	glEnumFonts = true;
	glEnumFontFamily = strdup(argv[1]);

	return 1;
}

static void checkArgs(int argc, char** argv, int required, int line)
{
	if (argc <= required) { notEnoughArguments(argv, line); }
	for (int i = 1; i < required; i++)
	{
		if (argv[i][0] == '-' || argv[i][0] == ':')
		{
			notEnoughArguments(argv, line);
		}
	}
}

static void invalidInput(char* description, char* input, int line)
{
	if (description)
	{
		if (input) { printf("%s - \"%s\"\n", description, input); }
		else { printf("%s\n", description); }
	}
	error("Invalid input!", "gl/glOptions.c", line);
}

static void notEnoughArguments(char** argv, int line)
{
	invalidInput("Not enough arguments for the operation", argv ? argv[0] : NULL, line);
}

static void unknownOption(char** argv, int line)
{
	invalidInput("Unknown option", argv[0], line);
}

#endif