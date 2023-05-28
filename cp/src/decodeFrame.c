#include "conplayer.h"

static const double CONSOLE_REFRESH_PERIOD = 0.2;

static Stream videoStream = { -1 };
static Stream audioStream = { -1 };
static enum AVPixelFormat destFormat;
static AVFormatContext* formatContext;
static struct SwsContext* swsContext = NULL;
static uint8_t* scaledFrameBuffer = NULL;
static int lastFrame = -1;
static bool useAVSeek = false;
static int64_t seekTimestamp = 0;

static void decodeVideoPacket(AVPacket* packet, AVFrame* videoFrame, AVFrame* filterFrame, AVFrame* scaledFrame);
static void decodeAudioPacket(AVPacket* packet, AVFrame* audioFrame, AVFrame* filterFrame);
static void addVideoFrame(AVFrame* frame);
static void refreshFrameSize(AVFrame* inputFrame, AVFrame* outputFrame);

void initDecodeFrame(const char* file, Stream** outAudioStream)
{
	if (settings.colorMode == CM_CSTD_16 ||
		settings.colorMode == CM_CSTD_256 ||
		settings.colorMode == CM_CSTD_RGB ||
		settings.colorMode == CM_WINAPI_16)
	{
		destFormat = AV_PIX_FMT_RGB24;
	}
	else
	{
		destFormat = AV_PIX_FMT_GRAY8;
	}

	av_register_all();
	avcodec_register_all();

	formatContext = avformat_alloc_context();
	if (!formatContext) { error("Failed to allocate format context", "decodeFrame.c", __LINE__); }
	if (avformat_open_input(&formatContext, file, NULL, NULL)) { error("Failed to open file!", "decodeFrame.c", __LINE__); }
	if (avformat_find_stream_info(formatContext, NULL) < 0) { error("Failed to find stream", "decodeFrame.c", __LINE__); }

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
		if (!videoStream.codecContext) { error("Failed to allocate video codec context", "decodeFrame.c", __LINE__); }
		if (avcodec_parameters_to_context(videoStream.codecContext, videoStream.codecParameters) < 0)
		{
			error("Failed to copy video codec parameters to context", "decodeFrame.c", __LINE__);
		}
		if (avcodec_open2(videoStream.codecContext, videoStream.codec, NULL) < 0)
		{
			error("Failed to open video codec", "decodeFrame.c", __LINE__);
		}
		vidW = videoStream.stream->codec->width;
		vidH = videoStream.stream->codec->height;
		fps = (double)videoStream.stream->r_frame_rate.num / (double)videoStream.stream->r_frame_rate.den;

		if (settings.videoFilters) { initFiltersV(&videoStream); }
	}

	if (audioStream.index != -1 && !settings.disableAudio)
	{
		audioStream.codecContext = avcodec_alloc_context3(audioStream.codec);
		if (!audioStream.codecContext) { error("Failed to allocate audio codec context", "decodeFrame.c", __LINE__); }
		if (avcodec_parameters_to_context(audioStream.codecContext, audioStream.codecParameters) < 0)
		{
			error("Failed to copy audio codec parameters to context", "decodeFrame.c", __LINE__);
		}
		if (avcodec_open2(audioStream.codecContext, audioStream.codec, NULL) < 0)
		{
			error("Failed to open audio codec", "decodeFrame.c", __LINE__);
		}

		if (settings.audioFilters) { initFiltersA(&audioStream); }
	}

	*outAudioStream = &audioStream;
}

void readFrames(void)
{
	double lastConRefresh = 0.0;

	AVPacket* packet = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();
	AVFrame* filterFrame = av_frame_alloc();
	AVFrame* scaledFrame = av_frame_alloc();

	while (av_read_frame(formatContext, packet) >= 0)
	{
		while (freezeThreads)
		{
			mainFreezed = true;
			Sleep(SLEEP_ON_FREEZE);
		}

		if (useAVSeek)
		{
			av_seek_frame(formatContext, -1, seekTimestamp, AVSEEK_FLAG_BACKWARD);
			useAVSeek = false;
		}

		double curTime = getTime();
		if (curTime > lastConRefresh + CONSOLE_REFRESH_PERIOD)
		{
			refreshSize();
			lastConRefresh = curTime;
		}

		if (packet->stream_index == videoStream.index) { decodeVideoPacket(packet, frame, filterFrame, scaledFrame); }
		else if (packet->stream_index == audioStream.index) { decodeAudioPacket(packet, frame, filterFrame); }

		av_packet_unref(packet);
	}

	av_frame_free(&scaledFrame);
	av_frame_free(&filterFrame);
	av_frame_free(&frame);
	av_packet_free(&packet);

	decodeEnd = true;
	while (1) { Sleep(30); }
}

void avSeek(int64_t timestamp)
{
	seekTimestamp = timestamp;
	useAVSeek = true;
}

