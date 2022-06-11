#include "conplayer.h"

// If you're changing SAMPLE_BITS or CHANNELS value, remember
// to change SAMPLE_SIZE value and modify "for loop"
// in playAudio function responsible for changing volume,
// which is hardcoded to 16 bits / 2 channels
static const enum AVSampleFormat SAMPLE_FORMAT = AV_SAMPLE_FMT_S16;
static const int SAMPLE_BITS = 16;
static const int CHANNELS = 2;
static const int SAMPLE_SIZE = 4; // (16 bits * 2 channels) / 8 bits
static const int SAMPLE_RATE = 48000;

static int initialized = 0;
static ao_sample_format aoSampleFormat;
static ao_device* aoDevice = NULL;
static SwrContext* resampleContext = NULL;

void initAudio(Stream* audioStream)
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

	resampleContext = swr_alloc_set_opts(NULL,
		av_get_default_channel_layout(aoSampleFormat.channels),             //out_channel_layout
		SAMPLE_FORMAT,                                                      //out_sample_fmt
		aoSampleFormat.rate,                                                //out_sample_rate
		av_get_default_channel_layout(audioStream->codecContext->channels), //in_channel_layout
		audioStream->codecContext->sample_fmt,                              //in_sample_fmt
		audioStream->codecContext->sample_rate,                             //in_sample_rate
		0, NULL);
	int swrRetVal = swr_init(resampleContext);
	if (aoDevice && swrRetVal >= 0) { initialized = 1; }
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
		av_samples_alloc(&queueFrame->audioFrame, NULL, aoSampleFormat.channels,
			queueFrame->audioFrameSize, SAMPLE_FORMAT, 0);
	}
	outSamples = swr_convert(resampleContext, &queueFrame->audioFrame, outSamples,
		frame->extended_data, frame->nb_samples);
	if (outSamples < 0) { return; }
	short* fullSamples = ((short*)queueFrame->audioFrame);
	for (int i = 0; i < outSamples; i++)
	{
		fullSamples[i * 2] = (short)((double)fullSamples[i * 2] * volume);
		fullSamples[i * 2 + 1] = (short)((double)fullSamples[i * 2 + 1] * volume);
	}
	queueFrame->isAudio = 1;
	queueFrame->audioSamplesNum = outSamples;
	enqueueFrame(STAGE_LOADED_FRAME);
}

void playAudio(Frame* frame)
{
	ao_play(aoDevice, frame->audioFrame, frame->audioSamplesNum * SAMPLE_SIZE);
}

void deinitAudio()
{
	if (aoDevice) { ao_close(aoDevice); }
	swr_free(&resampleContext);
	ao_shutdown();
}