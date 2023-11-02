#include "../conplayer.h"

bool uiKeepLoop = true;

static UIMenu mainMenu = { 0 };
static UIMenu settingsMenu = { 0 };
static UIMenu moreSettingsMenu = { 0 };
static UIMenu messageOverlay = { 0 };
static Settings defaultSettings;
static char* glsString = NULL;
static UIFileSelectorData inputSelector = { 0 };

static void initUI(void);
static void action_info(void);
static void action_run(void);
static void action_exit(void);
static void* selector_colorMode(UISelectorAction action, void* arg);
static void* selector_colorProcessingMode(UISelectorAction action, void* arg);
static void* selector_charset(UISelectorAction action, void* arg);
static void* selector_constantColor(UISelectorAction action, void* arg);
static void* selector_scalingMode(UISelectorAction action, void* arg);
static void* selector_syncMode(UISelectorAction action, void* arg);
static void value_volume(UIValueAction action);
static void value_size(UIValueAction action);
static void value_interlacing(UIValueAction action);
static void value_randomization(UIValueAction action);
static void value_fontRatio(UIValueAction action);
static void value_filters(UIValueAction action, const char* str, char** field);
static void value_videoFilters(UIValueAction action);
static void value_scaledVideoFilters(UIValueAction action);
static void value_audioFilters(UIValueAction action);
static void value_gls(UIValueAction action);
static void showMessage(const char* text);
static void selectorError(int line);
static int readIntOrDef(const char* str, int def, int radix);
static double readDoubleOrDef(const char* str, int def);
static char* readString(const char* str);

void uiShowRunScreen(void)
{
	initUI();
	uiMenuLoop(&mainMenu);
}

void uiAddElement(UIMenu* menu, UIMenuElementType type, const char* text, void* ptr)
{
	menu->elements = realloc(menu->elements, sizeof(UIMenuElement) * (menu->count + 1));
	menu->elements[menu->count].type = type;
	menu->elements[menu->count].text = text;
	menu->elements[menu->count].ptr = ptr;
	menu->count++;
}

void uiClearMenu(UIMenu* menu)
{
	free(menu->elements);
	menu->elements = NULL;
	menu->count = 0;
	menu->selected = 0;
	menu->shift = 0;
	menu->oldH = -1;
}

static void initUI(void)
{
	#ifdef _WIN32
	outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	#endif

	memcpy(&defaultSettings, &settings, sizeof(Settings));

	inputSelector.out = &inputFile;
	settings.charset = CHARSET_LONG;
	settings.charsetSize = strlen(CHARSET_LONG);

	uiAddElement(&mainMenu, UI_TEXT, CP_VERSION_STRING, NULL);
	uiAddElement(&mainMenu, UI_FILE_SELECTOR, "Select input", &inputSelector);
	uiAddElement(&mainMenu, UI_SUBMENU, "Settings", &settingsMenu);
	uiAddElement(&mainMenu, UI_ACTION, "Information", &action_info);
	uiAddElement(&mainMenu, UI_ACTION, "Run", &action_run);
	uiAddElement(&mainMenu, UI_ACTION, "Exit", &action_exit);

	uiAddElement(&settingsMenu, UI_TEXT, "Settings", NULL);
	uiAddElement(&settingsMenu, UI_SELECTOR, "Color mode", &selector_colorMode);
	uiAddElement(&settingsMenu, UI_VALUE, "Volume", &value_volume);
	uiAddElement(&settingsMenu, UI_VALUE, "Size", &value_size);
	uiAddElement(&settingsMenu, UI_SWITCH, "Fill", &settings.fillArea);
	uiAddElement(&settingsMenu, UI_SUBMENU, "[More settings]", &moreSettingsMenu);
	uiAddElement(&settingsMenu, UI_ACTION, "[Go back]", &uiPopMenu);

	uiAddElement(&moreSettingsMenu, UI_TEXT, "Advanced settings", NULL);
	uiAddElement(&moreSettingsMenu, UI_VALUE, "Interlacing", &value_interlacing);
	uiAddElement(&moreSettingsMenu, UI_VALUE, "Randomization", &value_randomization);
	uiAddElement(&moreSettingsMenu, UI_VALUE, "Font ratio", &value_fontRatio);
	uiAddElement(&moreSettingsMenu, UI_SELECTOR, "Charset", &selector_charset);
	uiAddElement(&moreSettingsMenu, UI_SELECTOR, "Constant color", &selector_constantColor);
	uiAddElement(&moreSettingsMenu, UI_SELECTOR, "Color processing mode", &selector_colorProcessingMode);
	uiAddElement(&moreSettingsMenu, UI_SELECTOR, "Scaling mode", &selector_scalingMode);
	uiAddElement(&moreSettingsMenu, UI_SELECTOR, "Synchronization mode", &selector_syncMode);
	uiAddElement(&moreSettingsMenu, UI_VALUE, "Video filters", &value_videoFilters);
	uiAddElement(&moreSettingsMenu, UI_VALUE, "Scaled video filters", &value_scaledVideoFilters);
	uiAddElement(&moreSettingsMenu, UI_VALUE, "Audio filters", &value_audioFilters);
	uiAddElement(&moreSettingsMenu, UI_SWITCH, "Disable clearing screen", &settings.disableCLS);
	uiAddElement(&moreSettingsMenu, UI_SWITCH, "Disable audio", &settings.disableAudio);
	uiAddElement(&moreSettingsMenu, UI_SWITCH, "Disable keyboard control", &settings.disableKeyboard);
	uiAddElement(&moreSettingsMenu, UI_SWITCH, "Enable Libav logs", &settings.libavLogs);
	uiAddElement(&moreSettingsMenu, UI_SWITCH, "Use fake console", &settings.useFakeConsole);
	uiAddElement(&moreSettingsMenu, UI_VALUE, "Fake console OpenGL settings", &value_gls);
	uiAddElement(&moreSettingsMenu, UI_ACTION, "[Go back]", &uiPopMenu);

	uiAddElement(&messageOverlay, UI_TEXT, "", NULL);
	uiAddElement(&messageOverlay, UI_ACTION, "OK", &uiPopMenu);
}

