#include "conplayer.h"

typedef struct
{
	int front, back;
	uint8_t** audioFrames;
	int* audioSamplesNum;
	int* audioFramesSize;
} AudioQueue;

#ifdef CP_USE_PORTAUDIO

static const enum AVSampleFormat SAMPLE_FORMAT_AV = AV_SAMPLE_FMT_FLT;
static const PaSampleFormat SAMPLE_FORMAT_PA = paFloat32;
static const int SAMPLE_BITS = 32;
static const int CHANNELS = 2;
static const int SAMPLE_RATE = 48000;

static PaStream* stream;
static PaStreamParameters audioParameters;

#else

static const enum AVSampleFormat SAMPLE_FORMAT_AV = AV_SAMPLE_FMT_S16;
static const int SAMPLE_BITS = 16;
static const int CHANNELS = 2;
static const int SAMPLE_SIZE = 4; // (16 bits * 2 channels) / 8 bits
static const int SAMPLE_RATE = 48000;

static ao_sample_format aoSampleFormat;
static ao_device* aoDevice = NULL;

#endif

static int initialized = 0;
static int libInitialized = 0;
static SwrContext* resampleContext = NULL;

static const int AUDIO_QUEUE_SIZE = 128;
static AudioQueue audioQueue;

static ThreadRetType CP_CALL_CONV initPortAudio(void* ptr);
static ThreadRetType CP_CALL_CONV initLibao(void* ptr);

void initAudio(Stream* avAudioStream)
{
	const int MS_TO_WAIT = 8;
	while (!libInitialized) { Sleep(MS_TO_WAIT); }

	if (libInitialized == -1) { return; }

	resampleContext = swr_alloc_set_opts(NULL,
		av_get_default_channel_layout(CHANNELS),                              //out_channel_layout
		SAMPLE_FORMAT_AV,                                                     //out_sample_fmt
		SAMPLE_RATE,                                                          //out_sample_rate
		av_get_default_channel_layout(avAudioStream->codecContext->channels), //in_channel_layout
		avAudioStream->codecContext->sample_fmt,                              //in_sample_fmt
		avAudioStream->codecContext->sample_rate,                             //in_sample_rate
		0, NULL);
	int swrRetVal = swr_init(resampleContext);
	if (swrRetVal < 0) { return; }

	initialized = 1;
}

void initAudioLib(void)
{
	#ifdef CP_USE_PORTAUDIO
	startThread(&initPortAudio, NULL);
	#else
	startThread(&initLibao, NULL);
	#endif
}

void addAudio(AVFrame* frame)
{
	if (!initialized) { return; }
	Frame* queueFrame = dequeueFrame(STAGE_FREE);
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

	#ifdef CP_USE_PORTAUDIO

	float* fullSamples = (float*)queueFrame->audioFrame;
	for (int i = 0; i < outSamples; i++)
	{
		fullSamples[i * 2] = (float)((double)fullSamples[i * 2] * volume);
		fullSamples[i * 2 + 1] = (float)((double)fullSamples[i * 2 + 1] * volume);
	}

	#else

	short* fullSamples = ((short*)queueFrame->audioFrame);
	for (int i = 0; i < outSamples; i++)
	{
		fullSamples[i * 2] = (short)((double)fullSamples[i * 2] * volume);
		fullSamples[i * 2 + 1] = (short)((double)fullSamples[i * 2 + 1] * volume);
	}

	#endif

	queueFrame->isAudio = 1;
	queueFrame->audioSamplesNum = outSamples;
	enqueueFrame(STAGE_LOADED_FRAME);
}

void audioLoop(void)
{
	const int TIME_TO_SLEEP = 8;

	audioQueue.front = 0;
	audioQueue.back = 0;
	audioQueue.audioFrames = (uint8_t**)malloc(AUDIO_QUEUE_SIZE * sizeof(uint8_t*));
	audioQueue.audioSamplesNum = (int*)malloc(AUDIO_QUEUE_SIZE * sizeof(int));
	audioQueue.audioFramesSize = (int*)malloc(AUDIO_QUEUE_SIZE * sizeof(int));

	for (int i = 0; i < AUDIO_QUEUE_SIZE; i++)
	{
		audioQueue.audioFrames[i] = NULL;
		audioQueue.audioFramesSize[i] = 0;
	}

	while (1)
	{
		if (audioQueue.front != audioQueue.back)
		{
			#ifdef CP_USE_PORTAUDIO

			Pa_WriteStream(stream, audioQueue.audioFrames[audioQueue.front],
				audioQueue.audioSamplesNum[audioQueue.front]);

			#else

			ao_play(aoDevice, audioQueue.audioFrames[audioQueue.front],
				audioQueue.audioSamplesNum[audioQueue.front] * SAMPLE_SIZE);

			#endif

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

static ThreadRetType CP_CALL_CONV initPortAudio(void* ptr)
{
	#ifdef CP_USE_PORTAUDIO

	PaError err;

	err = Pa_Initialize();
	if (err != paNoError)
	{
		libInitialized = -1;
		return;
	}

	audioParameters.device = Pa_GetDefaultOutputDevice();
	if (audioParameters.device == paNoDevice)
	{
		libInitialized = -1;
		return;
	}

	audioParameters.channelCount = CHANNELS;
	audioParameters.sampleFormat = SAMPLE_FORMAT_PA;
	audioParameters.suggestedLatency = Pa_GetDeviceInfo(audioParameters.device)->defaultHighOutputLatency;
	audioParameters.hostApiSpecificStreamInfo = NULL;

	Pa_OpenStream(&stream, NULL, &audioParameters, SAMPLE_RATE, 0, paNoFlag, NULL, NULL);
	err = Pa_StartStream(stream);

	if (err == paNoError) { libInitialized = 1; }
	else { libInitialized = -1; }

	#endif

	CP_END_THREAD
}

static ThreadRetType CP_CALL_CONV initLibao(void* ptr)
{
	#ifndef CP_USE_PORTAUDIO

	ao_initialize();
	int driver = ao_default_driver_id();

	memset(&aoSampleFormat, 0, sizeof(ao_sample_format));
	aoSampleFormat.bits = SAMPLE_BITS;
	aoSampleFormat.channels = CHANNELS;
	aoSampleFormat.rate = SAMPLE_RATE;
	aoSampleFormat.byte_format = AO_FMT_LITTLE;
	aoSampleFormat.matrix = 0;
	aoDevice = ao_open_live(driver, &aoSampleFormat, NULL);

	if (aoDevice) { libInitialized = 1; }
	else { libInitialized = -1; }

	#endif

	CP_END_THREAD
}