#include "conplayer.h"

// from threads.c
extern const int SLEEP_ON_FREEZE;
extern int freezeThreads;
extern int mainFreezed;

static const double CONSOLE_REFRESH_PERIOD = 0.2;

static enum AVPixelFormat destFormat;
static AVFormatContext* formatContext;
static struct SwsContext* swsContext = NULL;
static uint8_t* scaledFrameBuffer = NULL;
static Stream videoStream = { -1 };
static Stream audioStream = { -1 };
static int lastFrame = -1;
static int useAVSeek = 0;
static int64_t seekTimestamp = 0;

void setFrameSize(int inputW, int inputH, enum AVPixelFormat inputFormat,
	int outputW, int outputH, enum AVPixelFormat outputFormat, AVFrame* outFrame);
int decodeVideoPacket(AVPacket* packet, AVFrame* frame, AVFrame* scaledFrame);
int decodeAudioPacket(AVPacket* packet, AVFrame* frame);

void initAV(const char* file, Stream** outAudioStream)
{
	if (colorMode==CM_CSTD_16 ||
		colorMode == CM_CSTD_256||
		colorMode == CM_CSTD_RGB||
		colorMode == CM_WINAPI_16)
	{
		destFormat = AV_PIX_FMT_RGB24;
	}
	else
	{
		destFormat = AV_PIX_FMT_GRAY8;
	}

	formatContext = avformat_alloc_context();
	if (!formatContext) { error("Failed to allocate format context", "avDecode.c", __LINE__); }
	if (avformat_open_input(&formatContext, file, NULL, NULL)) { error("Failed to open file!", "avDecode.c", __LINE__); }
	if (avformat_find_stream_info(formatContext, NULL) < 0) { error("Failed to find stream", "avDecode.c", __LINE__); }

	for (int i = 0; i < (int)formatContext->nb_streams; i++)
	{
		AVCodecParameters* codecParameters = NULL;
		codecParameters = formatContext->streams[i]->codecpar;
		AVCodec* codec = NULL;
		codec = avcodec_find_decoder(codecParameters->codec_id);
		if (!codec) { continue; }
		if (codecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			if (videoStream.index == -1)
			{
				videoStream.index = i;
				videoStream.stream = formatContext->streams[i];
				videoStream.codec = codec;
				videoStream.codecParameters = codecParameters;
			}
		}
		else if (codecParameters->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			if (audioStream.index == -1)
			{
				audioStream.index = i;
				audioStream.stream = formatContext->streams[i];
				audioStream.codec = codec;
				audioStream.codecParameters = codecParameters;
			}
		}
	}

	if (videoStream.index != -1)
	{
		videoStream.codecContext = avcodec_alloc_context3(videoStream.codec);
		if (!videoStream.codecContext) { error("Failed to allocate video codec context", "avDecode.c", __LINE__); }
		if (avcodec_parameters_to_context(videoStream.codecContext, videoStream.codecParameters) < 0)
		{
			error("Failed to copy video codec parameters to context", "avDecode.c", __LINE__);
		}
		if (avcodec_open2(videoStream.codecContext, videoStream.codec, NULL) < 0)
		{
			error("Failed to open video codec", "avDecode.c", __LINE__);
		}
		vidW = videoStream.stream->codec->width;
		vidH = videoStream.stream->codec->height;
		fps = (double)videoStream.stream->r_frame_rate.num / (double)videoStream.stream->r_frame_rate.den;
	}

	if (audioStream.index != -1)
	{
		audioStream.codecContext = avcodec_alloc_context3(audioStream.codec);
		if (!audioStream.codecContext) { error("Failed to allocate audio codec context", "avDecode.c", __LINE__); }
		if (avcodec_parameters_to_context(audioStream.codecContext, audioStream.codecParameters) < 0)
		{
			error("Failed to copy audio codec parameters to context", "avDecode.c", __LINE__);
		}
		if (avcodec_open2(audioStream.codecContext, audioStream.codec, NULL) < 0)
		{
			error("Failed to open audio codec", "avDecode.c", __LINE__);
		}
	}

	*outAudioStream = &audioStream;
}

