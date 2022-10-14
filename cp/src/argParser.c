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
	bool isOperation;
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
static int opSetColor(int argc, char** argv);
static int opCharset(int argc, char** argv);
static int opSingleChar(int argc, char** argv);
static int opRand(int argc, char** argv);
static int opScalingMode(int argc, char** argv);
static int opFontRatio(int argc, char** argv);
static int opSync(int argc, char** argv);
static int opVideoFilters(int argc, char** argv);
static int opScaledVideoFilters(int argc, char** argv);
static int opAudioFilters(int argc, char** argv);
static int opDisableCLS(int argc, char** argv);
static int opDisableAudio(int argc, char** argv);
static int opDisableKeys(int argc, char** argv);
static int opLibavLogs(int argc, char** argv);
static int opFullInfo(int argc, char** argv);
static void invalidSyntax(int line);

const Option OPTIONS[] = {
	{"-h","--help",&opHelp,true},
	{"-i","--input",&opInput,false},
	{"-c","--colors",&opColors,false},
	{"-vol","--volume",&opVolume,false},
	{"-s","--size",&opSize,false},
	{"-f","--fill",&opFill,false},
	{"-inf","--information",&opInformation,true},
	{"-v","--version",&opVersion,true},
	{"-int","--interlaced",&opInterlaced,false},
	{"-sc","--set-color",&opSetColor,false},
	{"-cs","--charset",&opCharset,false},
	{"-sch","--single-char",&opSingleChar,false},
	{"-r","--rand",&opRand,false},
	{"-sm","--scaling-mode",&opScalingMode,false},
	{"-fr","--font-ratio",&opFontRatio,false},
	{"-sy","--sync",&opSync,false},
	{"-vf","--video-filters",&opVideoFilters,false},
	{"-svf","--scaled-video-filters",&opScaledVideoFilters,false},
	{"-af","--audio-filters",&opAudioFilters,false},
	{"-dcls","--disable-cls",&opDisableCLS,false},
	{"-da","--disable-audio",&opDisableAudio,false},
	{"-dk","--disable-keys",&opDisableKeys,false},
	{"-avl","--libav-logs",&opLibavLogs,false},
	{"-fi","--full-info",&opFullInfo,true} };

static int optionCount;
static char* inputFile = NULL;

char* argumentParser(int argc, char** argv)
{
	optionCount = sizeof(OPTIONS) / sizeof(Option);
	bool* optionsUsed = calloc(optionCount, sizeof(bool));

	settings.charset = CHARSET_LONG;
	settings.charsetSize = (int)strlen(settings.charset);

	for (int i = 0; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			if (i == 0 && !strcmp(argv[0], "-?")) { argv[0][1] = 'h'; }

			bool optionFound = false;

			for (int j = 0; j < optionCount; j++)
			{
				if (!strcmp(argv[i], OPTIONS[j].name) ||
					!strcmp(argv[i], OPTIONS[j].fullName))
				{
					if (optionsUsed[j]) { invalidSyntax(__LINE__); }
					if (OPTIONS[j].isOperation && i != 0) { invalidSyntax(__LINE__); }

					i += OPTIONS[j].parserFunction(argc - i - 1, argv + i + 1);

					if (OPTIONS[j].isOperation)
					{
						if (i != argc - 1) { invalidSyntax(__LINE__); }
						return NULL;
					}

					optionsUsed[j] = true;
					optionFound = true;
					break;
				}
			}

			if (!optionFound) { invalidSyntax(__LINE__); }
		}
		else if (i == 0)
		{
			if (!strcmp(argv[0], "/?"))
			{
				argv[0][0] = '-';
				argv[0][1] = 'h';
				i--;
			}
			else
			{
				opInput(argc, argv);
				optionsUsed[1] = 1;
			}
		}
		else
		{
			invalidSyntax(__LINE__);
		}
	}

	free(optionsUsed);

	if (inputFile == NULL) { invalidSyntax(__LINE__); }
	checkSettings();
	return inputFile;
}