static void decodeVideoPacket(AVPacket* packet, AVFrame* videoFrame, AVFrame* filterFrame, AVFrame* scaledFrame)
{
	if (avcodec_send_packet(videoStream.codecContext, packet) < 0) { return; }

	while (avcodec_receive_frame(videoStream.codecContext, videoFrame) >= 0)
	{
		if (settings.videoFilters && settings.scaledVideoFilters)
		{
			applyFiltersV(videoFrame);

			while (getFilteredFrameV(filterFrame))
			{
				refreshFrameSize(filterFrame, scaledFrame);
				sws_scale(swsContext, filterFrame->data, filterFrame->linesize, 0,
					filterFrame->height, scaledFrame->data, scaledFrame->linesize);
				av_frame_copy_props(scaledFrame, filterFrame);
				av_frame_unref(filterFrame);

				applyFiltersSV(scaledFrame);

				while (getFilteredFrameSV(filterFrame))
				{
					filterFrame->pts = videoFrame->pts;
					addVideoFrame(filterFrame);
					av_frame_unref(filterFrame);
				}
			}
		}
		else if (settings.videoFilters)
		{
			applyFiltersV(videoFrame);

			while (getFilteredFrameV(filterFrame))
			{
				refreshFrameSize(filterFrame, scaledFrame);
				sws_scale(swsContext, filterFrame->data, filterFrame->linesize, 0,
					filterFrame->height, scaledFrame->data, scaledFrame->linesize);
				scaledFrame->pts = videoFrame->pts;
				addVideoFrame(scaledFrame);

				av_frame_unref(filterFrame);
			}
		}
		else if (settings.scaledVideoFilters)
		{
			refreshFrameSize(videoFrame, scaledFrame);
			sws_scale(swsContext, videoFrame->data, videoFrame->linesize, 0, videoFrame->height,
				scaledFrame->data, scaledFrame->linesize);
			av_frame_copy_props(scaledFrame, videoFrame);

			applyFiltersSV(scaledFrame);

			while (getFilteredFrameSV(filterFrame))
			{
				filterFrame->pts = videoFrame->pts;
				addVideoFrame(filterFrame);
				av_frame_unref(filterFrame);
			}
		}
		else
		{
			refreshFrameSize(videoFrame, scaledFrame);
			sws_scale(swsContext, videoFrame->data, videoFrame->linesize, 0,
				videoFrame->height, scaledFrame->data, scaledFrame->linesize);
			scaledFrame->pts = videoFrame->pts;
			addVideoFrame(scaledFrame);
		}
	}
}

static void decodeAudioPacket(AVPacket* packet, AVFrame* audioFrame, AVFrame* filterFrame)
{
	if (settings.disableAudio) { return; }

	if (avcodec_send_packet(audioStream.codecContext, packet) < 0) { return; }

	while (avcodec_receive_frame(audioStream.codecContext, audioFrame) >= 0)
	{
		if (settings.audioFilters)
		{
			applyFiltersA(audioFrame);

			while (getFilteredFrameA(filterFrame))
			{
				addAudioFrame(filterFrame);
				av_frame_unref(filterFrame);
			}
		}
		else
		{
			addAudioFrame(audioFrame);
		}
	}
}

static void addVideoFrame(AVFrame* frame)
{
	Frame* queueFrame = dequeueFrame(STAGE_FREE, &mainFreezed);

	if (queueFrame->frameW != w ||
		queueFrame->frameH != h ||
		queueFrame->videoLinesize != frame->linesize[0])
	{
		if (queueFrame->videoFrame) { free(queueFrame->videoFrame); }
		if (queueFrame->output) { free(queueFrame->output); }
		if (queueFrame->outputLineOffsets) { free(queueFrame->outputLineOffsets); }
		queueFrame->frameW = w;
		queueFrame->frameH = h;
		queueFrame->videoLinesize = frame->linesize[0];
		queueFrame->videoFrame = malloc(frame->linesize[0] * h);
		queueFrame->output = malloc(getOutputArraySize(w, h));
		queueFrame->outputLineOffsets = malloc((h + 1) * sizeof(int));
	}

	memcpy(queueFrame->videoFrame, frame->data[0], frame->linesize[0] * h);

	queueFrame->time = (int64_t)(((double)frame->pts /
		(double)videoStream.stream->time_base.den) * (double)AV_TIME_BASE);
	queueFrame->isAudio = false;

	enqueueFrame(STAGE_LOADED_FRAME);
}

static void refreshFrameSize(AVFrame* inputFrame, AVFrame* outputFrame)
{
	static int lastFrameW = -1, lastFrameH = -1;
	static int lastConsoleW = -1, lastConsoleH = -1;
	static enum AVPixelFormat lastPixelFormat = AV_PIX_FMT_NONE;

	if (lastFrameW != inputFrame->width ||
		lastFrameH != inputFrame->height ||
		lastPixelFormat != inputFrame->format ||
		lastConsoleW != w ||
		lastConsoleH != h)
	{
		lastFrameW = inputFrame->width;
		lastFrameH = inputFrame->height;
		lastConsoleW = w;
		lastConsoleH = h;
		lastPixelFormat = inputFrame->format;

		if (swsContext) { sws_freeContext(swsContext); }
		if (scaledFrameBuffer) { av_free(scaledFrameBuffer); }

		swsContext = sws_getContext(inputFrame->width, inputFrame->height,
			inputFrame->format, w, h, destFormat, settings.scalingMode, NULL, NULL, NULL);
		scaledFrameBuffer = (uint8_t*)av_malloc(avpicture_get_size(destFormat, w, h) * sizeof(uint8_t));
		
		av_image_fill_arrays(outputFrame->data, outputFrame->linesize,
			scaledFrameBuffer, destFormat, w, h, 1);
		av_frame_copy_props(outputFrame, inputFrame);

		outputFrame->width = w;
		outputFrame->height = h;
		outputFrame->format = destFormat;
		outputFrame->sample_aspect_ratio = inputFrame->sample_aspect_ratio;

		if (settings.scaledVideoFilters) { initFiltersSV(&videoStream, outputFrame); }
	}
}