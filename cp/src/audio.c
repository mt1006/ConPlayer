#include "conplayer.h"

// If you're changing SAMPLE_BITS or CHANNELS value, remember
// to modify "for loop" in addAudio function, responsible for
// changing volume, which is hardcoded to 32 bits / 2 channels
static const enum AVSampleFormat SAMPLE_FORMAT_AV = AV_SAMPLE_FMT_FLT;
static const PaSampleFormat SAMPLE_FORMAT_PA = paFloat32;
static const int SAMPLE_BITS = 32;
static const int CHANNELS = 2;
static const int SAMPLE_RATE = 48000;

static int initialized = 0;
static int libInitialized = 0;
static PaStream* stream;
static PaStreamParameters audioParameters;
static SwrContext* resampleContext = NULL;

static ThreadRetType CP_CALL_CONV initAudioLibThread(void* ptr);

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
	startThread(&initAudioLibThread, NULL);
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
	float* fullSamples = (float*)queueFrame->audioFrame;
	for (int i = 0; i < outSamples; i++)
	{
		fullSamples[i * 2] = (float)((double)fullSamples[i * 2] * volume);
		fullSamples[i * 2 + 1] = (float)((double)fullSamples[i * 2 + 1] * volume);
	}
	queueFrame->isAudio = 1;
	queueFrame->audioSamplesNum = outSamples;
	enqueueFrame(STAGE_LOADED_FRAME);
}

void playAudio(Frame* frame)
{
	Pa_WriteStream(stream, frame->audioFrame, frame->audioSamplesNum);
}

void deinitAudio(void)
{
	if (initialized)
	{
		swr_free(&resampleContext);
		Pa_StopStream(stream);
		Pa_CloseStream(stream);
		Pa_Terminate();
	}
}

static ThreadRetType CP_CALL_CONV initAudioLibThread(void* ptr)
{
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
	audioParameters.suggestedLatency = Pa_GetDeviceInfo(audioParameters.device)->defaultLowOutputLatency;
	audioParameters.hostApiSpecificStreamInfo = NULL;

	Pa_OpenStream(&stream, NULL, &audioParameters, SAMPLE_RATE, 0, paNoFlag, NULL, NULL);
	err = Pa_StartStream(stream);

	if (err == paNoError) { libInitialized = 1; }
	else { libInitialized = -1; }

	CP_END_THREAD
}