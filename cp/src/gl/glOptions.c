#include "../conplayer.h"

#ifndef CP_DISABLE_OPENGL

static int opInit(void);
static int opStage(int argc, char** argv, int stage);
static int opStageShader(int argc, char** argv, int stage, ShaderType type);
static int opStageShaderAdd(int argc, char** argv, int stage, ShaderType type, bool isFile);
static int opStageSet(int argc, char** argv, int stage);
static bool checkArgs(int argc, char** argv, int required);
static void invalidInput(char* description, char* input, int line);
static void notEnoughArguments(char** argv, int line);
static void unknownOption(char** argv, int line);

int parseGlOptions(int argc, char** argv)
{
	int orgArgc = argc;
	while (argc)
	{
		char* str = *argv;
		if (str[0] != ':') { break; }

		int retVal = 0;
		if (!strcmp(str, ":init")) { retVal = opInit(); }
		else if (!strncmp(str, ":s1", 3)) { retVal = opStage(argc, argv, 1); }
		else if (!strncmp(str, ":s2", 3)) { retVal = opStage(argc, argv, 2); }
		else if (!strncmp(str, ":s3", 3)) { retVal = opStage(argc, argv, 3); }
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
	char* str = (*argv) + 3;

	if (!strncmp(str, ":vsh", 4)) { return opStageShader(argc, argv, stage, ST_VERTEX_SHADER); }
	else if (!strncmp(str, ":fsh", 4)) { return opStageShader(argc, argv, stage, ST_FRAGMENT_SHADER); }
	else if (!strcmp(str, ":set")) { return opStageSet(argc, argv, stage); }
	else { unknownOption(argv, __LINE__); }

	return 0;
}

static int opStageShader(int argc, char** argv, int stage, ShaderType type)
{
	char* str = (*argv) + 7;

	if (!strncmp(str, ":add", 4)) { return opStageShaderAdd(argc, argv, stage, type, false); }
	else if (!strncmp(str, ":add-file", 4)) { return opStageShaderAdd(argc, argv, stage, type, true); }
	else { unknownOption(argv, __LINE__); }

	return 0;
}

static int opStageShaderAdd(int argc, char** argv, int stage, ShaderType type, bool isFile)
{
	char* str = argv[1];

	ShaderSource source = isFile ? SS_FILE : SS_ARGUMENT;
	checkArgs(argc, argv, 2);

	if (type == ST_FRAGMENT_SHADER)
	{
		if (!strcmp(str, "simple") || !strcmp(str, "complex"))
		{
			invalidInput("Specify \":pre\" or \":post\" at the end of the type name", str, __LINE__);
		}

		if (!strcmp(str, "simple:pre"))
		{
			shAddShader(stage, ST_FRAGMENT_SHADER_PRE, SPT_SIMPLE, argv[2], source, NULL);
			return 2;
		}
		else if (!strcmp(str, "simple:post"))
		{
			shAddShader(stage, ST_FRAGMENT_SHADER_POST, SPT_SIMPLE, argv[2], source, NULL);
			return 2;
		}
		else if (!strcmp(str, "complex:pre"))
		{
			checkArgs(argc, argv, 3);
			shAddShader(stage, ST_FRAGMENT_SHADER_PRE, SPT_COMPLEX, argv[3], source, argv[2]);
			return 3;
		}
		else if (!strcmp(str, "complex:post"))
		{
			checkArgs(argc, argv, 3);
			shAddShader(stage, ST_FRAGMENT_SHADER_POST, SPT_COMPLEX, argv[3], source, argv[2]);
			return 3;
		}
	}
	else
	{
		if (!strcmp(str, "simple:pre") || !strcmp(str, "simple:post") ||
			!strcmp(str, "complex:post") || !strcmp(str, "complex:post"))
		{
			invalidInput("Specifying subtype is not necessary", str, __LINE__);
		}

		if (!strcmp(str, "simple"))
		{
			shAddShader(stage, type, SPT_SIMPLE, argv[2], source, NULL);
			return 2;
		}
		else if (!strcmp(str, "complex"))
		{
			checkArgs(argc, argv, 3);
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
	if (!checkArgs(argc, argv, 2)) { notEnoughArguments(argv, __LINE__); }
	shAddUniform(stage, argv[1], (float)atof(argv[2]));
	return 2;
}

static bool checkArgs(int argc, char** argv, int required)
{
	if (argc < required + 1) { return false; }
	for (int i = 1; i < required; i++)
	{
		if (argv[i][0] == '-' || argv[i][0] == ':') { return false; }
	}
	return true;
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

#else

int parseGlOptions(int argc, char** argv) { return 0; }

#endif