static void checkSettings(void)
{
	if (settings.colorMode == CM_WINAPI_GRAY ||
		settings.colorMode == CM_WINAPI_16)
	{
		#ifndef _WIN32
		error("WinAPI color mode not supported on Linux!", "argParser.c", __LINE__);
		#endif
	}

	if (settings.setColorMode == SCM_WINAPI)
	{
		#ifndef _WIN32
		error("WinAPI \"set color\" mode not supported on Linux!", "argParser.c", __LINE__);
		#endif
	}

	if (settings.setColorMode == SCM_WINAPI &&
		settings.colorMode != CM_WINAPI_GRAY &&
		settings.colorMode != CM_CSTD_GRAY)
	{
		error("\"Set color\" works only with grayscale color mode!", "argParser.c", __LINE__);
	}

	if (settings.setColorMode == SCM_WINAPI &&
		settings.colorMode != CM_WINAPI_GRAY &&
		settings.colorMode != CM_CSTD_GRAY)
	{
		error("WinAPI \"set color\" mode mode works only with grayscale color mode!", "argParser.c", __LINE__);
	}

	if ((settings.setColorMode == SCM_CSTD_256 ||
		settings.setColorMode == SCM_CSTD_RGB) &&
		settings.colorMode != CM_CSTD_GRAY)
	{
		error("C std \"set color\" mode works only with \"cstd-gray\" color mode!", "argParser.c", __LINE__);
	}

	if (settings.singleCharMode &&
		(settings.colorMode == CM_WINAPI_GRAY ||
			settings.colorMode == CM_CSTD_GRAY))
	{
		error("Single character mode requires colors!", "argParser.c", __LINE__);
	}

	if (settings.singleCharMode && settings.brightnessRand)
	{
		if (settings.brightnessRand < 0) { settings.brightnessRand = -settings.brightnessRand; }
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
	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }

	strToLower(argv[0]);
	if (!strcmp(argv[0], "winapi-gray")) { settings.colorMode = CM_WINAPI_GRAY; }
	else if (!strcmp(argv[0], "winapi-16")) { settings.colorMode = CM_WINAPI_16; }
	else if (!strcmp(argv[0], "cstd-gray")) { settings.colorMode = CM_CSTD_GRAY; }
	else if (!strcmp(argv[0], "cstd-16")) { settings.colorMode = CM_CSTD_16; }
	else if (!strcmp(argv[0], "cstd-256")) { settings.colorMode = CM_CSTD_256; }
	else if (!strcmp(argv[0], "cstd-rgb")) { settings.colorMode = CM_CSTD_RGB; }
	else { error("Invalid color mode!", "argParser.c", __LINE__); }

	return 1;
}

static int opVolume(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }
	settings.volume = atof(argv[0]);
	if (settings.volume < 0.0 || settings.volume > 1.0) { error("Invalid volume!", "argParser.c", __LINE__); }
	return 1;
}

static int opSize(int argc, char** argv)
{
	if (argc < 2 || argv[0][0] == '-' || argv[1][0] == '-') { invalidSyntax(__LINE__); }
	settings.argW = atoi(argv[0]);
	settings.argH = atoi(argv[1]);
	if ((settings.argW != 0 || settings.argH != 0) && (settings.argW < 4 || settings.argH < 4))
	{
		error("Invalid size!", "argParser.c", __LINE__);
	}
	return 2;
}

static int opFill(int argc, char** argv)
{
	settings.fillArea = true;
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
		if (!strcmp(argv[0], "basic"))              { showHelp(1, 0, 0, 0, 0); }
		else if (!strcmp(argv[0], "advanced"))      { showHelp(0, 1, 0, 0, 0); }
		else if (!strcmp(argv[0], "color-modes"))   { showHelp(0, 0, 1, 0, 0); }
		else if (!strcmp(argv[0], "scaling-modes")) { showHelp(0, 0, 0, 1, 0); }
		else if (!strcmp(argv[0], "keyboard"))      { showHelp(0, 0, 0, 0, 1); }
		else if (!strcmp(argv[0], "full"))          { showHelp(1, 1, 1, 1, 1); }
		else { error("Invalid help topic!", "argParser.c", __LINE__); }
		return 1;
	}
	else if (argc == 0)
	{
		showHelp(1, 0, 0, 0, 1);
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
		settings.scanlineHeight = atoi(argv[1]);
		if (settings.scanlineHeight < 1) { error("Invalid scanline height!", "argParser.c", __LINE__); }
		settings.scanlineCount = atoi(argv[0]);
		if (settings.scanlineCount < 1) { error("Invalid interlacing!", "argParser.c", __LINE__); }
		return 2;
	}
	else
	{
		settings.scanlineCount = atoi(argv[0]);
		if (settings.scanlineCount < 1) { error("Invalid interlacing!", "argParser.c", __LINE__); }
		return 1;
	}
}

