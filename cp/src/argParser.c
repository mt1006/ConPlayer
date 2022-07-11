#include "conplayer.h"

static const char* CHARSET_LONG = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B$@";
static const char* CHARSET_SHORT = " .-+*?#M&%@";
static const char* CHARSET_2 = " *";
static const char* CHARSET_BLOCKS = " \xB0\xB1\xB2\xDB";
static const char* CHARSET_OUTLINE = " @@@@@@@@@@@@@@@@ ";
static const char* CHARSET_BOLD_OUTLINE = " @@@@@@@@@@@@@@@@.";

typedef struct
{
	char* name;
	char* fullName;
	int (*parserFunction)(int, char**);
	int isOperation;
} Option;

static void checkSettings(void);
static int opHelp(int argc, char** argv);
static int opInput(int argc, char** argv);
static int opColors(int argc, char** argv);
static int opVolume(int argc, char** argv);
static int opSize(int argc, char** argv);
static int opFill(int argc, char** argv);
static int opInformation(int argc, char** argv);
static int opVersion(int argc, char** argv);
static int opInterlaced(int argc, char** argv);
static int opCharset(int argc, char** argv);
static int opSetColor(int argc, char** argv);
static int opFontRatio(int argc, char** argv);
static int opDisableCLS(int argc, char** argv);
static int opDisableSync(int argc, char** argv);
static int opDisableKeys(int argc, char** argv);
static int opFullInfo(int argc, char** argv);
static void invalidSyntax(int line);

const Option OPTIONS[] = {
	{"-h","--help",&opHelp,1},
	{"-i","--input",&opInput,0},
	{"-c","--colors",&opColors,0},
	{"-vol","--volume",&opVolume,0},
	{"-s","--size",&opSize,0},
	{"-f","--fill",&opFill,0},
	{"-inf","--information",&opInformation,1},
	{"-v","--version",&opVersion,1},
	{"-int","--interlaced",&opInterlaced,0},
	{"-cs","--charset",&opCharset,0},
	{"-sc","--set-color",&opSetColor,0},
	{"-fr","--font-ratio",&opFontRatio,0},
	{"-dcls","--disable-cls",&opDisableCLS,0},
	{"-ds","--disable-sync",&opDisableSync,0},
	{"-dk","--disable-keys",&opDisableKeys,0},
	{"-fi","--full-info",&opFullInfo,1} };

static int optionCount;
static char* inputFile = NULL;

char* argumentParser(int argc, unichar** argv)
{
	optionCount = sizeof(OPTIONS) / sizeof(Option);
	int* usedOptions = calloc(optionCount, sizeof(int));

	charset = CHARSET_LONG;
	charsetSize = (int)strlen(charset);

	char** input = (char**)malloc(argc * sizeof(char*));
	for (int i = 0; i < argc; i++)
	{
		input[i] = toUTF8(argv[i], (int)uc_strlen(argv[i]));
	}

	for (int i = 0; i < argc; i++)
	{
		if (input[i][0] == '-')
		{
			if (i == 0 && !strcmp(input[0], "-?")) { input[0][1] = 'h'; }

			int breaked = 0;
			for (int j = 0; j < optionCount; j++)
			{
				if (!strcmp(input[i], OPTIONS[j].name) ||
					!strcmp(input[i], OPTIONS[j].fullName))
				{
					if (usedOptions[j]) { invalidSyntax(__LINE__); }
					if (OPTIONS[j].isOperation && i != 0) { invalidSyntax(__LINE__); }

					i += OPTIONS[j].parserFunction(argc - i - 1, input + i + 1);

					if (OPTIONS[j].isOperation)
					{
						if (i != argc - 1) { invalidSyntax(__LINE__); }
						return NULL;
					}

					usedOptions[j] = 1;
					breaked = 1;
					break;
				}
			}

			if (!breaked) { invalidSyntax(__LINE__); }
		}
		else if (i == 0)
		{
			if (!strcmp(input[0], "/?"))
			{
				input[0][0] = '-';
				input[0][1] = 'h';
				i--;
			}
			else
			{
				opInput(argc, input);
				usedOptions[1] = 1;
			}
		}
		else
		{
			invalidSyntax(__LINE__);
		}
	}

	for (int i = 0; i < argc; i++)
	{
		free(input[i]);
	}
	free(input);
	free(usedOptions);

	if (inputFile == NULL) { invalidSyntax(__LINE__); }
	checkSettings();
	return inputFile;
}

