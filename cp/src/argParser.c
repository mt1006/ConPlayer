#include "conplayer.h"

const char* CHARSET_LONG = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B$@";
const char* CHARSET_SHORT = " .-+*?#M&%@";
const char* CHARSET_2 = " *";
const char* CHARSET_BLOCKS = " \xB0\xB1\xB2\xDB";
const char* CHARSET_OUTLINE = " @@@@@@@@@@@@@@@@ ";
const char* CHARSET_BOLD_OUTLINE = " @@@@@@@@@@@@@@@@.";

static const char* EXTRACTOR_PREFIX_DEFAULT = "yt-dlp -f \"bv*[height<=%d]+ba/bv*[height<=%d]/wv*+ba/w\" --no-warnings --get-url";
static const char* EXTRACTOR_PREFIX_NO_MAX_H = "yt-dlp --no-warnings --get-url";
static const int EXTRACTOR_PREFIX_DEFAULT_MAX_H = 480;
static const char* EXTRACTOR_SUFFIX_DEFAULT = "2>&1";
static const int INPUT_MAX_URL = 16384;
static const int MAX_LINE_COUNT = 128;

#ifdef _WIN32
static const char* QUOTE_MARK = "\"";
#else
static const char* QUOTE_MARK = "'";
#endif

typedef struct
{
	char* name;
	char* fullName;
	int (*parserFunction)(int, char**);
	bool isOperation;
} Option;

typedef enum
{
	EPM_DEFAULT,
	EPM_MAX_H,
	EPM_CUSTOM
} ExtractorPrefixMode;

static void extractUrl(void);
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
static int opColorProc(int argc, char** argv);
static int opSetColor(int argc, char** argv);
static int opCharset(int argc, char** argv);
static int opRand(int argc, char** argv);
static int opScalingMode(int argc, char** argv);
static int opFontRatio(int argc, char** argv);
static int opSync(int argc, char** argv);
static int opVideoFilters(int argc, char** argv);
static int opScaledVideoFilters(int argc, char** argv);
static int opAudioFilters(int argc, char** argv);
static int opPreload(int argc, char** argv);
static int opFakeConsole(int argc, char** argv);
static int opOpenGlSettings(int argc, char** argv);
static int opExtractorMaxHeight(int argc, char** argv);
static int opExtractorPrefix(int argc, char** argv);
static int opExtractorSuffix(int argc, char** argv);
static int opDisableCLS(int argc, char** argv);
static int opDisableAudio(int argc, char** argv);
static int opDisableKeys(int argc, char** argv);
static int opLibavLogs(int argc, char** argv);
static int opFullInfo(int argc, char** argv);
static void invalidInput(char* description, char* input, int line);
static void tooManyArguments(char** argv, int line);
static void notEnoughArguments(char** argv, int line);
static void extractorModeCollision(int line);

static const Option OPTIONS[] = {
	{"-h","--help",&opHelp,true},
	{"-i","--input",&opInput,false},
	{"-c","--colors",&opColors,false},
	{"-vol","--volume",&opVolume,false},
	{"-s","--size",&opSize,false},
	{"-f","--fill",&opFill,false},
	{"-inf","--information",&opInformation,true},
	{"-v","--version",&opVersion,true},
	{"-int","--interlaced",&opInterlaced,false},
	{"-cp","--color-proc",&opColorProc,false},
	{"-sc","--set-color",&opSetColor,false},
	{"-cs","--charset",&opCharset,false},
	{"-r","--rand",&opRand,false},
	{"-sm","--scaling-mode",&opScalingMode,false},
	{"-fr","--font-ratio",&opFontRatio,false},
	{"-sy","--sync",&opSync,false},
	{"-vf","--video-filters",&opVideoFilters,false},
	{"-svf","--scaled-video-filters",&opScaledVideoFilters,false},
	{"-af","--audio-filters",&opAudioFilters,false},
	{"-pl","--preload",&opPreload,false},
	{"-fc","--fake-console",&opFakeConsole,false},
	{"-gls","--opengl-settings",&opOpenGlSettings,false},
	{"-xmh","--extractor-max-height",&opExtractorMaxHeight,false},
	{"-xp","--extractor-prefix",&opExtractorPrefix,false},
	{"-xs","--extractor-suffix",&opExtractorSuffix,false},
	{"-dcls","--disable-cls",&opDisableCLS,false},
	{"-da","--disable-audio",&opDisableAudio,false},
	{"-dk","--disable-keys",&opDisableKeys,false},
	{"-avl","--libav-logs",&opLibavLogs,false},
	{"-fi","--full-info",&opFullInfo,true} };

