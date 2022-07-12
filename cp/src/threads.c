#include "conplayer.h"

#ifdef _WIN32

#define CALL_CONV __cdecl
typedef uintptr_t ThreadIDType;
typedef void ThreadRetType;

#else

#define CALL_CONV
typedef pthread_t ThreadIDType;
typedef void* ThreadRetType;

#endif

const int SLEEP_ON_FREEZE = 4;
int freezeThreads = 0;
int mainFreezed = 0;
int procFreezed = 0;
int drawFreezed = 0;

static const double TIME_TO_RESET_TIMER = 0.5;

static ThreadIDType procThreadID = 0;
static ThreadIDType drawThreadID = 0;
static ThreadIDType keyboardThreadID = 0;
static double startTime;
static int frameCounter;
static int64_t drawFrameTime = 0;
static int paused = 0;

static ThreadRetType CALL_CONV procThread(void* ptr);
static ThreadRetType CALL_CONV drawThread(void* ptr);
static ThreadRetType CALL_CONV keyboardThread(void* ptr);
static void seek(int64_t timestamp);

void beginThreads(void)
{
	#ifdef _WIN32

	procThreadID = _beginthread(&procThread, 0, NULL);
	drawThreadID = _beginthread(&drawThread, 0, NULL);
	if (!disableKeyboard)
	{
		keyboardThreadID = _beginthread(&keyboardThread, 0, NULL);
	}

	#else

	pthread_create(&procThreadID, NULL, &procThread, NULL);
	pthread_create(&drawThreadID, NULL, &drawThread, NULL);
	if (!disableKeyboard)
	{
		pthread_create(&keyboardThreadID, NULL, &keyboardThread, NULL);
	}

	#endif
}

void resetTimer(void)
{
	frameCounter = 0;
}

static ThreadRetType CALL_CONV procThread(void* ptr)
{
	while (1)
	{
		while (freezeThreads && !decodeEnd)
		{
			procFreezed = 1;
			Sleep(SLEEP_ON_FREEZE);
		}

		Frame* frame = dequeueFrame(STAGE_LOADED_FRAME);
		if (!frame->isAudio)
		{
			processFrame(frame);
		}
		enqueueFrame(STAGE_PROCESSED_FRAME);
	}
}

static ThreadRetType CALL_CONV drawThread(void* ptr)
{
	const int SLEEP_ON_PAUSE = 10;

	while (1)
	{
		while (freezeThreads && !decodeEnd)
		{
			drawFreezed = 1;
			Sleep(SLEEP_ON_FREEZE);
		}

		while (paused)
		{
			resetTimer();
			Sleep(SLEEP_ON_PAUSE);
		}

		Frame* frame = dequeueFrame(STAGE_PROCESSED_FRAME);
		if (disableSync)
		{
			if (!frame->isAudio)
			{
				drawFrameTime = frame->time;
				drawFrame(frame->output, frame->outputLineOffsets,
					frame->frameW, frame->frameH);
			}
		}
		else
		{
			if (frame->isAudio)
			{
				playAudio(frame);
			}
			else
			{
				drawFrameTime = frame->time;
				if (!frameCounter) { startTime = getTime(); }
				double curTime = getTime() - startTime;
				int curFrame = (int)(curTime * fps);
				if (curFrame <= frameCounter)
				{
					double timeToSleep = ((frameCounter + 1) / fps) - curTime;
					Sleep((DWORD)(timeToSleep * 1000.0));
				}
				drawFrame(frame->output, frame->outputLineOffsets,
					frame->frameW, frame->frameH);
				frameCounter++;
			}
		}
		enqueueFrame(STAGE_FREE);
	}
}

static ThreadRetType CALL_CONV keyboardThread(void* ptr)
{
	const int SLEEP_ON_START = 300;
	const double SEEK_SECONDS = 10.0;
	const double VOLUME_CHANGE = 0.05;
	const double KEYBOARD_DELAY = 0.2;
	static double keyTime = 0.0;

	Sleep(SLEEP_ON_START);

	int64_t timeToSet;
	while (1)
	{
		unsigned char key = _getch();

		double newKeyTime = getTime();
		if (newKeyTime < keyTime + KEYBOARD_DELAY) { continue; }
		keyTime = newKeyTime;

		double newVolume = volume;

		switch (key)
		{
		case VK_ESCAPE:
			cpExit(0);
			break;
		case VK_SPACE:
			paused = !paused;
			break;
		case '[':
			paused = 0;
			timeToSet = drawFrameTime;
			timeToSet -= (int64_t)(SEEK_SECONDS * AV_TIME_BASE);
			if (timeToSet < 0) { timeToSet = 0; }
			seek(timeToSet);
			break;
		case ']':
			paused = 0;
			timeToSet = drawFrameTime;
			timeToSet += (int64_t)(SEEK_SECONDS * AV_TIME_BASE);
			if (timeToSet < 0) { timeToSet = 0; }
			seek(timeToSet);
			break;
		case 'l':
			newVolume -= VOLUME_CHANGE;
			if (newVolume < 0.0) { newVolume = 0.0; }
			volume = newVolume;
			break;
		case 'o':
			newVolume += VOLUME_CHANGE;
			if (newVolume > 1.0) { newVolume = 1.0; }
			volume = newVolume;
			break;
		}
	}
}

static void seek(int64_t timestamp)
{
	freezeThreads = 1;

	while (!mainFreezed || !procFreezed || !drawFreezed) { Sleep(SLEEP_ON_FREEZE); }
	while (decodeEnd) { Sleep(30); }
	initQueue();
	avSeek(timestamp);

	freezeThreads = 0;
	mainFreezed = 0;
	procFreezed = 0;
	drawFreezed = 0;
}