static void action_info(void)
{
	showMessage(INFO_MESSAGE);
}

static void action_run(void)
{
	if (inputFile != NULL)
	{
		if (settings.useFakeConsole)
		{
			#ifdef CP_DISABLE_OPENGL
			
			#ifdef _WIN32
			showMessage("Fake console is currently only available on Windows!");
			#else
			showMessage("OpenGL options are currently only available on Windows!");
			#endif
			return;

			#else
			
			char** argv;
			int argc = parseStringAsArgv(glsString, &argv);
			parseGlOptions(argc, argv);

			#endif
		}
		uiKeepLoop = false;
	}
	else
	{
		showMessage("Specify input before running!");
	}
}

static void action_exit(void)
{
	clearScreen();
	cpExit(0);
}

static void* selector_colorMode(UISelectorAction action, void* arg)
{
	int pos = (int)(int64_t)arg;
	switch (action)
	{
	case UI_SELECTOR_GET_COUNT:
		return (void*)6;

	case UI_SELECTOR_GET_POS:
		return (void*)(int64_t)settings.colorMode;

	case UI_SELECTOR_GET_NAME:
		switch ((ColorMode)(int64_t)arg)
		{
		case CM_WINAPI_GRAY: return "winapi-gray";
		case CM_WINAPI_16: return "winapi-16";
		case CM_CSTD_GRAY: return "cstd-gray";
		case CM_CSTD_16: return "cstd-16";
		case CM_CSTD_256: return "cstd-256";
		case CM_CSTD_RGB: return "cstd-rgb";
		default: selectorError(__LINE__);
		}
		break;
		
	case UI_SELECTOR_GET_SELECTED_NAME:
		return selector_colorMode(UI_SELECTOR_GET_NAME, selector_colorMode(UI_SELECTOR_GET_POS, NULL));

	case UI_SELECTOR_SELECT:
		if (pos < CM_WINAPI_GRAY || pos > CM_CSTD_RGB) { selectorError(__LINE__); }
		if (!CP_IS_WINDOWS && (pos == 0 || pos == 1))
		{
			showMessage("WinAPI color mode not supported on Linux!");
			break;
		}
		settings.colorMode = (ColorMode)pos;
		uiPopMenu();
	}
	return NULL;
}

