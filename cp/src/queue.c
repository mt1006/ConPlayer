#include "conplayer.h"

static const int TIME_TO_WAIT = 16;
static Queue queue;

static Frame* queueNextElement(int currentPos);

void initQueue(void)
{
	static int queueExists = 0;

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
		queue.array[i].isAudio = 0;
		queue.array[i].audioSamplesNum = 0;
		queue.array[i].videoFrame = NULL;
		queue.array[i].audioFrame = NULL;
		queue.array[i].videoLinesize = 0;
		queue.array[i].output = NULL;
		queue.array[i].audioFrameSize = -1;
		queue.array[i].frameW = -1;
		queue.array[i].frameH = -1;
		queue.array[i].time = 0;
	}

	queueExists = 1;
}

Frame* dequeueFrame(Stage fromStage)
{
	int* pos;
	if (fromStage == STAGE_LOADED_FRAME) { pos = &queue.processingPos; }
	else if (fromStage == STAGE_PROCESSED_FRAME) { pos = &queue.drawingPos; }
	else { pos = &queue.loadingPos; }

	Frame* nextFrame = queueNextElement(*pos);
	while (nextFrame->stage != fromStage)
	{
		if (decodeEnd && fromStage == STAGE_PROCESSED_FRAME) { exit(0); }
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