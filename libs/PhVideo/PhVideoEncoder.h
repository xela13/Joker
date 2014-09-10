/**
 * Copyright (C) 2012-2014 Phonations
 * License: http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 */

#ifndef PHVIDEOENCODER_H
#define PHVIDEOENCODER_H



extern "C" {
#ifndef INT64_C
/** see http://code.google.com/p/ffmpegsource/issues/detail?id=11#c13 */
#define INT64_C(c) (c ## LL)
/** and http://code.google.com/p/ffmpegsource/issues/detail?id=11#c23 */
#define UINT64_C(c) (c ## ULL)
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/avcodec.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
}
#include <QString>
#include <PhTools/PhDebug.h>

static AVFormatContext *ifmt_ctx;
static AVFormatContext *ofmt_ctx;
typedef struct FilteringContext {
	AVFilterContext *buffersink_ctx;
	AVFilterContext *buffersrc_ctx;
	AVFilterGraph *filter_graph;
} FilteringContext;
static FilteringContext *filter_ctx;


class PhVideoEncoder
{
public:

	PhVideoEncoder(QString inputFile);
private:
	int open_input_file(const char *filename);
	int open_output_file(const char *filename);
	int init_filter(FilteringContext* fctx, AVCodecContext *dec_ctx, AVCodecContext *enc_ctx, const char *filter_spec);
	int init_filters(void);
	int encode_write_frame(AVFrame *filt_frame, unsigned int stream_index, int *got_frame);
	int filter_encode_write_frame(AVFrame *frame, unsigned int stream_index);
	int flush_encoder(unsigned int stream_index);

	int currentFrame;

};

#endif // PHVIDEOENCODER_H