char* inputFile = NULL;
char* secondInputFile = NULL;
static const int DEFAULT_OPTION = 1;
static int OPTION_COUNT = sizeof(OPTIONS) / sizeof(Option);

static ExtractorPrefixMode extractorPrefixMode = EPM_DEFAULT;
static int extractorMaxH = 0;
static const char* extractorPrefix = NULL;
static const char* extractorSuffix = NULL;

void argumentParser(int argc, char** argv)
{
	bool* optionsUsed = calloc(OPTION_COUNT, sizeof(bool));

	settings.charset = CHARSET_LONG;
	settings.charsetSize = (int)strlen(settings.charset);

	for (int i = 0; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			if (i == 0 && !strcmp(argv[0], "-?")) { argv[0][1] = 'h'; }

			bool optionFound = false;

			for (int j = 0; j < OPTION_COUNT; j++)
			{
				if ((OPTIONS[j].name && !strcmp(argv[i], OPTIONS[j].name)) ||
					(OPTIONS[j].fullName && !strcmp(argv[i], OPTIONS[j].fullName)))
				{
					if (optionsUsed[j]) { invalidInput("Option already used", argv[i], __LINE__); }
					if (OPTIONS[j].isOperation && i != 0) { invalidInput("Executing operation with options", argv[i], __LINE__); }

					i += OPTIONS[j].parserFunction(argc - i - 1, argv + i + 1);
					if (OPTIONS[j].isOperation)
					{
						if (i != argc - 1) { invalidInput("Executing operation with options", argv[i], __LINE__); }
						return;
					}

					optionsUsed[j] = true;
					optionFound = true;
					break;
				}
			}

			if (!optionFound) { invalidInput("Unknown option", argv[i], __LINE__); }
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
				i += OPTIONS[DEFAULT_OPTION].parserFunction(argc, argv) - 1;
				optionsUsed[DEFAULT_OPTION] = true;
				if (i < 0) { error("Default option parsed 0 arguments!", "argParser.c", __LINE__); }
			}
		}
		else
		{
			invalidInput("Expected option but received argument", argv[i], __LINE__);
		}
	}

	free(optionsUsed);

	if (inputFile == NULL) { invalidInput("Input not specified", NULL, __LINE__); }
	if (inputFile[0] == '$') { extractUrl(); }
	checkSettings();
}