static void checkSettings(void)
{
	if (colorMode == CM_WINAPI_GRAY ||
		colorMode == CM_WINAPI_16)
	{
		#ifndef _WIN32
		error("WinAPI color mode not supported on Linux!", "argParser.c", __LINE__);
		#endif
	}
	if (setColorMode == SCM_WINAPI)
	{
		#ifndef _WIN32
		error("WinAPI \"set color\" mode not supported on Linux!", "argParser.c", __LINE__);
		#endif
	}
	if (setColorMode == SCM_WINAPI &&
		colorMode != CM_WINAPI_GRAY &&
		colorMode != CM_CSTD_GRAY)
	{
		error("\"Set color\" works only with grayscale color mode!", "argParser.c", __LINE__);
	}
	if (setColorMode == SCM_WINAPI &&
		colorMode != CM_WINAPI_GRAY &&
		colorMode != CM_CSTD_GRAY)
	{
		error("WinAPI \"set color\" mode mode works only with grayscale color mode!", "argParser.c", __LINE__);
	}
	if ((setColorMode == SCM_CSTD_256 ||
		setColorMode == SCM_CSTD_RGB) &&
		colorMode != CM_CSTD_GRAY)
	{
		error("C std \"set color\" mode works only with \"cstd-gray\" color mode!", "argParser.c", __LINE__);
	}
}

static int opInput(int argc, char** argv)
{
	if (argc < 1) { invalidSyntax(__LINE__); }
	int strLen = strlen(argv[0]) + 1;
	inputFile = malloc(strLen * sizeof(char));
	memcpy(inputFile, argv[0], strLen * sizeof(char));
	return 1;
}

static int opColors(int argc, char** argv)
{
	if (argc > 0 && argv[0][0] != '-')
	{
		strToLower(argv[0]);
		if (!strcmp(argv[0], "winapi-gray")) { colorMode = CM_WINAPI_GRAY; }
		else if (!strcmp(argv[0], "winapi-16")) { colorMode = CM_WINAPI_16; }
		else if (!strcmp(argv[0], "cstd-gray")) { colorMode = CM_CSTD_GRAY; }
		else if (!strcmp(argv[0], "cstd-16")) { colorMode = CM_CSTD_16; }
		else if (!strcmp(argv[0], "cstd-256")) { colorMode = CM_CSTD_256; }
		else if (!strcmp(argv[0], "cstd-rgb")) { colorMode = CM_CSTD_RGB; }
		else { error("Invalid color mode!", "argParser.c", __LINE__); }
		return 1;
	}
	else
	{
		colorMode = CP_DEFAULT_COLOR_MODE_C;
		return 0;
	}
}

static int opVolume(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }
	volume = atof(argv[0]);
	if (volume < 0.0 || volume > 1.0) { error("Invalid volume!", "argParser.c", __LINE__); }
	return 1;
}

static int opSize(int argc, char** argv)
{
	if (argc < 2 || argv[0][0] == '-' || argv[1][0] == '-') { invalidSyntax(__LINE__); }
	argW = atoi(argv[0]);
	argH = atoi(argv[1]);
	if ((argW != 0 || argH != 0) && (argW < 4 || argH < 4))
	{
		error("Invalid size!", "argParser.c", __LINE__);
	}
	return 2;
}

static int opFill(int argc, char** argv)
{
	fillArea = 1;
	return 0;
}

static int opInformation(int argc, char** argv)
{
	if (argc > 0) { invalidSyntax(__LINE__); }
	showInfo();
	return 0;
}

static int opVersion(int argc, char** argv)
{
	if (argc > 0) { invalidSyntax(__LINE__); }
	showVersion();
	return 0;
}

