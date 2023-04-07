#include "conplayer.h"

static const double CONSOLE_REFRESH_PERIOD = 0.2;

static Stream videoStream = { -1 };
static Stream audioStream = { -1 };

static enum AVPixelFormat destFormat;

static AVFormatContext* formatContext;
static AVFormatContext* secondFormatContext = NULL;
static AVFormatContext* audioContext;
static AVFormatContext* videoContext;

static AVFrame* decodedFrame;
static AVFrame* rgbFrame;       // for video with first stage shaders enabled
static AVFrame* filterFrame;    // for FFmpeg filters
static AVFrame* scaledFrame;    // for video

static struct SwsContext* rgbContext = NULL;
static struct SwsContext* scalingContext = NULL;

static int lastFrame = -1;
static volatile bool useAVSeek = false;
static volatile int64_t seekTimestamp = 0;


static AVFormatContext* loadContextAndStreams(const char* file);
static void decodeVideoPacket(AVPacket* packet);
static void decodeAudioPacket(AVPacket* packet);
static void addVideoFrame(AVFrame* frame);
static void refeshRgbFrame(AVFrame* inputFrame);
static void refreshScaledFrame(AVFrame* swsInputFrame);
static scaleFrame(struct SwsContext* context, AVFrame* inputFrame, AVFrame* outputFrame);


uint8_t* buffer;
size_t bufferSize, bufferSizeOld;
int64_t pos;

//=========
// Temp code:
static int read_packet(void* opaque, uint8_t* buf, int buf_size)
{
	buf_size = buf_size < bufferSize ? buf_size : bufferSize;

	if (!buf_size) { return AVERROR_EOF; }

	memcpy(buf, buffer, buf_size);

	buffer += buf_size;
	bufferSize -= buf_size;
	pos += buf_size;

	return buf_size;
}

static int64_t io_seek(void* opaque, int64_t offset, int whence)
{
	// fix it - this is a tragedy
	if (whence == AVSEEK_SIZE) { return bufferSizeOld; }
	else if (whence == SEEK_CUR)
	{
		buffer += offset;
		bufferSize -= offset;
		pos += offset;
	}
	else if (whence == SEEK_END)
	{
		buffer += offset + bufferSizeOld - pos;
		bufferSize -= offset + bufferSizeOld - pos;
		pos = offset + bufferSizeOld;
	}
	else
	{
		buffer += offset - pos;
		bufferSize -= offset - pos;
		pos = offset;
	}

	printf("%lld %d\n", offset, whence);
	return 0;
}
//=========

void initDecodeFrame(const char* file, const char* secondFile, Stream** outAudioStream)
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

	if (settings.preload)
	{
		const int TEMP_BUFFER_SIZE = 0x100000;
		FILE* preloadFile = fopen(file, "rb");
		if (!preloadFile) { error("Failed to preload file", "decodeFrame.c", __LINE__); }

		char* tempBuffer = (char*)malloc(TEMP_BUFFER_SIZE);
		while (fread(tempBuffer, 1, TEMP_BUFFER_SIZE, preloadFile) == TEMP_BUFFER_SIZE);
		free(tempBuffer);

		fclose(preloadFile);
	}

	formatContext = loadContextAndStreams(file);
	if (secondFile) { secondFormatContext = loadContextAndStreams(secondFile); }

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

	if (audioStream.index != -1) { *outAudioStream = &audioStream; }
	else { *outAudioStream = NULL; }

	decodedFrame = av_frame_alloc();
	rgbFrame = av_frame_alloc();
	filterFrame = av_frame_alloc();
	scaledFrame = av_frame_alloc();

	//shStage1_init();
}

