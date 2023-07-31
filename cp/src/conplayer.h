/*
* Project: ConPlayer
* Version: 1.5
* Author: https://github.com/mt1006
*/

#pragma once
#define _CRT_SECURE_NO_WARNINGS

/*
*  vcpkg (WinXP):
*    vcpkg install libao:x86-windows
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <float.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavutil/imgutils.h"
#include "libavutil/file.h"
#include "libavutil/opt.h"
#include "libavutil/log.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include <ao/ao.h>
#include "dependencies/atomic.h"

#ifdef _WIN32

#include <conio.h>
#include <process.h>
#include <Windows.h>
#include <direct.h>
#include "dependencies/win_dirent.h"

#ifndef CP_DISABLE_OPENGL
#include <gl/GL.h>
#include <gl/GLU.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#endif

#define CP_OS "WinXP"

#else

#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sched.h>
#include <dirent.h>

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

#define CP_VERSION "1.5"
#define CP_VERSION_STRING "ConPlayer " CP_VERSION " [" CP_CPU "/" CP_OS "]"

#ifdef _WIN32

#define CP_IS_WINDOWS true
#define CP_CALL_CONV __cdecl
#define CP_END_THREAD return;
typedef uintptr_t ThreadIDType;
typedef void ThreadRetType;
typedef _beginthread_proc_type ThreadFuncPtr;

#else

#define CP_IS_WINDOWS false
#define CP_CALL_CONV
#define CP_END_THREAD return NULL;
typedef pthread_t ThreadIDType;
typedef void* ThreadRetType;
typedef void* (*ThreadFuncPtr)(void*);

typedef void* HWND;
typedef void* HANDLE;
typedef void* CHAR_INFO;
typedef unsigned long DWORD;

#define VK_ESCAPE   0x1B
#define VK_SPACE    0x20
#define VK_PRIOR    0x21 //page up
#define VK_NEXT     0x22 //page down
#define VK_END      0x23
#define VK_HOME     0x24
#define VK_LEFT     0x25
#define VK_UP       0x26
#define VK_RIGHT    0x27
#define VK_DOWN     0x28
#define VK_RETURN   0x0D

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
	SM_NEAREST,
	SM_FAST_BILINEAR,
	SM_BILINEAR,
	SM_BICUBIC
} ScalingMode;

typedef enum
{
	CPM_NONE,
	CPM_CHAR_ONLY,
	CPM_BOTH
} ColorProcMode;

typedef enum
{
	SYNC_DISABLED,
	SYNC_DRAW_ALL,
	SYNC_ENABLED
} SyncMode;

typedef enum
{
	STAGE_FREE,
	STAGE_LOADED_FRAME,
	STAGE_PROCESSED_FRAME
} Stage;

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
	ScalingMode scalingMode;
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
extern const char* CHARSET_LONG;
extern const char* CHARSET_SHORT;
extern const char* CHARSET_2;
extern const char* CHARSET_BLOCKS;
extern const char* CHARSET_OUTLINE;
extern const char* CHARSET_BOLD_OUTLINE;
extern char* inputFile;
extern char* secondInputFile;

//drawFrame.c
extern HANDLE outputHandle;

//thread.c
extern const int SLEEP_ON_FREEZE;
extern volatile bool freezeThreads;
extern volatile bool mainFreezed;
extern volatile bool procFreezed;
extern volatile bool drawFreezed;

//help.c
extern const char* INFO_MESSAGE;


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

//utils.c
extern double getTime(void);
extern ThreadIDType startThread(ThreadFuncPtr threadFunc, void* args);
extern void strToLower(char* str);
extern void getConsoleWindow(void);
extern void clearScreen(void);
extern void setDefaultColor(void);
extern void setCursorPos(int x, int y);
extern void getConsoleSize(int* w, int* h);
extern void enableANSI(void);
extern size_t getOutputArraySize(int frameW, int frameH);
extern void cpExit(int code);
extern void error(const char* description, const char* fileName, int line);
extern int getChar(bool wasdAsArrows);

#ifdef _WIN32
extern int getWindowsArgv(char*** pargv);
extern int parseStringAsArgv(char* str, char*** pargv);
#else
extern FILE* _popen(const char* command, const char* type);
extern int _pclose(FILE* stream);
extern void Sleep(DWORD ms);
#endif



// UI

typedef enum
{
	UI_TEXT,
	UI_ACTION,
	UI_SUBMENU,
	UI_SELECTOR,
	UI_VALUE,
	UI_SWITCH,
	UI_FILE_SELECTOR
} UIMenuElementType;

typedef enum
{
	UI_SELECTOR_GET_COUNT,
	UI_SELECTOR_GET_POS,
	UI_SELECTOR_GET_NAME,
	UI_SELECTOR_GET_SELECTED_NAME,
	UI_SELECTOR_SELECT
} UISelectorAction;

typedef enum
{
	UI_VALUE_PRINT,
	UI_VALUE_GET
} UIValueAction;

typedef struct
{
	char* currentPath;
	char** out;
} UIFileSelectorData;

typedef struct
{
	UIMenuElementType type;
	const char* text;
	void* ptr;
} UIMenuElement;

typedef struct
{
	UIMenuElement* elements;
	int count;
	int selected;
	int shift;
	int oldH;
} UIMenu;


//ui/ui.c
extern bool uiKeepLoop;


//ui/ui.c
extern void uiShowRunScreen(void);
extern void uiAddElement(UIMenu* menu, UIMenuElementType type, const char* text, void* ptr);
extern void uiClearMenu(UIMenu* menu);

//ui/menu.c
extern void uiMenuLoop(UIMenu* menu);
extern void uiPushMenu(UIMenu* menu);
extern void uiPopMenu(void);



// OpenGL

#ifndef CP_DISABLE_OPENGL

// https://registry.khronos.org/OpenGL/api/GL/glext.h
#define CP_GL_FRAGMENT_SHADER     0x8B30
#define CP_GL_VERTEX_SHADER       0x8B31
#define CP_GL_COMPILE_STATUS      0x8B81
#define CP_GL_LINK_STATUS         0x8B82
#define CP_GL_INFO_LOG_LENGTH     0x8B84
#define CP_GL_FRAMEBUFFER         0x8D40
#define CP_GL_RENDERBUFFER        0x8D41
#define CP_GL_COLOR_ATTACHMENT0   0x8CE0

typedef GLint* GLintptr;
typedef GLsizei* GLsizeiptr;

typedef struct
{
	GLuint (APIENTRY* createShader)(GLenum type);
	void   (APIENTRY* shaderSource)(GLuint shader, GLsizei count, const char* const* string, const GLint* length);
	void   (APIENTRY* compileShader)(GLuint shader);
	GLuint (APIENTRY* createProgram)(void);
	void   (APIENTRY* attachShader)(GLuint program, GLuint shader);
	void   (APIENTRY* linkProgram)(GLuint program);
	void   (APIENTRY* useProgram)(GLuint program);
	GLint  (APIENTRY* getUniformLocation)(GLuint program, const char* name);
	void   (APIENTRY* uniform1f)(GLint location, GLfloat v0);
	void   (APIENTRY* uniform2i)(GLint location, GLint v0, GLint v1);
	void   (APIENTRY* getShaderiv)(GLuint shader, GLenum pname, GLint* params);
	void   (APIENTRY* getProgramiv)(GLuint program, GLenum pname, GLint* params);
	void   (APIENTRY* getShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, char* infoLog);
	void   (APIENTRY* getProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei* length, char* infoLog);

	void   (APIENTRY* genFramebuffers)(GLsizei n, GLuint* framebuffers);
	void   (APIENTRY* bindFramebuffer)(GLenum target, GLuint framebuffer);
	void   (APIENTRY* genRenderbuffers)(GLsizei n, GLuint* renderbuffers);
	void   (APIENTRY* bindRenderbuffer)(GLenum target, GLuint renderbuffer);
	void   (APIENTRY* renderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
	void   (APIENTRY* framebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
} GlFunctions;


typedef enum
{
	ST_VERTEX_SHADER,
	ST_FRAGMENT_SHADER,
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

typedef enum
{
	GLWT_MAIN,
	GLWT_MAIN_CHILD,
	GLWT_DUMMY
} GlWindowType;

typedef struct
{
	uint8_t r, g, b;
	uint8_t ch;
} GlConsoleChar;

typedef struct
{
	const char* vshBegin;
	const char* vshMainBegin;
	const char* vshMainEnd;
	const char* fshBegin;
	const char* fshMainBegin;
	const char* fshMainMiddle;
	const char* fshMainEnd;
} ShadersStructure;

typedef struct
{
	HWND hwnd;
	HDC hdc;
	MSG msg;
	HGLRC hglrc;
	uint8_t* bitmap;
	int bmpW, bmpH;
	GLint maxTextureSize;
} GlWindow;

typedef struct
{
	GLuint texture;
	int x, y, w, h;
	int texW, texH;
	float texRatioW, texRatioH;
	uint8_t* data;
	int dataW, dataH;
} GlImagePart;

typedef struct
{
	GlImagePart* parts;
	int count;
	int w, h;
} GlImageParts;


//gl/glConsole.c
extern volatile float volGlCharW, volGlCharH;

//gl/glUtils.c
extern GlFunctions glf[4];

//gl/shaders/
extern bool shStageEnabled[4];


//gl/glConsole.c
extern void initOpenGlConsole(void);
extern void refreshFont(void);
extern void drawWithOpenGL(GlConsoleChar* output, int w, int h);
extern void peekMainMessages(void);

//gl/glOptions.c
extern int parseGlOptions(int argc, char** argv);

//gl/glUtils.c
extern GlWindow initGlWindow(GlWindowType type, int stage);
extern void peekWindowMessages(GlWindow* glw);
extern uint8_t* getGlBitmap(GlWindow* glw, int w, int h);
extern GLuint createTexture(void);
extern void setTexture(uint8_t* data, int w, int h, int format, GLuint id);
extern void setPixelMatrix(int w, int h);
extern void getImageParts(AVFrame* frame, GlImageParts* parts, int maxTextureSize);
extern void calcTextureSize(int w, int h, int* outW, int* outH, float* ratioW, float* ratioH, bool invertH);

//gl/shaders/glShaders.c
extern GLuint shLoadStageShaders(int stage, ShadersStructure* structure);
extern void shAddShader(int stage, ShaderType type, ShaderPartType partType, char* shader, ShaderSource source, char* data);
extern void shAddUniform(int stage, char* name, float val);
extern GLint shGetUniformLocation(GlFunctions* f, GLuint program, const char* prefix, const char* name);

//gl/shaders/glShStage1.c
extern void shStage1_init(void);
extern void shStage1_apply(AVFrame* videoFrame);

//gl/shaders/glShStage3.c
extern void shStage3_init(void);
extern void shStage3_setSize(int w, int h);

#endif