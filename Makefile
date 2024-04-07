ifeq ($(OS),Windows_NT)
 $(error Makefile is to compile ConPlayer for Linux; to compile it for Windows, open .sln file with Visual Studio (check README.md))
endif


C_COMPILER = gcc
RELEASE_FLAGS = -O2 -w
DEBUG_FLAGS = -g
OUTPUT_NAME = conpl

FILES = cp/src/argParser.c cp/src/audio.c cp/src/avFilters.c cp/src/decodeFrame.c cp/src/drawFrame.c cp/src/help.c cp/src/main.c cp/src/processFrame.c cp/src/queue.c cp/src/threads.c cp/src/utils.c cp/src/gl/glConsole.c cp/src/gl/glOptions.c cp/src/gl/glUtils.c cp/src/gl/shaders/glShaders.c cp/src/gl/shaders/glShStage1.c cp/src/gl/shaders/glShStage3.c cp/src/ui/ui.c cp/src/ui/menu.c
HEADERS = cp/src/conplayer.h cp/src/dependencies/atomic.h cp/src/dependencies/win_dirent.h
LIBRARIES = -lm -lpthread -lavcodec -lavformat -lavfilter -lavutil -lswresample -lswscale -lao


$(OUTPUT_NAME): $(FILES) $(HEADERS)
	$(C_COMPILER) $(RELEASE_FLAGS) $(FILES) $(LIBRARIES) -o $(OUTPUT_NAME)

$(OUTPUT_NAME)_32: $(FILES) $(HEADERS)
	$(C_COMPILER) -m32 $(RELEASE_FLAGS) $(FILES) $(LIBRARIES) -o $(OUTPUT_NAME)_32

$(OUTPUT_NAME)_dbg: $(FILES) $(HEADERS)
	$(C_COMPILER) $(DEBUG_FLAGS) $(FILES) $(LIBRARIES) -o $(OUTPUT_NAME)_dbg

$(OUTPUT_NAME)_libav_test: $(FILES) $(HEADERS)
	$(C_COMPILER) -L/usr/local/lib $(RELEASE_FLAGS) $(FILES) $(LIBRARIES) -o $(OUTPUT_NAME)_libav_test