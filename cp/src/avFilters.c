#include "conplayer.h"
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>

#define CP_FILTER_ARGS_MAX_SIZE 512

static AVFilterGraph* filterGraphV = NULL;
static AVFilterGraph* filterGraphSV = NULL;
static AVFilterGraph* filterGraphA = NULL;
static AVFilterContext* buffersrcV = NULL;
static AVFilterContext* buffersrcSV = NULL;
static AVFilterContext* buffersrcA = NULL;
static AVFilterContext* buffersinkV = NULL;
static AVFilterContext* buffersinkSV = NULL;
static AVFilterContext* buffersinkA = NULL;

static void initFilters(const char* args, char* srcFilterName, const char* sinkFilterName, const char* filters,
	enum AVPixelFormat pixelFormat, AVFilterGraph** filterGraph, AVFilterContext** buffersrc, AVFilterContext** buffersink);
static bool applyFilters(AVFrame* frame, AVFilterContext* buffersrc);
static bool getFilteredFrame(AVFrame* filterFrame, AVFilterContext* buffersink);

void initFiltersV(Stream* videoStream)
{
	char args[CP_FILTER_ARGS_MAX_SIZE];

	// https://www.ffmpeg.org/ffmpeg-filters.html#buffer
	snprintf(args, CP_FILTER_ARGS_MAX_SIZE,
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		videoStream->codecContext->width,
		videoStream->codecContext->height,
		videoStream->codecContext->pix_fmt,
		videoStream->stream->time_base.num,
		videoStream->stream->time_base.den,
		videoStream->stream->sample_aspect_ratio.num,
		videoStream->stream->sample_aspect_ratio.den);

	initFilters(args, "buffer", "buffersink", settings.videoFilters,
		(enum AVPixelFormat)videoStream->codecContext->pix_fmt,
		&filterGraphV, &buffersrcV, &buffersinkV);
}

void initFiltersSV(Stream* videoStream, AVFrame* scaledFrame)
{
	char args[CP_FILTER_ARGS_MAX_SIZE];

	AVRational timeBase;
	if (buffersinkV) { timeBase = buffersinkV->inputs[0]->time_base; }
	else { timeBase = videoStream->stream->time_base; }

	// https://www.ffmpeg.org/ffmpeg-filters.html#buffer
	snprintf(args, CP_FILTER_ARGS_MAX_SIZE,
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		scaledFrame->width,
		scaledFrame->height,
		scaledFrame->format,
		timeBase.num,
		timeBase.den,
		scaledFrame->sample_aspect_ratio.num,
		scaledFrame->sample_aspect_ratio.den);

	initFilters(args, "buffer", "buffersink", settings.scaledVideoFilters,
		(enum AVPixelFormat)scaledFrame->format,
		&filterGraphSV, &buffersrcSV, &buffersinkSV);
}

void initFiltersA(Stream* audioStream)
{
	char args[CP_FILTER_ARGS_MAX_SIZE];
	AVCodecContext* ctx = audioStream->codecContext;

	// https://www.ffmpeg.org/ffmpeg-filters.html#buffer
	snprintf(args, CP_FILTER_ARGS_MAX_SIZE,
		"time_base=%d/%d:sample_rate=%d:sample_fmt=%d:channel_layout=%"PRIu64":channels=%d",
		ctx->time_base.num,
		ctx->time_base.den,
		ctx->sample_rate,
		ctx->sample_fmt,
		(uint64_t)(ctx->ch_layout.order == AV_CHANNEL_ORDER_NATIVE ? ctx->ch_layout.u.mask : 0),
		ctx->ch_layout.nb_channels);

	initFilters(args, "abuffer", "abuffersink", settings.audioFilters,
		AV_PIX_FMT_NONE, &filterGraphA, &buffersrcA, &buffersinkA);
}

bool applyFiltersV(AVFrame* videoFrame)
{
	return applyFilters(videoFrame, buffersrcV);
}

bool applyFiltersSV(AVFrame* scaledVideoFrame)
{
	return applyFilters(scaledVideoFrame, buffersrcSV);
}

bool applyFiltersA(AVFrame* audioFrame)
{
	return applyFilters(audioFrame, buffersrcA);
}

bool getFilteredFrameV(AVFrame* filterFrame)
{
	return getFilteredFrame(filterFrame, buffersinkV);
}

bool getFilteredFrameSV(AVFrame* filterFrame)
{
	return getFilteredFrame(filterFrame, buffersinkSV);
}

bool getFilteredFrameA(AVFrame* filterFrame)
{
	return getFilteredFrame(filterFrame, buffersinkA);
}

static void initFilters(const char* args, char* srcFilterName, const char* sinkFilterName, const char* filters,
	enum AVPixelFormat pixelFormat, AVFilterGraph** filterGraph, AVFilterContext** buffersrc, AVFilterContext** buffersink)
{
	avfilter_graph_free(filterGraph);
	*filterGraph = avfilter_graph_alloc();

	if (avfilter_graph_create_filter(buffersrc, avfilter_get_by_name(srcFilterName),
			"in", args, NULL, *filterGraph) < 0)
	{
		error("Failed to create filter buffer source", "avFilters.c", __LINE__);
	}

	if (avfilter_graph_create_filter(buffersink, avfilter_get_by_name(sinkFilterName),
			"out", NULL, NULL, *filterGraph) < 0)
	{
		error("Failed to create filter buffer sink", "avFilters.c", __LINE__);
	}

	if (pixelFormat != AV_PIX_FMT_NONE)
	{
		enum AVPixelFormat pixelFormats[] = { pixelFormat, AV_PIX_FMT_NONE };
		av_opt_set_int_list(*buffersink, "pix_fmts", pixelFormats,
			AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
	}

	AVFilterInOut* outputs = avfilter_inout_alloc();
	AVFilterInOut* inputs = avfilter_inout_alloc();

	outputs->name = av_strdup("in");
	outputs->filter_ctx = *buffersrc;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	inputs->name = av_strdup("out");
	inputs->filter_ctx = *buffersink;
	inputs->pad_idx = 0;
	inputs->next = NULL;
	//(*buffersink)->outputs[0]

	if (avfilter_graph_parse_ptr(*filterGraph, filters, &inputs, &outputs, NULL) < 0
			|| avfilter_graph_config(*filterGraph, NULL) < 0)
	{
		puts("\nError while initializing filters!");
		if (!settings.libavLogs) { puts("Use \"-avl\" option to get more details."); }
		error("Failed to initialize filters", "avFilters.c", __LINE__);
	}

	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);
}

static bool applyFilters(AVFrame* frame, AVFilterContext* buffersrc)
{
	return av_buffersrc_add_frame_flags(buffersrc, frame, AV_BUFFERSRC_FLAG_KEEP_REF) >= 0;
}

static bool getFilteredFrame(AVFrame* filterFrame, AVFilterContext* buffersink)
{
	int ret = av_buffersink_get_frame(buffersink, filterFrame);

	if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { return false; }
	else if (ret < 0) { error("Error while filtering frames", "avFilters.c", __LINE__); }
	else { return true; }
}