static int opHelp(int argc, char** argv)
{
	if (argc == 1 && argv[0][0] != '-')
	{
		strToLower(argv[0]);
		if (!strcmp(argv[0], "basic")) { showHelp(1, 0, 0, 0); }
		else if (!strcmp(argv[0], "advanced")) { showHelp(0, 1, 0, 0); }
		else if (!strcmp(argv[0], "color-modes")) { showHelp(0, 0, 1, 0); }
		else if (!strcmp(argv[0], "keyboard")) { showHelp(0, 0, 0, 1); }
		else if (!strcmp(argv[0], "full")) { showHelp(1, 1, 1, 1); }
		else { error("Invalid help topic!", "argParser.c", __LINE__); }
		return 1;
	}
	else if (argc == 0)
	{
		showHelp(1, 0, 0, 1);
		puts("[To see full help use \"conpl -h full\"]");
	}
	else
	{
		invalidSyntax(__LINE__);
	}
	return 0;
}

static int opInterlaced(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }
	if (argc > 1 && argv[1][0] != '-')
	{
		scanlineHeight = atoi(argv[1]);
		if (scanlineHeight < 1) { error("Invalid scanline height!", "argParser.c", __LINE__); }
		scanlineCount = atoi(argv[0]);
		if (scanlineCount < 1) { error("Invalid interlacing!", "argParser.c", __LINE__); }
		return 2;
	}
	else
	{
		scanlineCount = atoi(argv[0]);
		if (scanlineCount < 1) { error("Invalid interlacing!", "argParser.c", __LINE__); }
		return 1;
	}
}

static int opCharset(int argc, char** argv)
{ 
	const int CHARSET_MAX_SIZE = 256;

	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }
	if (argv[0][0] == '#')
	{
		strToLower(argv[0]);
		if (!strcmp(argv[0], "#long")) { charset = CHARSET_LONG; }
		else if (!strcmp(argv[0], "#short")) { charset = CHARSET_SHORT; }
		else if (!strcmp(argv[0], "#2")) { charset = CHARSET_2; }
		else if (!strcmp(argv[0], "#blocks")) { charset = CHARSET_BLOCKS; }
		else if (!strcmp(argv[0], "#outline")) { charset = CHARSET_OUTLINE; }
		else if (!strcmp(argv[0], "#bold-outline")) { charset = CHARSET_BOLD_OUTLINE; }
		else { error("Invalid predefined charset name!", "argParser.c", __LINE__); }
		charsetSize = (int)strlen(charset);
	}
	else
	{
		FILE* charsetFile = fopen(argv[0], "rb");
		if (!charsetFile) { error("Failed to open charset file!", "argParser.c", __LINE__); }
		charset = (char*)malloc(CHARSET_MAX_SIZE * sizeof(char));
		charsetSize = (int)fread(charset, sizeof(char), CHARSET_MAX_SIZE, charsetFile);
		fclose(charsetFile);
	}
	return 1;
}

static int opSetColor(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }

	if (argv[0][0] >= '0' && argv[0][0] <= '9')
	{
		setColorMode = SCM_WINAPI;
		setColorVal = atoi(argv[0]);
	}
	else if (argv[0][0] == '$')
	{
		setColorMode = SCM_CSTD_256;
		setColorVal = atoi(argv[0] + 1);
	}
	else if (argv[0][0] == '#')
	{
		setColorMode = SCM_CSTD_RGB;
		setColorVal = strtol(argv[0] + 1, NULL, 16);
	}
	else
	{
		invalidSyntax(__LINE__);
	}

	return 1;
}

static int opFontRatio(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }
	constFontRatio = atof(argv[0]);
	if (constFontRatio <= 0.0) { error("Invalid font ratio!", "argParser.c", __LINE__); }
	return 1;
}

static int opDisableCLS(int argc, char** argv)
{
	disableCLS = 1;
	return 0;
}

static int opDisableSync(int argc, char** argv)
{
	disableSync = 1;
	return 0;
}

static int opDisableKeys(int argc, char** argv)
{
	disableKeyboard = 1;
	return 0;
}

static int opFullInfo(int argc, char** argv)
{
	if (argc > 0) { invalidSyntax(__LINE__); }
	showFullInfo();
	return 0;
}

static void invalidSyntax(int line)
{
	error("Invalid syntax!", "argParser.c", line);
}