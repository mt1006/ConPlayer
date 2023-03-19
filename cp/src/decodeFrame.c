#include "conplayer.h"

static const double CONSOLE_REFRESH_PERIOD = 0.2;

static Stream videoStream = { -1 };
static Stream audioStream = { -1 };
static enum AVPixelFormat destFormat;
static AVFormatContext* formatContext;
static AVFormatContext* secondFormatContext = NULL;
static AVFormatContext* audioContext;
static AVFormatContext* videoContext;
static struct SwsContext* swsContext = NULL;
static uint8_t* scaledFrameBuffer = NULL;
static int lastFrame = -1;
static volatile bool useAVSeek = false;
static volatile int64_t seekTimestamp = 0;

static AVFormatContext* loadContextAndStreams(const char* file);
static void decodeVideoPacket(AVPacket* packet, AVFrame* videoFrame, AVFrame* filterFrame, AVFrame* scaledFrame);
static void decodeAudioPacket(AVPacket* packet, AVFrame* audioFrame, AVFrame* filterFrame);
static void addVideoFrame(AVFrame* frame);
static void refreshFrameSize(AVFrame* inputFrame, AVFrame* outputFrame);

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
}

void readFrames(void)
{
	double lastConRefresh = 0.0;

	AVPacket* packet = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();
	AVFrame* filterFrame = av_frame_alloc();
	AVFrame* scaledFrame = av_frame_alloc();
	double lastDTS = DBL_MIN;
	int err1 = 0, err2 = 0;

	while (true)
	{
		if (settings.useFakeConsole) { peekMessages(); }

		while (freezeThreads)
		{
			mainFreezed = true;
			if (settings.useFakeConsole) { peekMessages(); }
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
					decodeVideoPacket(packet, frame, filterFrame, scaledFrame);
					dts = (double)packet->dts * (double)videoStream.stream->time_base.num /
						(double)videoStream.stream->time_base.den;
				}
				else if (packet->stream_index == audioStream.index && formatContext == audioContext)
				{
					decodeAudioPacket(packet, frame, filterFrame);
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
					decodeVideoPacket(packet, frame, filterFrame, scaledFrame);
					dts = (double)packet->dts * (double)videoStream.stream->time_base.num /
						(double)videoStream.stream->time_base.den;
				}
				else if (packet->stream_index == audioStream.index && secondFormatContext == audioContext)
				{
					decodeAudioPacket(packet, frame, filterFrame);
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

	av_frame_free(&scaledFrame);
	av_frame_free(&filterFrame);
	av_frame_free(&frame);
	av_packet_free(&packet);

	printf("%d (%d) / %d (%d)\n", (-err1) & 0xFF, err1, (-err2) & 0xFF, err2);
	decodeEnd = true;
	while (true)
	{
		if (settings.useFakeConsole) { peekMessages(); }
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

static void refreshFrameSize(AVFrame* inputFrame, AVFrame* outputFrame)
{
	static int lastFrameW = -1, lastFrameH = -1;
	static int lastConsoleW = -1, lastConsoleH = -1;
	static enum AVPixelFormat lastPixelFormat = AV_PIX_FMT_NONE;

	if (lastFrameW != inputFrame->width ||
		lastFrameH != inputFrame->height ||
		lastPixelFormat != inputFrame->format ||
		lastConsoleW != conW ||
		lastConsoleH != conH)
	{
		lastFrameW = inputFrame->width;
		lastFrameH = inputFrame->height;
		lastConsoleW = conW;
		lastConsoleH = conH;
		lastPixelFormat = inputFrame->format;

		if (swsContext) { sws_freeContext(swsContext); }
		if (scaledFrameBuffer) { av_free(scaledFrameBuffer); }

		swsContext = sws_getContext(inputFrame->width, inputFrame->height,
			inputFrame->format, conW, conH, destFormat, settings.scalingMode, NULL, NULL, NULL);
		scaledFrameBuffer = (uint8_t*)av_malloc(avpicture_get_size(destFormat, conW, conH) * sizeof(uint8_t));
		
		av_image_fill_arrays(outputFrame->data, outputFrame->linesize,
			scaledFrameBuffer, destFormat, conW, conH, 1);
		av_frame_copy_props(outputFrame, inputFrame);

		outputFrame->width = conW;
		outputFrame->height = conH;
		outputFrame->format = destFormat;
		outputFrame->sample_aspect_ratio = inputFrame->sample_aspect_ratio;

		if (settings.scaledVideoFilters) { initFiltersSV(&videoStream, outputFrame); }
	}
}