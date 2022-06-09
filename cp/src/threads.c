#include "conplayer.h"

const double TIME_TO_RESET_TIMER = 0.5;
const int SLEEP_ON_FREEZE = 4;

uintptr_t procThreadID = 0;
uintptr_t drawThreadID = 0;
uintptr_t keyboardThreadID = 0;
double startTime;
int frameCounter;
int64_t drawFrameTime = 0;
int paused = 0;

int freezeThreads = 0;
int mainFreezed = 0;
int procFreezed = 0;
int drawFreezed = 0;

static void __cdecl procThread(void* ptr);
static void __cdecl drawThread(void* ptr);
static void __cdecl keyboardThread(void* ptr);
static void seek(int64_t timestamp);

void beginThreads()
{
	procThreadID = _beginthread(&procThread, 0, NULL);
	drawThreadID = _beginthread(&drawThread, 0, NULL);
	keyboardThreadID = _beginthread(&keyboardThread, 0, NULL);
}

void resetTimer()
{
	frameCounter = 0;
}

static void __cdecl procThread(void* ptr)
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

static void __cdecl drawThread(void* ptr)
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
			drawFrame(frame->output, frame->frameW, frame->frameH);
			frameCounter++;
		}
		enqueueFrame(STAGE_FREE);
	}
}

static void __cdecl keyboardThread(void* ptr)
{
	const double SEEK_SECONDS = 10.0;
	const double KEYBOARD_DELAY = 0.2;
	static double keyTime = 0.0;

	int64_t timeToSet;
	while (1)
	{
		unsigned char key = _getch();

		double newKeyTime = getTime();
		if (newKeyTime < keyTime + KEYBOARD_DELAY) { continue; }
		keyTime = newKeyTime;

		switch (key)
		{
		case VK_ESCAPE:
			exit(0);
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
			Sleep(KEYBOARD_DELAY);
			break;
		case ']':
			paused = 0;
			timeToSet = drawFrameTime;
			timeToSet += (int64_t)(SEEK_SECONDS * AV_TIME_BASE);
			if (timeToSet < 0) { timeToSet = 0; }
			seek(timeToSet);
			Sleep(KEYBOARD_DELAY);
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