static void* selector_colorProcessingMode(UISelectorAction action, void* arg)
{
	int pos = (int)(int64_t)arg;
	switch (action)
	{
	case UI_SELECTOR_GET_COUNT:
		return (void*)3;

	case UI_SELECTOR_GET_POS:
		return (void*)(int64_t)settings.colorProcMode;

	case UI_SELECTOR_GET_NAME:
		switch ((ColorProcMode)(int64_t)arg)
		{
		case CPM_NONE: return "none";
		case CPM_CHAR_ONLY: return "char-only";
		case CPM_BOTH: return "both";
		default: selectorError(__LINE__);
		}
		break;

	case UI_SELECTOR_GET_SELECTED_NAME:
		return selector_colorProcessingMode(UI_SELECTOR_GET_NAME,
			selector_colorProcessingMode(UI_SELECTOR_GET_POS, NULL));

	case UI_SELECTOR_SELECT:
		if (pos < CPM_NONE || pos > CPM_BOTH) { selectorError(__LINE__); }
		settings.colorProcMode = (ColorProcMode)pos;
		uiPopMenu();
	}
	return NULL;
}

static void* selector_charset(UISelectorAction action, void* arg)
{
	static int charsetPos = 0;
	int pos = (int)(int64_t)arg;

	switch (action)
	{
	case UI_SELECTOR_GET_COUNT:
		return (void*)6;

	case UI_SELECTOR_GET_POS:
		return (void*)(int64_t)charsetPos;

	case UI_SELECTOR_GET_NAME:
		switch ((int64_t)arg)
		{
		case 0: return "long";
		case 1: return "short";
		case 2: return "2";
		case 3: return "blocks";
		case 4: return "outline";
		case 5: return "bold-outline";
		default: selectorError(__LINE__);
		}
		break;

	case UI_SELECTOR_GET_SELECTED_NAME:
		return selector_charset(UI_SELECTOR_GET_NAME,
			selector_charset(UI_SELECTOR_GET_POS, NULL));

	case UI_SELECTOR_SELECT:
		if (pos < 0 || pos > 5) { selectorError(__LINE__); }
		switch (pos)
		{
		case 0: settings.charset = CHARSET_LONG; break;
		case 1: settings.charset = CHARSET_SHORT; break;
		case 2: settings.charset = CHARSET_2; break;
		case 3: settings.charset = CHARSET_BLOCKS; break;
		case 4: settings.charset = CHARSET_OUTLINE; break;
		case 5: settings.charset = CHARSET_BOLD_OUTLINE; break;
		}
		settings.charsetSize = strlen(settings.charset);
		charsetPos = pos;
		uiPopMenu();
	}
	return NULL;
}

static void* selector_constantColor(UISelectorAction action, void* arg)
{
	int pos = (int)(int64_t)arg;
	switch (action)
	{
	case UI_SELECTOR_GET_COUNT:
		return (void*)4;

	case UI_SELECTOR_GET_POS:
		return (void*)(int64_t)settings.setColorMode;

	case UI_SELECTOR_GET_NAME:
		switch ((SetColorMode)(int64_t)arg)
		{
		case SCM_DISABLED: return "disabled";
		case SCM_WINAPI: return "winapi";
		case SCM_CSTD_256: return "cstd-256";
		case SCM_CSTD_RGB: return "cstd-rgb";
		default: selectorError(__LINE__);
		}
		break;

	case UI_SELECTOR_GET_SELECTED_NAME:
		return selector_constantColor(UI_SELECTOR_GET_NAME,
			selector_constantColor(UI_SELECTOR_GET_POS, NULL));

	case UI_SELECTOR_SELECT:
		if (pos < SCM_DISABLED || pos > SCM_CSTD_RGB) { selectorError(__LINE__); }
		settings.setColorMode = (SetColorMode)pos;
		uiPopMenu();
		clearScreen();

		int minVal = INT_MIN;
		int maxVal = INT_MAX;
		switch (settings.setColorMode)
		{
		case SCM_WINAPI:
			settings.setColorVal1 = readIntOrDef("Color value", defaultSettings.setColorVal1, 10);
			break;

		case SCM_CSTD_256:
			settings.setColorVal1 = readIntOrDef("Text color (0-255)", defaultSettings.setColorVal1, 10);
			settings.setColorVal2 = readIntOrDef("Background color (0-255)", defaultSettings.setColorVal2, 10);
			minVal = 0;
			maxVal = 255;
			break;

		case SCM_CSTD_RGB:
			settings.setColorVal1 = readIntOrDef("Text color (in hex)", defaultSettings.setColorVal1, 16);
			settings.setColorVal2 = readIntOrDef("Background color (in hex)", defaultSettings.setColorVal2, 16);
			minVal = 0;
			maxVal = 0xFFFFFF;
			break;
		}

		if (settings.setColorVal1 < minVal || settings.setColorVal1 > maxVal ||
			settings.setColorVal2 < minVal || settings.setColorVal2 > maxVal)
		{
			settings.setColorVal1 = defaultSettings.setColorVal1;
			settings.setColorVal2 = defaultSettings.setColorVal2;
			showMessage("Invalid color values!");
		}
		break;
	}
	return NULL;
}

