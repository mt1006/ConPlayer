/*
* Project: ConPlayer
* Version: 1.0
* Author: https://github.com/mt1006
*/

#pragma once
#define _CRT_SECURE_NO_WARNINGS

// vcpkg install ffmpeg:x64-windows
// vcpkg install libao:x64-windows

#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/log.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <ao/ao.h>
#include <conio.h>
#include <process.h>
#include <Windows.h>

#pragma warning(disable : 4996)

#if defined(__x86_64__) || defined(_M_AMD64)
#define CP_CPU "AMD64"
#elif defined(__i386__) || defined(_M_IX86)
#define CP_CPU "IA-32"
#else
#define CP_CPU "[unknown]"
#endif

#define CP_VERSION "1.0"
#define TO_STR(x) #x
#define DEF_TO_STR(x) TO_STR(x)

#ifdef _WIN32

#define USE_WCHAR 1
typedef wchar_t unichar;
#define uc(str) L##str
#define uc_strlen wcslen
#define uc_fopen _wfopen
#define uc_puts _putws
#define uc_fputs fputws

#else

#define USE_WCHAR 0
typedef char unichar;
#define uc(str) str
#define uc_strlen strlen
#define uc_fopen fopen
#define uc_puts puts
#define uc_fputs fputs

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
	STAGE_FREE,
	STAGE_LOADED_FRAME,
	STAGE_PROCESSED_FRAME
} Stage;

typedef struct
{
	Stage stage;
	int isAudio;
	int audioSamplesNum;
	uint8_t* videoFrame;
	uint8_t* audioFrame;
	int videoLinesize;
	void* output; // char* (video - C std) / CHAR_INFO* (video - WinAPI)
	int* outputLineOffsets;
	int audioFrameSize;
	int frameW, frameH;
	int64_t time;
} Frame;

typedef struct
{
	int drawingPos, processingPos, loadingPos;
	int size;
	Frame* array;
} Queue;

typedef struct
{
	int index;
	AVStream* stream;
	AVCodec* codec;
	AVCodecParameters* codecParameters;
	AVCodecContext* codecContext;
} Stream;

extern const int QUEUE_SIZE;

extern int w, h;
extern int conW, conH;
extern int fontW, fontH;
extern int vidW, vidH;
extern int argW, argH;
extern int fillArea;
extern ColorMode colorMode;
extern int scanlineCount, scanlineHeight;
extern double volume;
extern double fps;
extern int decodeEnd;
extern char* charset;
extern int charsetSize;

//argParser.c
extern char* argumentParser(int argc, unichar** argv);

//decodeFrame.c
extern void initAV(const char* file, Stream** outAudioStream);
extern void readFrames(void);
extern void avSeek(int64_t timestamp);
extern void unload(void);

//processFrame.c
extern void initProcessFrame(void);
extern void processFrame(Frame* frame);

//drawFrame.c
extern void initDrawFrame(void);
extern void refreshSize(void);
extern void drawFrame(void* output, int* lineOffsets, int fw, int fh);

//audio.c
extern void initAudio(Stream* audioStream);
extern void addAudio(AVFrame* frame);
extern void playAudio(Frame* frame);
extern void deinitAudio(void);

//threads.c
extern void beginThreads(void);
extern void resetTimer(void);

//queue.c
extern void initQueue(void);
extern Frame* dequeueFrame(Stage fromStage);
extern void enqueueFrame(Stage toStage);

//help.c
extern void showHelp(int basic, int advanced, int colorModes, int keyboard);
extern void showInfo(void);
extern void showFullInfo(void);
extern void showVersion(void);

//utils.c
extern double getTime(void);
extern void clearScreen(HANDLE handle);
extern void setDefaultColor(void);
extern size_t getOutputArraySize(void);
extern uint8_t rgbToAnsi256(uint8_t r, uint8_t g, uint8_t b);
extern ColorMode colorModeFromStr(char* str);
extern int utf8ArraySize(unichar* input, int inputSize);
extern void unicharArrayToUTF8(unichar* input, char* output, int inputSize);
extern char* toUTF8(unichar* input, int inputLen);
extern void error(const char* description, const char* fileName, int line);