void readFrames(void)
{
	double lastConRefresh = 0.0;

	AVPacket* packet = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();
	AVFrame* scaledFrame = av_frame_alloc();
	while (av_read_frame(formatContext, packet) >= 0)
	{
		while (freezeThreads)
		{
			mainFreezed = 1;
			Sleep(SLEEP_ON_FREEZE);
		}

		if (useAVSeek)
		{
			av_seek_frame(formatContext, -1, seekTimestamp, AVSEEK_FLAG_BACKWARD);
			useAVSeek = 0;
		}

		double curTime = getTime();
		if (curTime > lastConRefresh + CONSOLE_REFRESH_PERIOD)
		{
			refreshSize();
			lastConRefresh = curTime;
		}

		if (packet->stream_index == videoStream.index)
		{
			int response = decodeVideoPacket(packet, frame, scaledFrame);
		}
		else if (packet->stream_index == audioStream.index)
		{
			int response = decodeAudioPacket(packet, frame);
		}
		av_packet_unref(packet);
	}
	av_frame_free(&scaledFrame);
	av_frame_free(&frame);
	av_packet_free(&packet);

	decodeEnd = 1;
	while (1) { Sleep(30); }
}

void avSeek(int64_t timestamp)
{
	seekTimestamp = timestamp;
	useAVSeek = 1;
}

static void setFrameSize(int inputW, int inputH, enum AVPixelFormat inputFormat,
	int outputW, int outputH, enum AVPixelFormat outputFormat, AVFrame* outFrame)
{
	if (swsContext) { sws_freeContext(swsContext); }
	if (scaledFrameBuffer) { av_free(scaledFrameBuffer); }
	swsContext = sws_getContext(inputW, inputH, inputFormat,
		outputW, outputH, outputFormat, SWS_BICUBIC, NULL, NULL, NULL);
	int arraySize = avpicture_get_size(outputFormat, outputW, outputH);
	scaledFrameBuffer = (uint8_t*)av_malloc(arraySize * sizeof(uint8_t));
	avpicture_fill((AVPicture*)outFrame, scaledFrameBuffer, outputFormat, outputW, outputH);
}

static int decodeVideoPacket(AVPacket* packet, AVFrame* frame, AVFrame* scaledFrame)
{
	static int lastFrameW = -1, lastFrameH = -1;
	static int lastConsoleW = -1, lastConsoleH = -1;
	static enum AVPixelFormat lastPixelFormat = AV_PIX_FMT_NONE;

	int response = avcodec_send_packet(videoStream.codecContext, packet);
	if (response < 0) { return response; }
	while (response >= 0)
	{
		response = avcodec_receive_frame(videoStream.codecContext, frame);
		if (response < 0) { return 0; }
		Frame* queueFrame = dequeueFrame(STAGE_FREE);
		queueFrame->isAudio = 0;
		if (lastFrameW != frame->width ||
			lastFrameH != frame->height ||
			lastPixelFormat != frame->format ||
			lastConsoleW != w ||
			lastConsoleH != h)
		{
			lastFrameW = frame->width;
			lastFrameH = frame->height;
			lastConsoleW = w;
			lastConsoleH = h;
			lastPixelFormat = frame->format;
			setFrameSize(frame->width, frame->height, frame->format,
				w, h, destFormat, scaledFrame);
		}
		sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height,
			scaledFrame->data, scaledFrame->linesize);
		
		if (queueFrame->frameW != w ||
			queueFrame->frameH != h ||
			queueFrame->videoLinesize != scaledFrame->linesize[0])
		{
			if (queueFrame->videoFrame) { free(queueFrame->videoFrame); }
			if (queueFrame->output) { free(queueFrame->output); }
			if (queueFrame->outputLineOffsets) { free(queueFrame->outputLineOffsets); }
			queueFrame->frameW = w;
			queueFrame->frameH = h;
			queueFrame->videoLinesize = scaledFrame->linesize[0];
			queueFrame->videoFrame = malloc(scaledFrame->linesize[0] * h);
			queueFrame->output = malloc(getOutputArraySize());
			queueFrame->outputLineOffsets = malloc((h + 1) * sizeof(int));
		}
		memcpy(queueFrame->videoFrame, scaledFrame->data[0],
			scaledFrame->linesize[0] * h);
		queueFrame->isAudio = 0;
		queueFrame->time = (int64_t)(((double)frame->pts /
			(double)videoStream.stream->time_base.den) * (double)AV_TIME_BASE);
		enqueueFrame(STAGE_LOADED_FRAME);
	}
	return 0;
}

static int decodeAudioPacket(AVPacket* packet, AVFrame* frame)
{
	int success = 0;
	int len = avcodec_decode_audio4(audioStream.codecContext, frame, &success, packet);
	if (success) { addAudio(frame); }
	return 0;
}

void unload(void)
{
	sws_freeContext(swsContext);
	av_free(scaledFrameBuffer);
	avcodec_free_context(&videoStream.codecContext);
	avcodec_free_context(&audioStream.codecContext);
	avformat_close_input(&formatContext);
}