static void* selector_scalingMode(UISelectorAction action, void* arg)
{
	int pos = (int)(int64_t)arg;
	switch (action)
	{
	case UI_SELECTOR_GET_COUNT:
		return (void*)4;

	case UI_SELECTOR_GET_POS:
		return (void*)(int64_t)settings.scalingMode;

	case UI_SELECTOR_GET_NAME:
		switch ((ScalingMode)(int64_t)arg)
		{
		case SM_NEAREST: return "nearest";
		case SM_FAST_BILINEAR: return "fast-bilinear";
		case SM_BILINEAR: return "bilinear";
		case SM_BICUBIC: return "bicubic";
		default: selectorError(__LINE__);
		}
		break;

	case UI_SELECTOR_GET_SELECTED_NAME:
		return selector_scalingMode(UI_SELECTOR_GET_NAME, selector_scalingMode(UI_SELECTOR_GET_POS, NULL));

	case UI_SELECTOR_SELECT:
		if (pos < SM_NEAREST || pos > SM_BICUBIC) { selectorError(__LINE__); }
		settings.scalingMode = (ScalingMode)pos;
		uiPopMenu();
	}
	return NULL;
}

static void* selector_syncMode(UISelectorAction action, void* arg)
{
	int pos = (int)(int64_t)arg;
	switch (action)
	{
	case UI_SELECTOR_GET_COUNT:
		return (void*)3;

	case UI_SELECTOR_GET_POS:
		return (void*)(int64_t)settings.syncMode;

	case UI_SELECTOR_GET_NAME:
		switch ((SyncMode)(int64_t)arg)
		{
		case SYNC_DISABLED: return "disabled";
		case SYNC_DRAW_ALL: return "draw-all";
		case SYNC_ENABLED: return "enabled";
		default: selectorError(__LINE__);
		}
		break;

	case UI_SELECTOR_GET_SELECTED_NAME:
		return selector_syncMode(UI_SELECTOR_GET_NAME, selector_syncMode(UI_SELECTOR_GET_POS, NULL));

	case UI_SELECTOR_SELECT:
		if (pos < SYNC_DISABLED || pos > SYNC_ENABLED) { selectorError(__LINE__); }
		settings.syncMode = (SyncMode)pos;
		uiPopMenu();
	}
	return NULL;
}

static void value_volume(UIValueAction action)
{
	double newValue;
	switch (action)
	{
	case UI_VALUE_PRINT:
		printf("%.2lf", settings.volume);
		break;

	case UI_VALUE_GET:
		newValue = readDoubleOrDef("New volume", defaultSettings.volume);
		if (newValue < 0.0 || newValue > 1.0) { showMessage("Invalid volume!"); }
		else { settings.volume = newValue; }
		break;
	}
}

static void value_size(UIValueAction action)
{
	double newW, newH;
	switch (action)
	{
	case UI_VALUE_PRINT:
		if (settings.argW == -1 && settings.argH == -1) { fputs("[not specified]", stdout); }
		else { printf("%dx%d", settings.argW, settings.argH); }
		break;

	case UI_VALUE_GET:
		newW = readIntOrDef("New width", defaultSettings.argW, 10);
		if (newW < 4 && newW != 0 && newW != -1)
		{
			showMessage("Invalid width!");
			break;
		}

		newH = readIntOrDef("New height", defaultSettings.argH, 10);
		if (newH < 4 && newH != 0 && newH != -1)
		{
			showMessage("Invalid height!");
			break;
		}

		if ((newW == 0 || newH == 0 || newW == -1 || newH == -1) && newW != newH)
		{
			showMessage("Invalid size!");
		}
		else
		{
			settings.argW = newW;
			settings.argH = newH;
		}
		break;
	}
}

