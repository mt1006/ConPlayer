#include "../conplayer.h"

typedef void* (*SelectorFunc)(UISelectorAction, void*);
typedef void (*ValueFunc)(UIValueAction);

typedef enum
{
	SHIFT_TO_BEGIN,
	SHIFT_UP,
	SHIFT_DOWN,
	SHIFT_TO_END
} ShiftType;

static const int TEMP_FILENAMES_SIZE = 64;
static UIMenu* menuStack[16];
static int menuStackSize = 0;
static UIMenu selectorMenu = { 0 };
static char** tempFilenames = NULL;
static int tempFilenameCount = 0;
static int tempFilenamesSize = 0;

static void draw(void);
static void drawElement(UIMenuElement* element, bool isSelected);
static void runSelected(void);
static void shiftSelected(UIMenu* menu, ShiftType shiftType);
static void shiftMenuShift(UIMenu* menu);
static bool properSelected(UIMenu* menu);
static void buildMenuFromSelector(UIMenuElement* element, UIMenu* out);
static void buildFileSelector(UIMenuElement* element, UIMenu* out);
static void selectorAction(void);
static void inputSelectorAction(void);
static bool isSlash(char ch);

void uiMenuLoop(UIMenu* mainMenu)
{
	const int SLEEP_ON_START = 300;
	const double KEYBOARD_DELAY = 0.16;

	menuStack[0] = mainMenu;
	menuStackSize++;

	UIMenu* menu = mainMenu;
	shiftSelected(menu, SHIFT_TO_BEGIN);
	draw();

	Sleep(SLEEP_ON_START);

	double keyTime = 0.0;
	double mutedVolume = 0.0;

	while (uiEnabled)
	{
		menu = menuStack[menuStackSize - 1];
		unsigned char key = getChar(true);

		double newKeyTime = getTime();
		if (newKeyTime < keyTime + KEYBOARD_DELAY) { continue; }
		keyTime = newKeyTime;

		double newVolume = settings.volume;

		switch (key)
		{
		case VK_LEFT:
		case VK_ESCAPE:
			uiPopMenu();
			if (menuStackSize == 0)
			{
				clearScreen();
				cpExit(0);
			}
			break;

		case VK_RIGHT:
		case VK_RETURN:
			runSelected();
			break;

		case VK_DOWN:
			shiftSelected(menu, SHIFT_DOWN);
			break;

		case VK_UP:
			shiftSelected(menu, SHIFT_UP);
			break;

		case VK_HOME:
			shiftSelected(menu, SHIFT_TO_BEGIN);
			break;

		case VK_END:
			shiftSelected(menu, SHIFT_TO_END);
			break;
		}
		draw();
	}
}

void uiPushMenu(UIMenu* menu)
{
	if (menuStackSize > 15) { error("UIMenu stack overflow!", "ui/menu.c", __LINE__); }
	menuStack[menuStackSize] = menu;
	menuStackSize++;
	if (!properSelected(menu)) { shiftSelected(menu, SHIFT_TO_BEGIN); }
}

void uiPopMenu(void)
{
	if (menuStackSize == 0) { error("Trying to remove main menu from UIMenu stack!", "ui/menu.c", __LINE__); }
	menuStackSize--;
}

static void draw(void)
{
	UIMenu* menu = menuStack[menuStackSize - 1];
	clearScreen();

	int w, h;
	getConsoleSize(&w, &h);
	
	if (menu->oldH != h)
	{
		menu->shift = 0;
		menu->oldH = h;
	}

	if (menu->count > h - 2)
	{
		if (menu->shift == 0) { puts("------"); }
		else { puts("/\\/\\/\\"); }

		for (int i = menu->shift; i < menu->shift + h - 4; i++)
		{
			if (i >= menu->count) { error("UI error!", "menu/menu.c", __LINE__); }
			drawElement(&menu->elements[i], menu->selected == i);
		}

		if (menu->shift == menu->count - h + 4) { puts("------"); }
		else { puts("\\/\\/\\/"); }
	}
	else
	{
		for (int i = 0; i < menu->count; i++)
		{
			drawElement(&menu->elements[i], menu->selected == i);
		}
	}

	if (CP_IS_WINDOWS)
	{
		setCursorPos(0, h - 2);
		fputs("Use arrows/enter to navigate UI", stdout);
		setCursorPos(0, h - 1);
		fputs("Instead of UI you can also use command line", stdout);
	}

	setCursorPos(0, 0);
}