static void extractUrl(void)
{
	puts("Extracting URLs...");

	if (extractorPrefixMode == EPM_DEFAULT)
	{
		extractorMaxH = EXTRACTOR_PREFIX_DEFAULT_MAX_H;
		extractorPrefixMode = EPM_MAX_H;
	}

	if (extractorPrefixMode == EPM_MAX_H)
	{
		if (extractorMaxH != 0)
		{
			char* prefix = malloc(strlen(EXTRACTOR_PREFIX_DEFAULT) + 32);
			sprintf(prefix, EXTRACTOR_PREFIX_DEFAULT, extractorMaxH, extractorMaxH);
			extractorPrefix = prefix;
		}
		else
		{
			extractorPrefix = EXTRACTOR_PREFIX_NO_MAX_H;
		}
	}

	if (extractorSuffix == NULL) { extractorSuffix = EXTRACTOR_SUFFIX_DEFAULT; }

	int commandLen = strlen(extractorPrefix) + strlen(inputFile + 1) +
		strlen(extractorSuffix) + (strlen(QUOTE_MARK) * 2) + 3; // 3 = 2 spaces + '\0'
	char* command = (char*)malloc(commandLen);
	snprintf(command, commandLen, "%s %s%s%s %s", extractorPrefix,
		QUOTE_MARK, inputFile + 1, QUOTE_MARK, extractorSuffix);

	FILE* proc = _popen(command, "r");

	int lineCount = 0;
	char** lines = NULL;

	while (lineCount < MAX_LINE_COUNT)
	{
		lines = realloc(lines, (lineCount + 1) * sizeof(char*));
		lines[lineCount] = malloc(INPUT_MAX_URL);

		if (!fgets(lines[lineCount], INPUT_MAX_URL, proc)) { break; }

		int lineLen = strlen(lines[lineCount]);
		if (lineLen > 0 && lines[lineCount][lineLen - 1] == '\n') { lines[lineCount][lineLen - 1] = '\0'; }
		lineCount++;
	}

	_pclose(proc);

	bool correctOutput = true;
	for (int i = 0; i < lineCount; i++)
	{
		if (strncmp(lines[i], "http://", 7) && strncmp(lines[i], "https://", 8))
		{
			correctOutput = false;
			break;
		}
	}

	if (correctOutput && lineCount > 0)
	{
		free(inputFile);
		inputFile = av_strndup(lines[0], INPUT_MAX_URL - 1);
		if (lineCount > 1) { secondInputFile = av_strndup(lines[1], INPUT_MAX_URL - 1); }

		if (lineCount < MAX_LINE_COUNT) { lineCount++; }
		for (int i = 0; i < lineCount; i++)
		{
			free(lines[i]);
		}
		free(lines);
	}
	else
	{
		puts("\nIncorrect output!");
		printf("Executed procedure: \"%s\"\n", command);

		puts("Output:");
		for (int i = 0; i < lineCount; i++)
		{
			fputs(lines[i], stdout);
		}

		if (lineCount == MAX_LINE_COUNT) { puts("\n[REACHED LINE COUNT LIMIT]"); }

		error("Incorrect URL extractor output!", "argParser.c", __LINE__);
	}

	free(command);
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

	if ((settings.setColorMode == SCM_CSTD_256 ||
		settings.setColorMode == SCM_CSTD_RGB) &&
		settings.colorMode != CM_CSTD_GRAY)
	{
		error("C std \"set color\" mode works only with \"cstd-gray\" color mode!", "argParser.c", __LINE__);
	}

	if (settings.colorProcMode == CPM_NONE &&
		(settings.colorMode == CM_WINAPI_GRAY ||
			settings.colorMode == CM_CSTD_GRAY))
	{
		error("Single character mode requires colors!", "argParser.c", __LINE__);
	}

	if (settings.colorProcMode == CPM_NONE && settings.brightnessRand)
	{
		if (settings.brightnessRand < 0) { settings.brightnessRand = -settings.brightnessRand; }
	}
}

static int opInput(int argc, char** argv)
{
	if (argc < 1) { notEnoughArguments(NULL, __LINE__); }

	inputFile = strdup(argv[0]);
	if (argc > 1 && argv[1][0] != '-')
	{
		if (argv[0][0] == '$' || argv[1][0] == '$') { invalidInput("Trying to pass two inputs with URL!", NULL, __LINE__); }
		secondInputFile = strdup(argv[1]);
		return 2;
	}
	return 1;
}

static int opColors(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }

	strToLower(argv[0]);
	if (!strcmp(argv[0], "winapi-gray")) { settings.colorMode = CM_WINAPI_GRAY; }
	else if (!strcmp(argv[0], "winapi-16")) { settings.colorMode = CM_WINAPI_16; }
	else if (!strcmp(argv[0], "cstd-gray")) { settings.colorMode = CM_CSTD_GRAY; }
	else if (!strcmp(argv[0], "cstd-16")) { settings.colorMode = CM_CSTD_16; }
	else if (!strcmp(argv[0], "cstd-256")) { settings.colorMode = CM_CSTD_256; }
	else if (!strcmp(argv[0], "cstd-rgb")) { settings.colorMode = CM_CSTD_RGB; }
	else { invalidInput("Invalid color mode", argv[0], __LINE__); }

	return 1;
}

static int opVolume(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }
	settings.volume = atof(argv[0]);
	if (settings.volume < 0.0 || settings.volume > 1.0) { invalidInput("Invalid volume", argv[0], __LINE__); }
	return 1;
}

