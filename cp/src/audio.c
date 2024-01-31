#include "conplayer.h"

typedef struct
{
	int front, back;
	uint8_t** audioFrames;
	int* audioSamplesNum;
	int* audioFramesSize;
} AudioQueue;

#ifdef _WIN32

static const enum AVSampleFormat SAMPLE_FORMAT_AV = AV_SAMPLE_FMT_S32;
static const int SAMPLE_BITS = 32;
static const int CHANNELS = 2;
static const int SAMPLE_SIZE = 8; // (32 bits * 2 channels) / 8 bits
static const int SAMPLE_RATE = 48000;

#else

static const enum AVSampleFormat SAMPLE_FORMAT_AV = AV_SAMPLE_FMT_S16;
static const int SAMPLE_BITS = 16;
static const int CHANNELS = 2;
static const int SAMPLE_SIZE = 4; // (16 bits * 2 channels) / 8 bits
static const int SAMPLE_RATE = 48000;

#endif

static bool initialized = false;
static SwrContext* resampleContext = NULL;

static const int AUDIO_QUEUE_SIZE = 128;
static volatile AudioQueue audioQueue;

static ao_sample_format aoSampleFormat;
static ao_device* aoDevice = NULL;

static bool initAudioLib(void);

void initAudio(Stream* avAudioStream)
{
	if (!avAudioStream) { return; }
	if (!initAudioLib()) { return; }

	resampleContext = swr_alloc_set_opts(NULL,
		av_get_default_channel_layout(CHANNELS),                               //out_channel_layout
		SAMPLE_FORMAT_AV,                                                      //out_sample_fmt
		SAMPLE_RATE,                                                           //out_sample_rate
		av_get_default_channel_layout(avAudioStream->codecContext->channels),  //in_channel_layout
		avAudioStream->codecContext->sample_fmt,                               //in_sample_fmt
		avAudioStream->codecContext->sample_rate,                              //in_sample_rate
		0, NULL);
	int swrRetVal = swr_init(resampleContext);
	if (swrRetVal < 0) { return; }

	audioQueue.front = 0;
	audioQueue.back = 0;
	audioQueue.audioFrames = (uint8_t**)calloc(AUDIO_QUEUE_SIZE, sizeof(uint8_t*));
	audioQueue.audioSamplesNum = (int*)malloc(AUDIO_QUEUE_SIZE * sizeof(int));
	audioQueue.audioFramesSize = (int*)calloc(AUDIO_QUEUE_SIZE, sizeof(int));

	initialized = true;
}

void addAudioFrame(AVFrame* frame)
{
	if (!initialized) { return; }

	Frame* queueFrame = dequeueFrame(STAGE_FREE, NULL);
	
	int outSamples = swr_get_out_samples(resampleContext, frame->nb_samples);
	if (queueFrame->audioFrameSize < outSamples)
	{
		queueFrame->audioFrameSize = outSamples * 2; // just for prevention
		if (queueFrame->audioFrame) { av_free(queueFrame->audioFrame); }
		av_samples_alloc(&queueFrame->audioFrame, NULL, CHANNELS,
			queueFrame->audioFrameSize, SAMPLE_FORMAT_AV, 0);
	}

	outSamples = swr_convert(resampleContext, &queueFrame->audioFrame, outSamples,
		frame->extended_data, frame->nb_samples);
	if (outSamples < 0) { return; }

	queueFrame->audioSamplesNum = outSamples;
	queueFrame->isAudio = true;

	enqueueFrame(STAGE_LOADED_FRAME);
}

void audioLoop(void)
{
	const int TIME_TO_SLEEP = 8;
	if (!initialized) { return; }

	while (true)
	{
		if (audioQueue.front != audioQueue.back)
		{
			#ifdef _WIN32

			int* fullSamples = (int*)audioQueue.audioFrames[audioQueue.front];
			for (int i = 0; i < audioQueue.audioSamplesNum[audioQueue.front]; i++)
			{
				fullSamples[i * 2] = (int)((double)fullSamples[i * 2] * settings.volume);
				fullSamples[i * 2 + 1] = (int)((double)fullSamples[i * 2 + 1] * settings.volume);
			}

			#else

			short* fullSamples = (short*)audioQueue.audioFrames[audioQueue.front];
			for (int i = 0; i < audioQueue.audioSamplesNum[audioQueue.front]; i++)
			{
				fullSamples[i * 2] = (short)((double)fullSamples[i * 2] * settings.volume);
				fullSamples[i * 2 + 1] = (short)((double)fullSamples[i * 2 + 1] * settings.volume);
			}

			#endif

			ao_play(aoDevice, audioQueue.audioFrames[audioQueue.front],
				audioQueue.audioSamplesNum[audioQueue.front] * SAMPLE_SIZE);
			
			audioQueue.front++;
			if (audioQueue.front == AUDIO_QUEUE_SIZE) { audioQueue.front = 0; }
		}

		Sleep(TIME_TO_SLEEP);
	}
}

void playAudio(Frame* frame)
{
	while ((audioQueue.back + 1 == audioQueue.front) ||
		(audioQueue.back == AUDIO_QUEUE_SIZE - 1 && audioQueue.front == 0))
	{
		Sleep(0);
	}

	if (audioQueue.audioFramesSize[audioQueue.back] < frame->audioFrameSize)
	{
		if (audioQueue.audioFrames[audioQueue.back])
		{
			av_free(audioQueue.audioFrames[audioQueue.back]);
		}
		av_samples_alloc(&audioQueue.audioFrames[audioQueue.back], NULL,
			CHANNELS, frame->audioFrameSize, SAMPLE_FORMAT_AV, 0);
		audioQueue.audioFramesSize[audioQueue.back] = frame->audioFrameSize;
	}

	audioQueue.audioSamplesNum[audioQueue.back] = frame->audioSamplesNum;
	av_samples_copy(&audioQueue.audioFrames[audioQueue.back],
		&frame->audioFrame, 0, 0, frame->audioSamplesNum,
		CHANNELS, SAMPLE_FORMAT_AV);

	audioQueue.back++;
	if (audioQueue.back == AUDIO_QUEUE_SIZE) { audioQueue.back = 0; }
}

static bool initAudioLib(void)
{
	ao_initialize();
	int driver = ao_default_driver_id();

	memset(&aoSampleFormat, 0, sizeof(ao_sample_format));
	aoSampleFormat.bits = SAMPLE_BITS;
	aoSampleFormat.channels = CHANNELS;
	aoSampleFormat.rate = SAMPLE_RATE;
	aoSampleFormat.byte_format = AO_FMT_LITTLE;
	aoSampleFormat.matrix = 0;
	aoDevice = ao_open_live(driver, &aoSampleFormat, NULL);

	return aoDevice != NULL;
}