static void drawElement(UIMenuElement* element, bool isSelected)
{
	char* str;
	int val;

	switch (element->type)
	{
	case UI_TEXT:
		puts(element->text);
		break;

	case UI_SELECTOR:
		str = ((SelectorFunc)element->ptr)(UI_SELECTOR_GET_SELECTED_NAME, NULL);
		if (isSelected) { printf(">%s: [%s]<\n", element->text, str); }
		else { printf(" %s: [%s]\n", element->text, str); }
		break;

	case UI_VALUE:
		if (isSelected) { printf(">%s: ", element->text); }
		else { printf(" %s: ", element->text); }
		((ValueFunc)element->ptr)(UI_VALUE_PRINT);
		if (isSelected) { puts("<"); }
		else { puts(""); }
		break;

	case UI_SWITCH:
		str = *(bool*)element->ptr ? "true" : "false";
		if (isSelected) { printf(">%s: %s<\n", element->text, str); }
		else { printf(" %s: %s\n", element->text, str); }
		break;

	case UI_FILE_SELECTOR:
		str = *((UIFileSelectorData*)element->ptr)->out;
		if (str != NULL)
		{
			for (int i = strlen(str) - 1; i >= 0; i--)
			{
				if (isSlash(str[i]))
				{
					str += i + 1;
					break;
				}
			}
		}
		else
		{
			str = "[not selected]";
		}

		if (isSelected) { printf(">%s: %s<\n", element->text, str); }
		else { printf(" %s: %s\n", element->text, str); }
		break;

	default:
		if (isSelected) { printf(">%s<\n", element->text); }
		else { printf(" %s\n", element->text); }
	}
}

static void runSelected(void)
{
	UIMenu* menu = menuStack[menuStackSize - 1];
	UIMenuElement* element = &menu->elements[menu->selected];

	void (*func)(void);

	switch (element->type)
	{
	case UI_ACTION:
		((void (*)(void))element->ptr)(); // executing function that ptr points to
		break;

	case UI_SUBMENU:
		uiPushMenu(element->ptr);
		break;

	case UI_SELECTOR:
		uiClearMenu(&selectorMenu);
		buildMenuFromSelector(element, &selectorMenu);
		uiPushMenu(&selectorMenu);
		break;

	case UI_VALUE:
		clearScreen();
		((ValueFunc)element->ptr)(UI_VALUE_GET);
		clearScreen();
		break;

	case UI_SWITCH:
		*(bool*)element->ptr = !(*(bool*)element->ptr);
		break;

	case UI_FILE_SELECTOR:
		uiClearMenu(&selectorMenu);
		buildFileSelector(element, &selectorMenu);
		uiPushMenu(&selectorMenu);
		break;
	}
}

static void shiftSelected(UIMenu* menu, ShiftType shiftType)
{
	int oldSelected = menu->selected;

	switch (shiftType)
	{
	case SHIFT_TO_BEGIN:
		menu->selected = 0;
		break;

	case SHIFT_UP:
		if (menu->selected > 0) { menu->selected--; }
		break;

	case SHIFT_DOWN:
		if (menu->selected < menu->count - 1) { menu->selected++; }
		break;

	case SHIFT_TO_END:
		menu->selected = menu->count - 1;
		break;
	}

	switch (shiftType)
	{
	case SHIFT_TO_BEGIN:
	case SHIFT_DOWN:
		while (menu->selected < menu->count)
		{
			if (properSelected(menu)) { break; }
			menu->selected++;
		}
		if (menu->selected >= menu->count) { menu->selected = oldSelected; }
		break;

	case SHIFT_UP:
	case SHIFT_TO_END:
		while (menu->selected >= 0)
		{
			if (properSelected(menu)) { break; }
			menu->selected--;
		}
		if (menu->selected < 0) { menu->selected = oldSelected; }
		break;
	}

	shiftMenuShift(menu);
}

static void shiftMenuShift(UIMenu* menu)
{
	if (menu->count <= menu->oldH - 2) { return; }

	int topElement = menu->shift;
	int bottomElement = menu->shift + menu->oldH - 5;

	if (menu->selected < topElement)
	{
		menu->shift -= topElement - menu->selected;
	}
	else if (menu->selected > bottomElement)
	{
		menu->shift += menu->selected - bottomElement;
	}

	if (menu->count > 0 && menu->elements[0].type == UI_TEXT
		&& bottomElement != topElement && menu->shift == 1)
	{
		menu->shift = 0;
	}
}

static bool properSelected(UIMenu* menu)
{
	return menu->elements[menu->selected].type != UI_TEXT;
}

static void buildMenuFromSelector(UIMenuElement* element, UIMenu* out)
{
	SelectorFunc func = element->ptr;

	uiAddElement(out, UI_TEXT, element->text, NULL);
	for (int i = 0; i < (int64_t)func(UI_SELECTOR_GET_COUNT, NULL); i++)
	{
		uiAddElement(out, UI_ACTION, func(UI_SELECTOR_GET_NAME, (void*)(int64_t)i), &selectorAction);
	}
	uiAddElement(out, UI_ACTION, "[Cancel]", &uiPopMenu);

	out->selected = (int)(int64_t)func(UI_SELECTOR_GET_POS, NULL) + 1;
}