void readFrames(void)
{
	double lastConRefresh = 0.0;

	AVPacket* packet = av_packet_alloc();
	double lastDTS = DBL_MIN;
	int err1 = 0, err2 = 0;

	while (true)
	{
		if (settings.useFakeConsole) { peekMainMessages(); }

		while (freezeThreads)
		{
			mainFreezed = true;
			if (settings.useFakeConsole) { peekMainMessages(); }
			Sleep(SLEEP_ON_FREEZE);
		}

		if (useAVSeek)
		{
			av_seek_frame(formatContext, -1, seekTimestamp, AVSEEK_FLAG_BACKWARD);
			if (secondFormatContext) { av_seek_frame(secondFormatContext, -1, seekTimestamp, AVSEEK_FLAG_BACKWARD); }
			useAVSeek = false;
		}

		double curTime = getTime();
		if (curTime > lastConRefresh + CONSOLE_REFRESH_PERIOD)
		{
			if (settings.useFakeConsole) { refreshFont(); }
			refreshSize();
			lastConRefresh = curTime;
		}

		bool decoded = false;
		double dts = DBL_MIN;

		while (true)
		{
			if ((err1 = av_read_frame(formatContext, packet)) >= 0)
			{
				if (packet->stream_index == videoStream.index && formatContext == videoContext)
				{
					decodeVideoPacket(packet);
					dts = (double)packet->dts * (double)videoStream.stream->time_base.num /
						(double)videoStream.stream->time_base.den;
				}
				else if (packet->stream_index == audioStream.index && formatContext == audioContext)
				{
					decodeAudioPacket(packet);
					dts = (double)packet->dts * (double)audioStream.stream->time_base.num /
						(double)audioStream.stream->time_base.den;
				}

				av_packet_unref(packet);
				decoded = true;
			}
			else
			{
				break;
			}

			if (dts > lastDTS || !secondFormatContext) { break; }
		}

		lastDTS = dts;

		while (secondFormatContext)
		{
			if (secondFormatContext && (err2 = av_read_frame(secondFormatContext, packet)) >= 0)
			{
				if (packet->stream_index == videoStream.index && secondFormatContext == videoContext)
				{
					decodeVideoPacket(packet);
					dts = (double)packet->dts * (double)videoStream.stream->time_base.num /
						(double)videoStream.stream->time_base.den;
				}
				else if (packet->stream_index == audioStream.index && secondFormatContext == audioContext)
				{
					decodeAudioPacket(packet);
					dts = (double)packet->dts * (double)audioStream.stream->time_base.num /
						(double)audioStream.stream->time_base.den;
				}

				av_packet_unref(packet);
				decoded = true;
			}
			else
			{
				break;
			}

			if (dts > lastDTS) { break; }
		}

		lastDTS = dts;
		if (!decoded) { break; }
	}

	av_packet_free(&packet);

	printf("%d (%d) / %d (%d)\n", (-err1) & 0xFF, err1, (-err2) & 0xFF, err2);
	decodeEnd = true;
	while (true)
	{
		if (settings.useFakeConsole) { peekMainMessages(); }
		Sleep(30);
	}
}

void avSeek(int64_t timestamp)
{
	seekTimestamp = timestamp;
	useAVSeek = true;
}

static AVFormatContext* loadContextAndStreams(const char* file)
{
	AVFormatContext* ctx = avformat_alloc_context();
	if (!ctx) { error("Failed to allocate format context", "decodeFrame.c", __LINE__); }

	//=========
	/*av_file_map(file, &buffer, &bufferSize, 0, NULL);
	bufferSizeOld = bufferSize;

	size_t ctxBufferSize = 4096;
	uint8_t* ctxBuffer = av_malloc(ctxBufferSize);

	AVIOContext* avio_ctx = avio_alloc_context(ctxBuffer, ctxBufferSize, 0, NULL, &read_packet, NULL, &io_seek);
	formatContext->pb = avio_ctx;

	if (avformat_open_input(&formatContext, NULL, NULL, NULL)) { error("Failed to open file!", "decodeFrame.c", __LINE__); }*/
	//=========

	if (avformat_open_input(&ctx, file, NULL, NULL))
	{
		printf("Failed to open file: \"%s\"\n", file);
		error("Failed to open file!", "decodeFrame.c", __LINE__);
	}
	if (avformat_find_stream_info(ctx, NULL) < 0)
	{
		error("Failed to find stream", "decodeFrame.c", __LINE__);
	}

	for (int i = 0; i < (int)ctx->nb_streams; i++)
	{
		AVCodecParameters* codecParameters = ctx->streams[i]->codecpar;
		AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);
		if (!codec) { continue; }

		if (codecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			if (videoStream.index == -1)
			{
				videoStream.index = i;
				videoStream.stream = ctx->streams[i];
				videoStream.codec = codec;
				videoStream.codecParameters = codecParameters;
				videoContext = ctx;
			}
		}
		else if (codecParameters->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			if (audioStream.index == -1)
			{
				audioStream.index = i;
				audioStream.stream = ctx->streams[i];
				audioStream.codec = codec;
				audioStream.codecParameters = codecParameters;
				audioContext = ctx;
			}
		}
	}
	return ctx;
}

