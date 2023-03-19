/*
* Project: ConPlayer
* Version: 1.4
* Author: https://github.com/mt1006
*/

#pragma once
#define _CRT_SECURE_NO_WARNINGS

/*
*  vcpkg (Windows):
*    vcpkg install ffmpeg[gpl,freetype,fontconfig,fribidi,ass,opencl,dav1d]:x64-windows
*    vcpkg install libao:x64-windows
*  apt-get (Linux):
*    sudo apt-get install libavcodec-dev
*    sudo apt-get install libavformat-dev
*    sudo apt-get install libavfilter-dev
*    sudo apt-get install libswscale-dev
*    sudo apt-get install libao-dev
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <float.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavutil/imgutils.h>
#include <libavutil/file.h>
#include <libavutil/opt.h>
#include <libavutil/log.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <ao/ao.h>

#ifdef _WIN32

#include <conio.h>
#include <process.h>
#include <Windows.h>
#include <shellscalingapi.h>

#pragma comment(lib, "Shcore")

#ifndef CP_DISABLE_OPENGL
#include <gl/GL.h>
#include <gl/GLU.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#endif

#define CP_OS "Windows"

#else

#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sched.h>

#define CP_DISABLE_OPENGL

#ifdef __linux__
#define CP_OS "Linux"
#else
#define CP_OS "[unknown]"
#endif

#endif

#pragma warning(disable : 4996)

#if defined(__x86_64__) || defined(_M_AMD64)
#define CP_CPU "AMD64"
#elif defined(__i386__) || defined(_M_IX86)
#define CP_CPU "IA-32"
#else
#define CP_CPU "[unknown]"
#endif

#define CP_VERSION "1.4"

#ifdef _WIN32

#define CP_CALL_CONV __cdecl
#define CP_END_THREAD return;
typedef uintptr_t ThreadIDType;
typedef void ThreadRetType;
typedef _beginthread_proc_type ThreadFuncPtr;

#else

#define CP_CALL_CONV
#define CP_END_THREAD return NULL;
typedef pthread_t ThreadIDType;
typedef void* ThreadRetType;
typedef void* (*ThreadFuncPtr)(void*);

typedef void* HWND;
typedef void* HANDLE;
typedef void* CHAR_INFO;
typedef unsigned long DWORD;

#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20

#endif


typedef enum
{
	CM_WINAPI_GRAY,
	CM_WINAPI_16,
	CM_CSTD_GRAY,
	CM_CSTD_16,
	CM_CSTD_256,
	CM_CSTD_RGB
} ColorMode;

typedef enum
{
	SCM_DISABLED,
	SCM_WINAPI,
	SCM_CSTD_256,
	SCM_CSTD_RGB
} SetColorMode;

typedef enum
{
	CPM_NONE,
	CPM_CHAR_ONLY,
	CPM_BOTH
} ColorProcMode;

typedef enum
{
	SM_DISABLED,
	SM_DRAW_ALL,
	SM_ENABLED
} SyncMode;

typedef enum
{
	STAGE_FREE,
	STAGE_LOADED_FRAME,
	STAGE_PROCESSED_FRAME
} Stage;

typedef enum
{
	ST_VERTEX_SHADER,
	ST_FRAGMENT_SHADER,
	ST_FRAGMENT_SHADER_PRE,
	ST_FRAGMENT_SHADER_POST
} ShaderType;

typedef enum
{
	SPT_NONE,
	SPT_SIMPLE,
	SPT_COMPLEX,
	SPT_UTILS,
	SPT_FULL
} ShaderPartType;

typedef enum
{
	SS_ARGUMENT,
	SS_FILE,
	SS_PREDEF
} ShaderSource;

typedef struct
{
	Stage stage;
	bool isAudio;
	int64_t time;

	// video
	int w, h;

	// video - STAGE_LOADED_FRAME
	uint8_t* videoFrame;
	int videoLinesize;

	// video - STAGE_PROCESSED_FRAME
	void* output; // char* (C std) / CHAR_INFO* (WinAPI) / GLConsoleChar* (-fc)
	int* outputLineOffsets;

	// audio
	uint8_t* audioFrame;
	int audioFrameSize;
	int audioSamplesNum;
} Frame;

typedef struct
{
	int index;
	AVStream* stream;
	AVCodec* codec;
	AVCodecParameters* codecParameters;
	AVCodecContext* codecContext;
} Stream;

typedef struct
{
	uint8_t r, g, b;
	uint8_t ch;
} GlConsoleChar;

typedef struct
{
	int argW, argH;
	bool fillArea;
	ColorMode colorMode;
	int scanlineCount, scanlineHeight;
	double volume;
	const char* charset;
	int charsetSize;
	SetColorMode setColorMode;
	int setColorVal1, setColorVal2;
	double constFontRatio;
	int brightnessRand;
	int scalingMode;
	ColorProcMode colorProcMode;
	SyncMode syncMode;
	char* videoFilters;
	char* scaledVideoFilters;
	char* audioFilters;
	bool preload;
	bool useFakeConsole;
	bool disableKeyboard;
	bool disableCLS;
	bool disableAudio;
	bool libavLogs;
} Settings;


//main.c
extern const int QUEUE_SIZE;
extern HWND conHWND, wtDragBarHWND;
extern int conW, conH;
extern int vidW, vidH;
extern double fps;
extern bool ansiEnabled;
extern bool decodeEnd;
extern Settings settings;

//argParser.c
extern char* inputFile;
extern char* secondInputFile;

//drawFrame.c
extern HANDLE outputHandle;

//gl/glConsole.c
extern volatile float volGlCharW, volGlCharH;

//thread.c
extern const int SLEEP_ON_FREEZE;
extern volatile bool freezeThreads;
extern volatile bool mainFreezed;
extern volatile bool procFreezed;
extern volatile bool drawFreezed;


//argParser.c
extern void argumentParser(int argc, char** argv);

//decodeFrame.c
extern void initDecodeFrame(const char* file, const char* secondFile, Stream** outAudioStream);
extern void readFrames(void);
extern void avSeek(int64_t timestamp);

//processFrame.c
extern void processFrame(Frame* frame);

//drawFrame.c
extern void initDrawFrame(void);
extern void refreshSize(void);
extern void drawFrame(void* output, int* lineOffsets, int fw, int fh);

//audio.c
extern void initAudio(Stream* audioStream);
extern void addAudioFrame(AVFrame* frame);
extern void audioLoop(void);
extern void playAudio(Frame* frame);

//avFilters.c
extern void initFiltersV(Stream* videoStream);
extern void initFiltersSV(Stream* videoStream, AVFrame* scaledFrame);
extern void initFiltersA(Stream* audioStream);
extern bool applyFiltersV(AVFrame* videoFrame);
extern bool applyFiltersSV(AVFrame* scaledVideoFrame);
extern bool applyFiltersA(AVFrame* audioFrame);
extern bool getFilteredFrameV(AVFrame* filterFrame);
extern bool getFilteredFrameSV(AVFrame* filterFrame);
extern bool getFilteredFrameA(AVFrame* filterFrame);

//gl/glConsole.c
extern void initOpenGlConsole(void);
extern void refreshFont(void);
extern void drawWithOpenGL(GlConsoleChar* output, int w, int h);
extern void peekMessages(void);

//gl/glOptions.c
extern int parseGlOptions(int argc, char** argv);

//gl/glShaders.c
extern void initShaders(void);
extern void shAddShader(int stage, ShaderType type, ShaderPartType partType, char* shader, ShaderSource source, char* data);
extern void shAddUniform(int stage, char* name, float val);
extern void shSetSize(int w, int h);

//threads.c
extern void beginThreads(void);

//queue.c
extern void initQueue(void);
extern Frame* dequeueFrame(Stage fromStage, bool* threadFreezedFlag);
extern void enqueueFrame(Stage toStage);

//help.c
extern void showHelp(bool basic, bool advanced, bool modes, bool keyboard);
extern void showInfo(void);
extern void showFullInfo(void);
extern void showVersion(void);
extern void showNoArgsInfo(void);

//utils.c
extern double getTime(void);
extern ThreadIDType startThread(ThreadFuncPtr threadFunc, void* args);
extern void strToLower(char* str);
extern void getConsoleWindow(void);
extern void clearScreen(void);
extern void setDefaultColor(void);
extern void setCursorPos(int x, int y);
extern void enableANSI(void);
extern size_t getOutputArraySize(int frameW, int frameH);
extern void cpExit(int code);
extern void error(const char* description, const char* fileName, int line);

#ifdef _WIN32
extern int getWindowsArgv(char*** pargv);
#else
extern FILE* _popen(const char* command, const char* type);
extern int _pclose(FILE* stream);
extern int _getch(void);
extern void setTermios(bool deinit);
extern void Sleep(DWORD ms);
#endif