static void buildFileSelector(UIMenuElement* element, UIMenu* out)
{
	UIFileSelectorData* selectorData = element->ptr;

	if (tempFilenames != NULL)
	{
		for (int i = 0; i < tempFilenameCount; i++)
		{
			free(tempFilenames[i]);
		}
		free(tempFilenames);
	}

	tempFilenames = malloc(TEMP_FILENAMES_SIZE * sizeof(char*));
	tempFilenameCount = 0;
	tempFilenamesSize = TEMP_FILENAMES_SIZE;

	if (selectorData->currentPath == NULL)
	{
		selectorData->currentPath = malloc(4096);
		if (!getcwd(selectorData->currentPath, 4096)) { error("Failed to get current directory!", "ui/menu.c", __LINE__); }
	}

	uiAddElement(out, UI_TEXT, selectorData->currentPath, NULL);
	uiAddElement(out, UI_ACTION, "<..>", &inputSelectorAction);

	DIR* dir = opendir(selectorData->currentPath);
	struct dirent* dirElement;

	while ((dirElement = readdir(dir)) != NULL)
	{
		if (dirElement->d_type == DT_DIR)
		{
			if (!strcmp(dirElement->d_name, ".") || !strcmp(dirElement->d_name, "..")) { continue; }

			int newStrSize = strlen(dirElement->d_name) + 3;
			char* newStr = malloc(newStrSize);
			snprintf(newStr, newStrSize, "<%s>", dirElement->d_name);

			uiAddElement(out, UI_ACTION, newStr, &inputSelectorAction);

			tempFilenameCount++;
			if (tempFilenameCount > tempFilenamesSize)
			{
				tempFilenamesSize += TEMP_FILENAMES_SIZE;
				tempFilenames = realloc(tempFilenames, tempFilenamesSize * sizeof(char*));
			}
			tempFilenames[tempFilenameCount - 1] = newStr;
		}
	}

	rewinddir(dir);

	while ((dirElement = readdir(dir)) != NULL)
	{
		if (dirElement->d_type != DT_DIR)
		{
			char* newStr = strdup(dirElement->d_name);
			uiAddElement(out, UI_ACTION, newStr, &inputSelectorAction);

			tempFilenameCount++;
			if (tempFilenameCount > tempFilenamesSize)
			{
				tempFilenamesSize += TEMP_FILENAMES_SIZE;
				tempFilenames = realloc(tempFilenames, tempFilenamesSize * sizeof(char*));
			}
			tempFilenames[tempFilenameCount - 1] = newStr;
		}
	}

	closedir(dir);
	uiAddElement(out, UI_ACTION, "[Cancel]", &uiPopMenu);

	out->selected = 1;
}

static void selectorAction(void)
{
	int pos = menuStack[menuStackSize - 1]->selected - 1;
	UIMenu* menu = menuStack[menuStackSize - 2];
	((SelectorFunc)menu->elements[menu->selected].ptr)(UI_SELECTOR_SELECT, (void*)(int64_t)pos);
}

static void inputSelectorAction(void)
{
	UIMenu* menu = menuStack[menuStackSize - 1];
	UIMenu* parentMenu = menuStack[menuStackSize - 2];
	UIMenuElement* parent = &parentMenu->elements[parentMenu->selected];
	UIMenuElement* element = &menu->elements[menu->selected];
	UIFileSelectorData* selectorData = parent->ptr;

	if (element->text[0] == '<')
	{
		if (!strcmp(element->text, "<..>"))
		{
			for (int i = strlen(selectorData->currentPath) - 1; i >= 0; i--)
			{
				if (isSlash(selectorData->currentPath[i]))
				{
					selectorData->currentPath[i] = '\0';
					break;
				}
			}

			int newLen = strlen(selectorData->currentPath);
			bool slashFound = false;
			for (int i = 0; i < newLen; i++)
			{
				if (isSlash(selectorData->currentPath[i]))
				{
					slashFound = true;
					break;
				}
			}

			if (!slashFound)
			{
				selectorData->currentPath[newLen] = CP_IS_WINDOWS ? '\\' : '/';
				selectorData->currentPath[newLen + 1] = '\0';
			}
		}
		else
		{
			int len = strlen(selectorData->currentPath);
			if (len == 0 || !isSlash(selectorData->currentPath[len - 1]))
			{
				selectorData->currentPath[len++] = CP_IS_WINDOWS ? '\\' : '/';
			}

			int addLen = strlen(element->text) - 2;
			if (len + addLen + 1 > 4096) { error("Path too long!", "ui/menu.c", __LINE__); }

			memcpy(selectorData->currentPath + len, element->text + 1, addLen);
			selectorData->currentPath[len + addLen] = '\0';
		}
	}
	else
	{
		int cur = 0;
		int len = strlen(selectorData->currentPath);
		int filenameLen = strlen(element->text);

		free(*selectorData->out);
		*selectorData->out = malloc(len + filenameLen + 2);

		memcpy(*selectorData->out, selectorData->currentPath, len);
		cur = len;

		if (len == 0 || !isSlash(selectorData->currentPath[len - 1]))
		{
			(*selectorData->out)[cur++] = CP_IS_WINDOWS ? '\\' : '/';
		}

		memcpy((*selectorData->out) + cur, element->text, filenameLen);
		(*selectorData->out)[cur + filenameLen] = '\0';
		uiPopMenu();
	}

	uiClearMenu(menu);
	buildFileSelector(parent, menu);
	return;
}

static bool isSlash(char ch)
{
	return ch == '/' || ch == '\\';
}