static int opSetColor(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }

	if (argv[0][0] >= '0' && argv[0][0] <= '9')
	{
		settings.setColorMode = SCM_WINAPI;
		settings.setColorVal1 = atoi(argv[0]);
	}
	else if (argv[0][0] == '@')
	{
		settings.setColorMode = SCM_CSTD_256;
		settings.setColorVal1 = atoi(argv[0] + 1);
		if (settings.setColorVal1 < 0 || settings.setColorVal1 > 255)
		{
			error("Invalid color!", "argParser.c", __LINE__);
		}
		
		if (argc > 1 && argv[1][0] != '-')
		{
			settings.setColorVal2 = atoi(argv[1]);
			if (settings.setColorVal2 < 0 || settings.setColorVal2 > 255)
			{
				error("Invalid color!", "argParser.c", __LINE__);
			}
			return 2;
		}
	}
	else if (argv[0][0] == '#')
	{
		settings.setColorMode = SCM_CSTD_RGB;
		settings.setColorVal1 = strtol(argv[0] + 1, NULL, 16);
		if (settings.setColorVal1 < 0 || settings.setColorVal1 > 0xFFFFFF)
		{
			error("Invalid color!", "argParser.c", __LINE__);
		}

		if (argc > 1 && argv[1][0] != '-')
		{
			settings.setColorVal2 = strtol(argv[1], NULL, 16);
			if (settings.setColorVal2 < 0 || settings.setColorVal2 > 0xFFFFFF)
			{
				error("Invalid color!", "argParser.c", __LINE__);
			}
			return 2;
		}
	}
	else
	{
		invalidSyntax(__LINE__);
	}

	return 1;
}

static int opCharset(int argc, char** argv)
{
	const int CHARSET_MAX_SIZE = 256;

	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }
	if (argv[0][0] == '#')
	{
		strToLower(argv[0]);
		if (!strcmp(argv[0], "#long")) { settings.charset = CHARSET_LONG; }
		else if (!strcmp(argv[0], "#short")) { settings.charset = CHARSET_SHORT; }
		else if (!strcmp(argv[0], "#2")) { settings.charset = CHARSET_2; }
		else if (!strcmp(argv[0], "#blocks")) { settings.charset = CHARSET_BLOCKS; }
		else if (!strcmp(argv[0], "#outline")) { settings.charset = CHARSET_OUTLINE; }
		else if (!strcmp(argv[0], "#bold-outline")) { settings.charset = CHARSET_BOLD_OUTLINE; }
		else { error("Invalid predefined charset name!", "argParser.c", __LINE__); }
		settings.charsetSize = (int)strlen(settings.charset);
	}
	else
	{
		FILE* charsetFile = fopen(argv[0], "rb");
		if (!charsetFile) { error("Failed to open charset file!", "argParser.c", __LINE__); }
		settings.charset = (char*)malloc(CHARSET_MAX_SIZE * sizeof(char));
		settings.charsetSize = (int)fread(settings.charset, sizeof(char), CHARSET_MAX_SIZE, charsetFile);
		fclose(charsetFile);
	}
	return 1;
}

static int opSingleChar(int argc, char** argv)
{
	settings.singleCharMode = true;
	return 0;
}

static int opRand(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }
	srand(time(NULL));

	if (argv[0][0] == '@') { settings.brightnessRand = -atoi(argv[0] + 1); }
	else { settings.brightnessRand = atoi(argv[0]); }

	if (settings.brightnessRand < -255 || settings.brightnessRand > 255) { error("Invalid rand value!", "argParser.c", __LINE__); }
	return 1;
}

static int opScalingMode(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }

	strToLower(argv[0]);
	if (!strcmp(argv[0], "nearest")) { settings.scalingMode = SWS_POINT; }
	else if (!strcmp(argv[0], "fast-bilinear")) { settings.scalingMode = SWS_FAST_BILINEAR; }
	else if (!strcmp(argv[0], "bilinear")) { settings.scalingMode = SWS_BILINEAR; }
	else if (!strcmp(argv[0], "bicubic")) { settings.scalingMode = SWS_BICUBIC; }
	else { error("Invalid scaling mode!", "argParser.c", __LINE__); }

	return 1;
}

static int opFontRatio(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }
	settings.constFontRatio = atof(argv[0]);
	if (settings.constFontRatio <= 0.0) { error("Invalid font ratio!", "argParser.c", __LINE__); }
	return 1;
}

static int opSync(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }

	strToLower(argv[0]);
	if (!strcmp(argv[0], "disabled")) { settings.syncMode = SM_DISABLED; }
	else if (!strcmp(argv[0], "draw-all")) { settings.syncMode = SM_DRAW_ALL; }
	else if (!strcmp(argv[0], "enabled")) { settings.syncMode = SM_ENABLED; }
	else { error("Invalid synchronization mode!", "argParser.c", __LINE__); }

	return 1;
}

static int opVideoFilters(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }
	settings.videoFilters = argv[0];
	return 1;
}

static int opScaledVideoFilters(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }
	settings.scaledVideoFilters = argv[0];
	return 1;
}

static int opAudioFilters(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { invalidSyntax(__LINE__); }
	settings.audioFilters = argv[0];
	return 1;
}

static int opDisableCLS(int argc, char** argv)
{
	settings.disableCLS = true;
	return 0;
}

static int opDisableAudio(int argc, char** argv)
{
	settings.disableAudio = true;
	return 0;
}

static int opDisableKeys(int argc, char** argv)
{
	settings.disableKeyboard = true;
	return 0;
}

static int opLibavLogs(int argc, char** argv)
{
	settings.libavLogs = true;
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