ifeq ($(OS),Windows_NT)
 $(error Makefile is to compile ConPlayer for Linux; to compile it for Windows, open .sln file with Visual Studio (check README.md))
endif


C_COMPILER = gcc
COMPILER_FLAGS = -O2 -w
OUTPUT_NAME = conpl

FILES = cp/src/argParser.c cp/src/audio.c cp/src/avFilters.c cp/src/decodeFrame.c cp/src/drawFrame.c cp/src/help.c cp/src/main.c cp/src/processFrame.c cp/src/queue.c cp/src/threads.c cp/src/utils.c cp/src/gl/glConsole.c cp/src/gl/glOptions.c cp/src/gl/glUtils.c cp/src/gl/shaders/glShaders.c cp/src/gl/shaders/glShStage1.c
LIBRARIES = -lm -lpthread -lavcodec -lavformat -lavfilter -lavutil -lswresample -lswscale -lao


$(OUTPUT_NAME): $(FILES_TO_COMPILE) cp/src/conplayer.h
	$(C_COMPILER) $(COMPILER_FLAGS) $(FILES) $(LIBRARIES) -o $(OUTPUT_NAME)

$(OUTPUT_NAME)_32: $(FILES_TO_COMPILE) cp/src/conplayer.h
	$(C_COMPILER) -m32 $(FILES_TO_COMPILE) $(COMPILER_FLAGS) $(LIBRARIES) -o $(OUTPUT_NAME)_32