static void decodeVideoPacket(AVPacket* packet)
{
	if (avcodec_send_packet(videoStream.codecContext, packet) < 0) { return; }

	while (avcodec_receive_frame(videoStream.codecContext, decodedFrame) >= 0)
	{
		AVFrame* inputFrame = decodedFrame;

		/*if (shStage1_enabled)
		{
			refeshRgbFrame(decodedFrame);
			scaleFrame(rgbContext, decodedFrame, rgbFrame);
			shStage1_apply(rgbFrame);
			inputFrame = rgbFrame;
		}*/

		if (settings.videoFilters && settings.scaledVideoFilters)
		{
			applyFiltersV(inputFrame);

			while (getFilteredFrameV(filterFrame))
			{
				refreshScaledFrame(filterFrame);
				scaleFrame(scalingContext, filterFrame, scaledFrame);
				av_frame_unref(filterFrame);

				applyFiltersSV(scaledFrame);

				while (getFilteredFrameSV(filterFrame))
				{
					filterFrame->pts = inputFrame->pts;
					addVideoFrame(filterFrame);
					av_frame_unref(filterFrame);
				}
			}
		}
		else if (settings.videoFilters)
		{
			applyFiltersV(inputFrame);

			while (getFilteredFrameV(filterFrame))
			{
				refreshScaledFrame(filterFrame);
				scaleFrame(scalingContext, filterFrame, scaledFrame);
				scaledFrame->pts = inputFrame->pts;
				addVideoFrame(scaledFrame);

				av_frame_unref(filterFrame);
			}
		}
		else if (settings.scaledVideoFilters)
		{
			refreshScaledFrame(inputFrame);
			scaleFrame(scalingContext, inputFrame, scaledFrame);

			applyFiltersSV(scaledFrame);

			while (getFilteredFrameSV(filterFrame))
			{
				filterFrame->pts = inputFrame->pts;
				addVideoFrame(filterFrame);
				av_frame_unref(filterFrame);
			}
		}
		else
		{
			refreshScaledFrame(inputFrame);
			scaleFrame(scalingContext, inputFrame, scaledFrame);
			scaledFrame->pts = inputFrame->pts;
			addVideoFrame(scaledFrame);
		}
	}
}

static void decodeAudioPacket(AVPacket* packet)
{
	if (settings.disableAudio) { return; }
	if (avcodec_send_packet(audioStream.codecContext, packet) < 0) { return; }

	while (avcodec_receive_frame(audioStream.codecContext, decodedFrame) >= 0)
	{
		if (settings.audioFilters)
		{
			applyFiltersA(decodedFrame);

			while (getFilteredFrameA(filterFrame))
			{
				addAudioFrame(filterFrame);
				av_frame_unref(filterFrame);
			}
		}
		else
		{
			addAudioFrame(decodedFrame);
		}
	}
}

static void addVideoFrame(AVFrame* frame)
{
	Frame* queueFrame = dequeueFrame(STAGE_FREE, &mainFreezed);

	if (queueFrame->w != conW ||
		queueFrame->h != conH ||
		queueFrame->videoLinesize != frame->linesize[0])
	{
		if (queueFrame->outputLineOffsets) { free(queueFrame->outputLineOffsets); }
		if (queueFrame->output) { free(queueFrame->output); }
		if (queueFrame->videoFrame) { free(queueFrame->videoFrame); }

		queueFrame->w = conW;
		queueFrame->h = conH;
		queueFrame->videoLinesize = frame->linesize[0];
		queueFrame->videoFrame = (uint8_t*)malloc(frame->linesize[0] * conH);
		queueFrame->output = malloc(getOutputArraySize(conW, conH));
		queueFrame->outputLineOffsets = malloc((conH + 1) * sizeof(int));
	}

	memcpy(queueFrame->videoFrame, frame->data[0], frame->linesize[0] * conH);

	queueFrame->time = (int64_t)(((double)frame->pts /
		(double)videoStream.stream->time_base.den) * (double)AV_TIME_BASE);
	queueFrame->isAudio = false;

	enqueueFrame(STAGE_LOADED_FRAME);
}

