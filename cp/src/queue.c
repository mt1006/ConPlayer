#include "conplayer.h"

typedef struct
{
	int drawingPos, processingPos, loadingPos;
	int size;
	Frame* array;
} Queue;

static const int TIME_TO_WAIT = 16;
static volatile Queue queue;

static Frame* queueNextElement(int currentPos);

void initQueue(void)
{
	static bool queueExists = false;

	if (queueExists)
	{
		for (int i = 0; i < QUEUE_SIZE; i++)
		{
			if (queue.array[i].videoFrame) { free(queue.array[i].videoFrame); }
			if (queue.array[i].audioFrame) { av_free(queue.array[i].audioFrame); }
			if (queue.array[i].output) { free(queue.array[i].output); }
		}
	}
	else
	{
		queue.size = QUEUE_SIZE;
		queue.array = (Frame*)malloc(QUEUE_SIZE * sizeof(Frame));
	}

	queue.drawingPos = 0;
	queue.processingPos = 0;
	queue.loadingPos = 0;

	for (int i = 0; i < QUEUE_SIZE; i++)
	{
		queue.array[i].stage = STAGE_FREE;
		queue.array[i].isAudio = false;
		queue.array[i].time = 0;

		queue.array[i].w = -1;
		queue.array[i].h = -1;

		queue.array[i].videoFrame = NULL;
		queue.array[i].videoLinesize = 0;

		queue.array[i].output = NULL;
		queue.array[i].outputLineOffsets = NULL;

		queue.array[i].audioFrame = NULL;
		queue.array[i].audioFrameSize = -1;
		queue.array[i].audioSamplesNum = 0;
	}

	queueExists = true;
}

Frame* dequeueFrame(Stage fromStage, bool* threadFreezedFlag)
{
	int* pos;
	if (fromStage == STAGE_LOADED_FRAME) { pos = &queue.processingPos; }
	else if (fromStage == STAGE_PROCESSED_FRAME) { pos = &queue.drawingPos; }
	else { pos = &queue.loadingPos; }

	Frame* nextFrame = queueNextElement(*pos);

	while (nextFrame->stage != fromStage)
	{
		if (decodeEnd && fromStage == STAGE_PROCESSED_FRAME &&
			(nextFrame->stage == STAGE_FREE || freezeThreads))
		{
			cpExit(0);
		}

		if (freezeThreads && threadFreezedFlag)
		{
			while (freezeThreads)
			{
				*threadFreezedFlag = true;
				if (settings.useFakeConsole &&
					threadFreezedFlag == &mainFreezed) { peekMessages(); }
				Sleep(SLEEP_ON_FREEZE);
			}
			return dequeueFrame(fromStage, threadFreezedFlag);
		}

		if (settings.useFakeConsole &&
			threadFreezedFlag == &mainFreezed) { peekMessages(); }
		Sleep(TIME_TO_WAIT);
	}

	return nextFrame;
}

void enqueueFrame(Stage toStage)
{
	int* pos;
	if (toStage == STAGE_LOADED_FRAME) { pos = &queue.loadingPos; }
	else if (toStage == STAGE_PROCESSED_FRAME) { pos = &queue.processingPos; }
	else { pos = &queue.drawingPos; }

	Frame* nextFrame = queueNextElement(*pos);

	nextFrame->stage = toStage;
	(*pos)++;
	if (*pos == queue.size) { *pos = 0; }
}

static Frame* queueNextElement(int currentPos)
{
	if (currentPos + 1 == queue.size) { return &queue.array[0]; }
	else { return &queue.array[currentPos + 1]; }
}