static int opSize(int argc, char** argv)
{
	if (argc < 2 || argv[0][0] == '-' || argv[1][0] == '-') { notEnoughArguments(argv, __LINE__); }
	settings.argW = atoi(argv[0]);
	settings.argH = atoi(argv[1]);
	if ((settings.argW != 0 || settings.argH != 0) && (settings.argW < 4 || settings.argH < 4))
	{
		invalidInput("Invalid size", NULL, __LINE__);
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
	if (argc > 0) { tooManyArguments(argv, __LINE__); }
	showInfo();
	return 0;
}

static int opVersion(int argc, char** argv)
{
	if (argc > 0) { tooManyArguments(argv, __LINE__); }
	showVersion();
	return 0;
}

static int opHelp(int argc, char** argv)
{
	if (argc == 1 && argv[0][0] != '-')
	{
		strToLower(argv[0]);
		if (!strcmp(argv[0], "basic"))              { showHelp(1, 0, 0, 0); }
		else if (!strcmp(argv[0], "advanced"))      { showHelp(0, 1, 0, 0); }
		else if (!strcmp(argv[0], "modes"))         { showHelp(0, 0, 1, 0); }
		else if (!strcmp(argv[0], "keyboard"))      { showHelp(0, 0, 0, 1); }
		else if (!strcmp(argv[0], "full"))          { showHelp(1, 1, 1, 1); }
		else { invalidInput("Invalid help topic", argv[0], __LINE__); }
		return 1;
	}
	else if (argc == 0)
	{
		showHelp(1, 0, 0, 1);
		puts("[To see full help use \"conpl -h full\"]");
	}
	else
	{
		tooManyArguments(argv, __LINE__);
	}
	return 0;
}

static int opInterlaced(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }
	if (argc > 1 && argv[1][0] != '-')
	{
		settings.scanlineHeight = atoi(argv[1]);
		if (settings.scanlineHeight < 1) { invalidInput("Invalid scanline height", argv[1], __LINE__); }
		settings.scanlineCount = atoi(argv[0]);
		if (settings.scanlineCount < 1) { invalidInput("Invalid scanline count", argv[0], __LINE__); }
		return 2;
	}
	else
	{
		settings.scanlineCount = atoi(argv[0]);
		if (settings.scanlineCount < 1) { invalidInput("Invalid scanline count", argv[0], __LINE__); }
		return 1;
	}
}

static int opColorProc(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }

	strToLower(argv[0]);
	if (!strcmp(argv[0], "none")) { settings.colorProcMode = CPM_NONE; }
	else if (!strcmp(argv[0], "char-only")) { settings.colorProcMode = CPM_CHAR_ONLY; }
	else if (!strcmp(argv[0], "both")) { settings.colorProcMode = CPM_BOTH; }
	else { invalidInput("Invalid color processing mode", argv[0], __LINE__); }

	return 1;
}

static int opSetColor(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }

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
			invalidInput("Invalid color", argv[0], __LINE__);
		}
		
		if (argc > 1 && argv[1][0] != '-')
		{
			settings.setColorVal2 = atoi(argv[1]);
			if (settings.setColorVal2 < 0 || settings.setColorVal2 > 255)
			{
				invalidInput("Invalid color", argv[1], __LINE__);
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
			invalidInput("Invalid color", argv[0], __LINE__);
		}

		if (argc > 1 && argv[1][0] != '-')
		{
			settings.setColorVal2 = strtol(argv[1], NULL, 16);
			if (settings.setColorVal2 < 0 || settings.setColorVal2 > 0xFFFFFF)
			{
				invalidInput("Invalid color", argv[1], __LINE__);
			}
			return 2;
		}
	}
	else
	{
		invalidInput("Invalid input", argv[0], __LINE__);
	}
	return 1;
}

static int opCharset(int argc, char** argv)
{
	const int CHARSET_MAX_SIZE = 256;

	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }
	if (argv[0][0] == '#')
	{
		strToLower(argv[0]);
		if (!strcmp(argv[0], "#long")) { settings.charset = CHARSET_LONG; }
		else if (!strcmp(argv[0], "#short")) { settings.charset = CHARSET_SHORT; }
		else if (!strcmp(argv[0], "#2")) { settings.charset = CHARSET_2; }
		else if (!strcmp(argv[0], "#blocks")) { settings.charset = CHARSET_BLOCKS; }
		else if (!strcmp(argv[0], "#outline")) { settings.charset = CHARSET_OUTLINE; }
		else if (!strcmp(argv[0], "#bold-outline")) { settings.charset = CHARSET_BOLD_OUTLINE; }
		else { invalidInput("Invalid predefined charset name", argv[0], __LINE__); }
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

static int opRand(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }
	srand(time(NULL));

	if (argv[0][0] == '@') { settings.brightnessRand = -atoi(argv[0] + 1); }
	else { settings.brightnessRand = atoi(argv[0]); }

	if (settings.brightnessRand < -255 || settings.brightnessRand > 255) { invalidInput("Invalid rand value", argv[0], __LINE__); }
	return 1;
}