static void refeshRgbFrame(AVFrame* inputFrame)
{
	static int lastFrameW = -1, lastFrameH = -1;
	static enum AVPixelFormat lastPixelFormat = AV_PIX_FMT_NONE;
	static uint8_t* rgbFrameBuffer = NULL;

	const enum AVPixelFormat RGB_PIXEL_FORMAT = AV_PIX_FMT_RGB24;

	int w = inputFrame->width;
	int h = inputFrame->height;
	int format = inputFrame->format;

	if (lastFrameW != w ||
		lastFrameH != h ||
		lastPixelFormat != format)
	{
		lastFrameW = w;
		lastFrameH = h;
		lastPixelFormat = format;

		if (rgbContext) { sws_freeContext(rgbContext); }
		if (rgbFrameBuffer) { av_free(rgbFrameBuffer); }

		rgbContext = sws_getContext(w, h, format, w, h, RGB_PIXEL_FORMAT, SWS_POINT, NULL, NULL, NULL);
		rgbFrameBuffer = (uint8_t*)av_malloc(avpicture_get_size(RGB_PIXEL_FORMAT, w, h) * sizeof(uint8_t));

		av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, rgbFrameBuffer, RGB_PIXEL_FORMAT, w, h, 1);
		av_frame_copy_props(rgbFrame, inputFrame);

		rgbFrame->width = w;
		rgbFrame->height = h;
		rgbFrame->format = RGB_PIXEL_FORMAT;
		rgbFrame->sample_aspect_ratio = inputFrame->sample_aspect_ratio;
	}
}

static void refreshScaledFrame(AVFrame* inputFrame)
{
	static int lastW = -1, lastH = -1;
	static int lastConW = -1, lastConH = -1;
	static enum AVPixelFormat lastPixelFormat = AV_PIX_FMT_NONE;
	static uint8_t* scaledFrameBuffer = NULL;

	int w = inputFrame->width;
	int h = inputFrame->height;
	int format = inputFrame->format;

	if (lastW != w ||
		lastH != h ||
		lastPixelFormat != format ||
		lastConW != conW ||
		lastConH != conH)
	{
		lastW = w;
		lastH = h;
		lastConW = conW;
		lastConH = conH;
		lastPixelFormat = format;

		if (scalingContext) { sws_freeContext(scalingContext); }
		if (scaledFrameBuffer) { av_free(scaledFrameBuffer); }

		scalingContext = sws_getContext(w, h, format, conW, conH, destFormat, settings.scalingMode, NULL, NULL, NULL);
		scaledFrameBuffer = (uint8_t*)av_malloc(avpicture_get_size(destFormat, conW, conH) * sizeof(uint8_t));
		
		av_image_fill_arrays(scaledFrame->data, scaledFrame->linesize, scaledFrameBuffer, destFormat, conW, conH, 1);
		av_frame_copy_props(scaledFrame, inputFrame);

		scaledFrame->width = conW;
		scaledFrame->height = conH;
		scaledFrame->format = destFormat;
		scaledFrame->sample_aspect_ratio = inputFrame->sample_aspect_ratio;

		if (settings.scaledVideoFilters) { initFiltersSV(&videoStream, scaledFrame); }
	}
}

static scaleFrame(struct SwsContext* context, AVFrame* inputFrame, AVFrame* outputFrame)
{
	sws_scale(context, inputFrame->data, inputFrame->linesize, 0,
		inputFrame->height, outputFrame->data, outputFrame->linesize);
	//av_frame_copy_props(outputFrame, inputFrame);
}