static void value_interlacing(UIValueAction action)
{
	int newValue, newScanlineHeight;
	switch (action)
	{
	case UI_VALUE_PRINT:
		if (settings.scanlineCount == 1) { fputs("[disabled]", stdout); }
		else if (settings.scanlineHeight == 1) { printf("%d", settings.scanlineCount); }
		else { printf("%d (scanline height: %d)", settings.scanlineCount, settings.scanlineHeight); }
		break;

	case UI_VALUE_GET:
		newValue = readIntOrDef("New value", defaultSettings.scanlineCount, 10);
		if (newValue == 1)
		{
			settings.scanlineCount = 1;
			settings.scanlineHeight = 1;
			break;
		}
		else if (newValue < 1)
		{
			showMessage("Invalid value!");
			break;
		}

		newScanlineHeight = readIntOrDef("New scanline height", defaultSettings.scanlineHeight, 10);
		if (newScanlineHeight < 1)
		{
			showMessage("Invalid scanline height!");
			break;
		}

		settings.scanlineCount = newValue;
		settings.scanlineHeight = newScanlineHeight;
		break;
	}
}

static void value_randomization(UIValueAction action)
{
	int newValue;
	switch (action)
	{
	case UI_VALUE_PRINT:
		printf("%d", settings.brightnessRand);
		break;

	case UI_VALUE_GET:
		newValue = readIntOrDef("New value", defaultSettings.brightnessRand, 10);
		if (newValue < -255 || newValue > 255) { showMessage("Invalid randomization value!"); }
		else { settings.brightnessRand = newValue; }
	}
}

static void value_fontRatio(UIValueAction action)
{
	double newValue;
	switch (action)
	{
	case UI_VALUE_PRINT:
		if (settings.constFontRatio == 0.0) { fputs("[auto]", stdout); }
		else { printf("%.3f", settings.constFontRatio); }
		break;

	case UI_VALUE_GET:
		newValue = readDoubleOrDef("New value", defaultSettings.constFontRatio);
		if (newValue < 0.0) { showMessage("Invalid font ratio!"); }
		else { settings.constFontRatio = newValue; }
	}
}

static void value_filters(UIValueAction action, const char* str, char** field)
{
	switch (action)
	{
	case UI_VALUE_PRINT:
		if (*field == NULL) { fputs("[disabled]", stdout); }
		else { fputs(*field, stdout); }
		break;

	case UI_VALUE_GET:
		puts("FFmpeg filters documentation: https://www.ffmpeg.org/ffmpeg-filters.html");
		free(*field);
		*field = readString(str);
	}
}

static void value_videoFilters(UIValueAction action)
{
	value_filters(action, "New video filters", &settings.videoFilters);
}

static void value_scaledVideoFilters(UIValueAction action)
{
	value_filters(action, "New scaled video filters", &settings.scaledVideoFilters);
}

static void value_audioFilters(UIValueAction action)
{
	value_filters(action, "New audio filters", &settings.audioFilters);
}

static void value_gls(UIValueAction action)
{
	switch (action)
	{
	case UI_VALUE_PRINT:
		if (glsString == NULL) { fputs("[empty]", stdout); }
		else { fputs(glsString, stdout); }
		break;

	case UI_VALUE_GET:
		free(glsString);
		glsString = readString("New OpenGL settings");
	}
}

static void showMessage(const char* text)
{
	messageOverlay.elements[0].text = text;
	uiPushMenu(&messageOverlay);
}

static void selectorError(int line)
{
	error("Selector error!", "ui/ui.c", line);
}

static int readIntOrDef(const char* str, int def, int radix)
{
	char buf[32];
	printf("%s: ", str);
	fgets(buf, 32, stdin);
	return buf[0] != '\n' ? strtol(buf, NULL, radix) : def;
}

static double readDoubleOrDef(const char* str, int def)
{
	char buf[32];
	printf("%s: ", str);
	fgets(buf, 32, stdin);
	return buf[0] != '\n' ? atof(buf) : def;
}

static char* readString(const char* str)
{
	char buf[256];
	char* retStr = NULL;
	int size = 0;

	printf("%s: ", str);
	while (true)
	{
		fgets(buf, 256, stdin);
		int bufSize = strlen(buf);
		retStr = realloc(retStr, size + bufSize + 1);
		memcpy(retStr + size, buf, bufSize + 1);
		size += bufSize;
		if (buf[bufSize - 1] == '\n') { break; }
	}

	if (retStr == NULL || retStr[0] == '\n' || size < 1) { return NULL; }
	retStr[size - 1] = '\0';
	return retStr;
}