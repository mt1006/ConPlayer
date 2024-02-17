#include "conplayer.h"

typedef struct
{
	void* output;
	int* outputLineOffsets;
	int w, h;
} ConsoleFrame;

const int SLEEP_ON_FREEZE = 4;
volatile bool freezeThreads = false;
volatile bool mainFreezed = false;
volatile bool procFreezed = false;
volatile bool drawFreezed = false;

static const double TIME_TO_RESET_TIMER = 0.5;

static ThreadIDType procThreadID = 0;
static ThreadIDType drawThreadID = 0;
static ThreadIDType audioThreadID = 0;
static ThreadIDType consoleThreadID = 0;
static ThreadIDType keyboardThreadID = 0;
static double startTime;
static int frameCounter;
static int64_t drawFrameTime = 0;
static volatile bool paused = false;
static volatile ConsoleFrame consoleFrame;
static volatile bool waitingForFrame = false;

static ThreadRetType CP_CALL_CONV procThread(void* ptr);
static ThreadRetType CP_CALL_CONV drawThread(void* ptr);
static ThreadRetType CP_CALL_CONV audioThread(void* ptr);
static ThreadRetType CP_CALL_CONV consoleThread(void* ptr);
static ThreadRetType CP_CALL_CONV keyboardThread(void* ptr);
static void seek(int64_t timestamp);

void beginThreads(void)
{
	consoleFrame.output = NULL;
	consoleFrame.outputLineOffsets = NULL;
	consoleFrame.w = -1;
	consoleFrame.h = -1;

	procThreadID = startThread(&procThread, NULL);
	drawThreadID = startThread(&drawThread, NULL);
	if (!settings.disableAudio) { audioThreadID = startThread(&audioThread, NULL); }
	if (settings.syncMode == SYNC_ENABLED) { consoleThreadID = startThread(&consoleThread, NULL); }
	if (!settings.disableKeyboard) { keyboardThreadID = startThread(&keyboardThread, NULL); }
}

static ThreadRetType CP_CALL_CONV procThread(void* ptr)
{
	while (true)
	{
		while (freezeThreads && !decodeEnd)
		{
			procFreezed = true;
			Sleep(SLEEP_ON_FREEZE);
		}

		Frame* frame = dequeueFrame(STAGE_LOADED_FRAME, &procFreezed);
		if (!frame->isAudio)
		{
			processFrame(frame);
		}
		enqueueFrame(STAGE_PROCESSED_FRAME);
	}

	CP_END_THREAD
}

static ThreadRetType CP_CALL_CONV drawThread(void* ptr)
{
	const int SLEEP_ON_PAUSE = 10;

	while (true)
	{
		while (freezeThreads && !decodeEnd)
		{
			drawFreezed = true;
			Sleep(SLEEP_ON_FREEZE);
		}

		while (paused)
		{
			frameCounter = 0;
			Sleep(SLEEP_ON_PAUSE);
		}

		Frame* frame = dequeueFrame(STAGE_PROCESSED_FRAME, &drawFreezed);
		if (settings.syncMode == SYNC_DISABLED)
		{
			if (!frame->isAudio)
			{
				drawFrameTime = frame->time;
				drawFrame(frame->output, frame->outputLineOffsets,
					frame->w, frame->h);
			}
		}
		else
		{
			if (frame->isAudio)
			{
				if (!settings.disableAudio) { playAudio(frame); }
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

				if (settings.syncMode == SYNC_DRAW_ALL)
				{
					drawFrame(frame->output, frame->outputLineOffsets,
						frame->w, frame->h);
				}
				else
				{
					if (waitingForFrame)
					{
						int outputArraySize = (int)getOutputArraySize(frame->w, frame->h);
						int lineOffsetsArraySize = (frame->h + 1) * sizeof(int);

						if (frame->w != consoleFrame.w ||
							frame->h != consoleFrame.h)
						{
							if (consoleFrame.output) { free(consoleFrame.output); }
							if (consoleFrame.outputLineOffsets) { free(consoleFrame.outputLineOffsets); }

							consoleFrame.output = malloc(outputArraySize);
							consoleFrame.outputLineOffsets = (int*)malloc(lineOffsetsArraySize);
							consoleFrame.w = frame->w;
							consoleFrame.h = frame->h;
						}

						memcpy(consoleFrame.output, frame->output, outputArraySize);
						memcpy(consoleFrame.outputLineOffsets, frame->outputLineOffsets,
							lineOffsetsArraySize);

						waitingForFrame = false;
					}
				}
				
				frameCounter++;
			}
		}

		enqueueFrame(STAGE_FREE);
	}

	CP_END_THREAD
}

static ThreadRetType CP_CALL_CONV consoleThread(void* ptr)
{
	while (true)
	{
		waitingForFrame = true;
		while (waitingForFrame) { Sleep(0); }

		drawFrame(consoleFrame.output,
			consoleFrame.outputLineOffsets,
			consoleFrame.w, consoleFrame.h);
	}

	CP_END_THREAD
}

static ThreadRetType CP_CALL_CONV audioThread(void* ptr)
{
	audioLoop();
	CP_END_THREAD
}

static ThreadRetType CP_CALL_CONV keyboardThread(void* ptr)
{
	const int SLEEP_ON_START = 300;
	const double SEEK1_SECONDS = 10.0;
	const double SEEK2_SECONDS = 30.0;
	const double VOLUME_CHANGE = 0.05;
	const double KEYBOARD_DELAY = 0.2;

	Sleep(SLEEP_ON_START);

	double keyTime = 0.0;
	double mutedVolume = 0.0;

	while (true)
	{
		unsigned char key = getChar(true);

		double newKeyTime = getTime();
		if (newKeyTime < keyTime + KEYBOARD_DELAY) { continue; }
		keyTime = newKeyTime;

		double newVolume = settings.volume;

		switch (key)
		{
		case VK_ESCAPE:
			cpExit(0);
			break;

		case VK_SPACE:
			paused = !paused;
			break;

		case VK_LEFT:
			seek(drawFrameTime - (int64_t)(SEEK1_SECONDS * AV_TIME_BASE));
			break;

		case VK_RIGHT:
			seek(drawFrameTime + (int64_t)(SEEK1_SECONDS * AV_TIME_BASE));
			break;

		case '-':
			seek(drawFrameTime - (int64_t)(SEEK2_SECONDS * AV_TIME_BASE));
			break;

		case '=':
			seek(drawFrameTime + (int64_t)(SEEK2_SECONDS * AV_TIME_BASE));
			break;

		case VK_DOWN:
			mutedVolume = 0.0;
			newVolume -= VOLUME_CHANGE;
			if (newVolume < 0.0) { newVolume = 0.0; }
			settings.volume = newVolume;
			break;

		case VK_UP:
			mutedVolume = 0.0;
			newVolume += VOLUME_CHANGE;
			if (newVolume > 1.0) { newVolume = 1.0; }
			settings.volume = newVolume;
			break;

		case 'm':
			newVolume = mutedVolume;
			mutedVolume = settings.volume;
			settings.volume = newVolume;
			break;
		}
	}

	CP_END_THREAD
}

static void seek(int64_t timestamp)
{
	freezeThreads = true;
	paused = false;
	if (timestamp < 0) { timestamp = 0; }

	while (!mainFreezed || !procFreezed || !drawFreezed) { Sleep(SLEEP_ON_FREEZE); }
	while (decodeEnd) { Sleep(30); }
	initQueue();
	avSeek(timestamp);

	freezeThreads = false;
	mainFreezed = false;
	procFreezed = false;
	drawFreezed = false;
}