static int opScalingMode(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }

	strToLower(argv[0]);
	if (!strcmp(argv[0], "nearest")) { settings.scalingMode = SM_NEAREST; }
	else if (!strcmp(argv[0], "fast-bilinear")) { settings.scalingMode = SM_FAST_BILINEAR; }
	else if (!strcmp(argv[0], "bilinear")) { settings.scalingMode = SM_BILINEAR; }
	else if (!strcmp(argv[0], "bicubic")) { settings.scalingMode = SM_BICUBIC; }
	else { invalidInput("Invalid scaling mode", argv[0], __LINE__); }

	return 1;
}

static int opFontRatio(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }
	settings.constFontRatio = atof(argv[0]);
	if (settings.constFontRatio <= 0.0) { invalidInput("Invalid font ratio", argv[0], __LINE__); }
	return 1;
}

static int opSync(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }

	strToLower(argv[0]);
	if (!strcmp(argv[0], "disabled")) { settings.syncMode = SYNC_DISABLED; }
	else if (!strcmp(argv[0], "draw-all")) { settings.syncMode = SYNC_DRAW_ALL; }
	else if (!strcmp(argv[0], "enabled")) { settings.syncMode = SYNC_ENABLED; }
	else { invalidInput("Invalid synchronization mode", argv[0], __LINE__); }

	return 1;
}

static int opVideoFilters(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }
	settings.videoFilters = argv[0];
	return 1;
}

static int opScaledVideoFilters(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }
	settings.scaledVideoFilters = argv[0];
	return 1;
}

static int opAudioFilters(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }
	settings.audioFilters = argv[0];
	return 1;
}

static int opPreload(int argc, char** argv)
{
	settings.preload = true;
	return 0;
}

static int opFakeConsole(int argc, char** argv)
{
	#ifdef CP_DISABLE_OPENGL
	#ifndef _WIN32
	puts("Fake console is currently only available on Windows!");
	#endif
	error("ConPlayer compiled with disabled OpenGL!", "argParser.c", __LINE__);
	#endif

	settings.useFakeConsole = true;
	return 0;
}

static int opOpenGlSettings(int argc, char** argv)
{
	#ifdef CP_DISABLE_OPENGL

	#ifndef _WIN32
	puts("OpenGL options are currently only available on Windows!");
	#endif
	error("ConPlayer compiled with disabled OpenGL!", "argParser.c", __LINE__);

	#else
	return parseGlOptions(argc, argv);
	#endif
}

static int opExtractorMaxHeight(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }
	if (extractorPrefixMode != EPM_DEFAULT) { extractorModeCollision(__LINE__); }

	extractorMaxH = atoi(argv[0]);
	extractorPrefixMode = EPM_MAX_H;
	if (extractorMaxH < 0) { invalidInput("Maximum extractor video height cannot be negative", argv[0], __LINE__); }
	return 1;
}

static int opExtractorPrefix(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }
	if (extractorPrefixMode != EPM_DEFAULT) { extractorModeCollision(__LINE__); }

	extractorPrefix = argv[0];
	extractorPrefixMode = EPM_CUSTOM;
	return 1;
}

static int opExtractorSuffix(int argc, char** argv)
{
	if (argc < 1 || argv[0][0] == '-') { notEnoughArguments(argv, __LINE__); }
	extractorSuffix = argv[0];
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
	if (argc > 0) { tooManyArguments(argv, __LINE__); }
	showFullInfo();
	return 0;
}

static void invalidInput(char* description, char* input, int line)
{
	if (description)
	{
		if (input) { printf("%s - \"%s\"\n", description, input); }
		else { printf("%s\n", description); }
	}
	error("Invalid input!", "argParser.c", line);
}

static void tooManyArguments(char** argv, int line)
{
	invalidInput("Too many arguments for the operation", argv ? argv[-1] : NULL, line);
}

static void notEnoughArguments(char** argv, int line)
{
	invalidInput("Not enough arguments for the operation", argv ? argv[-1] : NULL, line);
}

static void extractorModeCollision(int line)
{
	invalidInput("Extractor max height and extractor prefix options are not compatible", NULL, line);
}