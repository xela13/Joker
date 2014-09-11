/**
 * @file
 * @copyright (C) 2012-2014 Phonations
 * @license http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 */

#include <QProgressDialog>
#include <QMessageBox>
#include "PhVideoEngine.h"

PhVideoEngine::PhVideoEngine(PhVideoSettings *settings) :
	_settings(settings),
	_fileName(""),
	_tcType(PhTimeCodeType25),
	_frameIn(0),
	_pFormatContext(NULL),
	_videoStream(NULL),
	_videoFrame(NULL),
	_currentFrame(PHFRAMEMIN),
	_useAudio(false),
	_audioStream(NULL),
	_audioFrame(NULL),
	_deinterlace(false),
	_rgb(NULL)
{
	PHDEBUG << "Using FFMpeg widget for video playback.";
	av_register_all();
	avcodec_register_all();
}

bool PhVideoEngine::ready()
{
	return (_pFormatContext && _videoStream && _videoFrame);
}

void PhVideoEngine::setDeinterlace(bool deinterlace)
{
	PHDEBUG << deinterlace;
	_deinterlace = deinterlace;
	if(_rgb) {
		delete _rgb;
		_rgb = NULL;
	}
	_currentFrame = PHFRAMEMIN;
}

bool PhVideoEngine::bilinearFiltering()
{
	return _videoRect.bilinearFiltering();
}

void PhVideoEngine::setBilinearFiltering(bool bilinear)
{
	_videoRect.setBilinearFiltering(bilinear);
}

bool PhVideoEngine::open(QString fileName)
{
	close();
	PHDEBUG << fileName;

	_clock.setTime(0);
	_clock.setRate(0);
	_currentFrame = PHFRAMEMIN;

	if(avformat_open_input(&_pFormatContext, fileName.toStdString().c_str(), NULL, NULL) < 0)
		return false;

	// Retrieve stream information
	if (avformat_find_stream_info(_pFormatContext, NULL) < 0)
		return false; // Couldn't find stream information

	av_dump_format(_pFormatContext, 0, fileName.toStdString().c_str(), 0);

	// Find video stream :
	for(int i = 0; i < (int)_pFormatContext->nb_streams; i++) {
		AVMediaType streamType = _pFormatContext->streams[i]->codec->codec_type;
		PHDEBUG << i << ":" << streamType;
		switch(streamType) {
		case AVMEDIA_TYPE_VIDEO:
			_videoStream = _pFormatContext->streams[i];
			PHDEBUG << "\t=> video";
			break;
		case AVMEDIA_TYPE_AUDIO:
			if(_useAudio && (_audioStream == NULL))
				_audioStream = _pFormatContext->streams[i];
			PHDEBUG << "\t=> audio";
			break;
		default:
			PHDEBUG << "\t=> unknown";
			break;
		}
	}

	if(_videoStream == NULL)
		return false;

	// Looking for timecode type
	_tcType = PhTimeCode::computeTimeCodeType(this->framePerSecond());
	emit timeCodeTypeChanged(_tcType);

	// Reading timestamp :
	AVDictionaryEntry *tag = av_dict_get(_pFormatContext->metadata, "timecode", NULL, AV_DICT_IGNORE_SUFFIX);
	if(tag == NULL)
		tag = av_dict_get(_videoStream->metadata, "timecode", NULL, AV_DICT_IGNORE_SUFFIX);

	if(tag) {
		PHDEBUG << "Found timestamp:" << tag->value;
		_frameIn = PhTimeCode::frameFromString(tag->value, _tcType);
	}


	PHDEBUG << "size : " << _videoStream->codec->width << "x" << _videoStream->codec->height;
	AVCodec * videoCodec = avcodec_find_decoder(_videoStream->codec->codec_id);
	if(videoCodec == NULL) {
		PHDEBUG << "Unable to find the codec:" << _videoStream->codec->codec_id;
		return false;
	}


	if (avcodec_open2(_videoStream->codec, videoCodec, NULL) < 0) {
		PHDEBUG << "Unable to open the codec:" << _videoStream->codec;
		return false;
	}

	_videoFrame = av_frame_alloc();

	PHDEBUG << "length:" << this->frameLength();
	PHDEBUG << "fps:" << this->framePerSecond();

	if(_audioStream) {
		AVCodec* audioCodec = avcodec_find_decoder(_audioStream->codec->codec_id);
		if(audioCodec) {
			if(avcodec_open2(_audioStream->codec, audioCodec, NULL) < 0) {
				PHDEBUG << "Unable to open audio codec.";
				_audioStream = NULL;
			}
			else {
				_audioFrame = av_frame_alloc();
				PHDEBUG << "Audio OK.";
			}
		}
		else {
			PHDEBUG << "Unable to find codec for audio.";
			_audioStream = NULL;
		}
	}

	decodeFrame(0);
	_fileName = fileName;

	return true;
}

void PhVideoEngine::close()
{
	PHDEBUG << _fileName;
	if(_rgb) {
		delete _rgb;
		_rgb = NULL;
	}

	if(_pFormatContext) {
		PHDEBUG << "Close the media context.";
		if(_videoStream)
			avcodec_close(_videoStream->codec);
		if(_audioStream)
			avcodec_close(_audioStream->codec);
		avformat_close_input(&_pFormatContext);
	}
	_frameIn = 0;
	_pFormatContext = NULL;
	_videoStream = NULL;
	_audioStream = NULL;
	PHDEBUG << _fileName << "closed";

	_fileName = "";
}

void PhVideoEngine::drawVideo(int x, int y, int w, int h)
{
	if(_videoStream) {
		PhFrame delay = _settings->screenDelay() * PhTimeCode::getFps(_tcType) * _clock.rate() / 1000;
		decodeFrame(_clock.frame(_tcType) + delay);
	}
	_videoRect.setRect(x, y, w, h);
	_videoRect.setZ(-10);
	_videoRect.draw();
}

void PhVideoEngine::setFrameIn(PhFrame frameIn)
{
	PHDEBUG << frameIn;
	_frameIn = frameIn;
}

void PhVideoEngine::setTimeIn(PhTime timeIn)
{
	setFrameIn(timeIn / PhTimeCode::timePerFrame(_tcType));
}

PhFrame PhVideoEngine::frameLength()
{
	if(_videoStream)
		return time2frame(_videoStream->duration);
	return 0;
}

PhTime PhVideoEngine::length()
{
	return frameLength() * PhTimeCode::timePerFrame(_tcType);
}

PhVideoEngine::~PhVideoEngine()
{
	close();
}

int PhVideoEngine::width()
{
	if(_videoStream)
		return _videoStream->codec->width;
	return 0;
}

int PhVideoEngine::height()
{
	if(_videoStream)
		return _videoStream->codec->height;
	return 0;
}

float PhVideoEngine::framePerSecond()
{
	float result = 0;
	if(_videoStream) {
		result = _videoStream->avg_frame_rate.num / _videoStream->avg_frame_rate.den;
		// See http://stackoverflow.com/a/570694/2307070
		// for NaN handling
		if(result != result) {
			result = _videoStream->time_base.den;
			result /= _videoStream->time_base.num;
		}
	}

	return result;
}

QString PhVideoEngine::codecName()
{
	if(_videoStream)
		return _videoStream->codec->codec_name;
	return "";
}

bool PhVideoEngine::decodeFrame(PhFrame frame)
{
	if(!ready()) {
		PHDEBUG << "not ready";
		return false;
	}

	if(frame < this->frameIn())
		frame = this->frameIn();
	if (frame >= this->frameOut())
		frame = this->frameOut();

	bool result = false;
	// Do not perform frame seek if the rate is 0 and the last frame is the same frame
	if (frame == _currentFrame)
		result = true;
	else {
		// Do not perform frame seek if the rate is 1 and the last frame is the previous frame
		if(frame - _currentFrame != 1) {
			int flags = AVSEEK_FLAG_ANY;
			int64_t timestamp = frame2time(frame - _frameIn);
			PHDEBUG << "seek:" << frame;
			av_seek_frame(_pFormatContext, _videoStream->index, timestamp, flags);
		}

		AVPacket packet;

		bool lookingForVideoFrame = true;
		while(lookingForVideoFrame) {
			int error = av_read_frame(_pFormatContext, &packet);
			switch(error) {
			case 0:
				if(packet.stream_index == _videoStream->index) {
					_currentFrame = frame;

					int frameFinished = 0;
					avcodec_decode_video2(_videoStream->codec, _videoFrame, &frameFinished, &packet);
					if(frameFinished) {

						int frameHeight = _videoFrame->height;
						if(_deinterlace)
							frameHeight = _videoFrame->height / 2;

						// As the following formats are deprecated (see https://libav.org/doxygen/master/pixfmt_8h.html#a9a8e335cf3be472042bc9f0cf80cd4c5)
						// we replace its with the new ones recommended by LibAv
						// in order to get ride of the warnings
						AVPixelFormat pixFormat;
						switch (_videoStream->codec->pix_fmt) {
						case AV_PIX_FMT_YUVJ420P:
							pixFormat = AV_PIX_FMT_YUV420P;
							break;
						case AV_PIX_FMT_YUVJ422P:
							pixFormat = AV_PIX_FMT_YUV422P;
							break;
						case AV_PIX_FMT_YUVJ444P:
							pixFormat = AV_PIX_FMT_YUV444P;
							break;
						case AV_PIX_FMT_YUVJ440P:
							pixFormat = AV_PIX_FMT_YUV440P;
						default:
							pixFormat = _videoStream->codec->pix_fmt;
							break;
						}
						/* Note: we output the frames in AV_PIX_FMT_BGRA rather than AV_PIX_FMT_RGB24,
						 * because this format is native to most video cards and will avoid a conversion
						 * in the video driver */
						SwsContext * swsContext = sws_getContext(_videoFrame->width, _videoStream->codec->height, pixFormat,
						                                         _videoStream->codec->width, frameHeight, AV_PIX_FMT_BGRA,
						                                         SWS_POINT, NULL, NULL, NULL);

						if(_rgb == NULL)
							_rgb = new uint8_t[avpicture_get_size(AV_PIX_FMT_BGRA, _videoFrame->width, frameHeight)];
						int linesize = _videoFrame->width * 4;
						if (0 <= sws_scale(swsContext, (const uint8_t * const *) _videoFrame->data,
						                   _videoFrame->linesize, 0, _videoStream->codec->height, &_rgb,
						                   &linesize)) {

							_videoRect.createTextureFromBGRABuffer(_rgb, _videoFrame->width, frameHeight);


							_videoFrameTickCounter.tick();
							result = true;
						}
						lookingForVideoFrame = false;
					} // if frame decode is not finished, let's read another packet.
				}
				else if(_audioStream && (packet.stream_index == _audioStream->index)) {
					int ok = 0;
					avcodec_decode_audio4(_audioStream->codec, _audioFrame, &ok, &packet);
					if(ok) {
						PHDEBUG << "audio:" << _audioFrame->nb_samples;
					}
				}
				break;
			case AVERROR_INVALIDDATA:
			case AVERROR_EOF:
			default:
				{
					char errorStr[256];
					av_strerror(error, errorStr, 256);
					PHDEBUG << frame << "error:" << errorStr;
					lookingForVideoFrame = false;
					break;
				}
			}
			//Avoid memory leak
			av_free_packet(&packet);
		}
	}

	return result;
}

int64_t PhVideoEngine::frame2time(PhFrame f)
{
	int64_t t = 0;
	if(_videoStream) {
		PhFrame fps = PhTimeCode::getFps(_tcType);
		t = f * _videoStream->time_base.den / _videoStream->time_base.num / fps;
	}
	return t;
}

PhFrame PhVideoEngine::time2frame(int64_t t)
{
	PhFrame f = 0;
	if(_videoStream) {
		PhFrame fps = PhTimeCode::getFps(_tcType);
		f = t * _videoStream->time_base.num * fps / _videoStream->time_base.den;
	}
	return f;
}

void PhVideoEngine::startEncoder()
{
	_currentEncodedFrame = 0;
	QString timeOut = PhTimeCode::stringFromFrame(_frameIn + frameLength(), PhTimeCode::computeTimeCodeType(framePerSecond()));
	QString outputFile = _fileName.split(".").first() + "_MJPEG." + _fileName.split(".").last();
	QProgressDialog progressionDialog;
	progressionDialog.show();
	progressionDialog.setGeometry(progressionDialog.x(), progressionDialog.y(), 600, progressionDialog.height());
	progressionDialog.setMinimum(0);
	progressionDialog.setMaximum(frameLength());

	int ret;
	AVPacket packet = { .data = NULL, .size = 0 };
	AVFrame *frame = NULL;
	enum AVMediaType type;
	unsigned int stream_index;
	unsigned int i;
	int got_frame;
	int (*dec_func)(AVCodecContext *, AVFrame *, int *, const AVPacket *);

	avfilter_register_all();
	if ((ret = open_input_file(_fileName.toStdString().c_str())) < 0)
		goto end;
	if ((ret = open_output_file(outputFile.toStdString().c_str())) < 0)
		goto end;
	if ((ret = init_filters()) < 0)
		goto end;

	/* read all packets */
	while (1) {
		if ((ret = av_read_frame(ifmt_ctx, &packet)) < 0) {
			PHDEBUG << "out at read frame";
			break;
		}
		stream_index = packet.stream_index;
		type = ifmt_ctx->streams[packet.stream_index]->codec->codec_type;
		if (filter_ctx[stream_index].filter_graph) {
			frame = av_frame_alloc();
			if (!frame) {
				ret = AVERROR(ENOMEM);
				{
					PHDEBUG << "frame alloc error";
					break;
				}
			}
			packet.dts = av_rescale_q_rnd(packet.dts,
			                              ifmt_ctx->streams[stream_index]->time_base,
			                              ifmt_ctx->streams[stream_index]->codec->time_base,
			                              (AVRounding) (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
			packet.pts = av_rescale_q_rnd(packet.pts,
			                              ifmt_ctx->streams[stream_index]->time_base,
			                              ifmt_ctx->streams[stream_index]->codec->time_base,
			                              (AVRounding) (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
			dec_func = (type == AVMEDIA_TYPE_VIDEO) ? avcodec_decode_video2 :
			           avcodec_decode_audio4;
			ret = dec_func(ifmt_ctx->streams[stream_index]->codec, frame,
			               &got_frame, &packet);
			if (ret < 0) {
				av_frame_free(&frame);
				PHDEBUG <<  "Decoding failed";
				break;
			}
			if (got_frame) {
				frame->pts = av_frame_get_best_effort_timestamp(frame);
				ret = filter_encode_write_frame(frame, stream_index);

				if(type == AVMEDIA_TYPE_VIDEO) {
					if(_currentEncodedFrame % 10 == 0) {
						progressionDialog.setValue(_currentEncodedFrame);
						progressionDialog.setLabelText(PhTimeCode::stringFromFrame(_frameIn + _currentEncodedFrame, PhTimeCode::computeTimeCodeType(framePerSecond()))
						                               + "/" + timeOut);
						QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
					}
					_currentEncodedFrame++;
					if(progressionDialog.wasCanceled())
						goto end;
				}

				av_frame_free(&frame);
				if (ret < 0) {
					PHDEBUG << "filter encode write error";
					goto end;
				}
			}
			else {
				av_frame_free(&frame);
			}
		}
		else {
			/* remux this frame without reencoding */
			packet.dts = av_rescale_q_rnd(packet.dts,
			                              ifmt_ctx->streams[stream_index]->time_base,
			                              ofmt_ctx->streams[stream_index]->time_base,
			                              (AVRounding) (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
			packet.pts = av_rescale_q_rnd(packet.pts,
			                              ifmt_ctx->streams[stream_index]->time_base,
			                              ofmt_ctx->streams[stream_index]->time_base,
			                              (AVRounding) (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
			ret = av_interleaved_write_frame(ofmt_ctx, &packet);
			if (ret < 0) {
				PHDEBUG << "av interleaved frame error";
				goto end;
			}
		}
		av_free_packet(&packet);
	}
	/* flush filters and encoders */
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		/* flush filter */
		if (!filter_ctx[i].filter_graph)
			continue;
		ret = filter_encode_write_frame(NULL, i);
		if (ret < 0) {
			PHDEBUG <<  "Flushing filter failed";
			goto end;
		}
		/* flush encoder */
		ret = flush_encoder(i);
		if (ret < 0) {
			PHDEBUG <<  "Flushing encoder failed";
			goto end;
		}
	}
	av_write_trailer(ofmt_ctx);
end:
	av_free_packet(&packet);
	av_frame_free(&frame);
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		if (ofmt_ctx && ofmt_ctx->nb_streams > i && ofmt_ctx->streams[i] && ofmt_ctx->streams[i]->codec)
			avcodec_close(ofmt_ctx->streams[i]->codec);
		if (filter_ctx && filter_ctx[i].filter_graph)
			avfilter_graph_free(&filter_ctx[i].filter_graph);
	}
	av_free(filter_ctx);
	avformat_close_input(&ifmt_ctx);
	if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
	if (ret < 0)
		PHDEBUG <<  "Error occurred:" << av_err2str(ret);
	else {
		progressionDialog.setValue(progressionDialog.maximum());
		QMessageBox msgBox;
		msgBox.setText("Conversion succeed");
		msgBox.setInformativeText("Do you want to load the new MJPEG file ?\n" + outputFile);
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		msgBox.setDefaultButton(QMessageBox::Yes);
		if(msgBox.exec() == QMessageBox::Yes) {
			open(outputFile);
		}
	}
}

int PhVideoEngine::open_input_file(const char *filename)
{
	int ret;
	unsigned int i;
	ifmt_ctx = NULL;
	if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0) {
		PHDEBUG <<  "Cannot open input file";
		return ret;
	}
	if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
		PHDEBUG << "Cannot find stream information";
		return ret;
	}
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		AVStream *stream;
		AVCodecContext *codec_ctx;
		stream = ifmt_ctx->streams[i];
		codec_ctx = stream->codec;
		/* Reencode video & audio and remux subtitles etc. */
		if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
		    || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
			/* Open decoder */
			ret = avcodec_open2(codec_ctx,
			                    avcodec_find_decoder(codec_ctx->codec_id), NULL);
			if (ret < 0) {
				PHDEBUG << "Failed to open decoder for stream #" << i;
				return ret;
			}
		}
	}
	av_dump_format(ifmt_ctx, 0, filename, 0);
	return 0;
}
int PhVideoEngine::open_output_file(const char *filename)
{
	AVStream *out_stream;
	AVStream *in_stream;
	AVCodecContext *dec_ctx, *enc_ctx;
	AVCodec *encoder;
	int ret;
	unsigned int i;
	ofmt_ctx = NULL;
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename);
	if (!ofmt_ctx) {
		PHDEBUG <<  "Could not create output context";
		return AVERROR_UNKNOWN;
	}
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		out_stream = avformat_new_stream(ofmt_ctx, NULL);
		if (!out_stream) {
			PHDEBUG <<  "Failed allocating output stream";
			return AVERROR_UNKNOWN;
		}
		in_stream = ifmt_ctx->streams[i];
		dec_ctx = in_stream->codec;
		enc_ctx = out_stream->codec;
		if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
		    || dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
			/* In this example, we transcode to same properties (picture size,
			 * sample rate etc.). These properties can be changed for output
			 * streams easily using filters */
			if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
				encoder = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
				enc_ctx->height = dec_ctx->height;
				enc_ctx->width = dec_ctx->width;
				enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
				/* take first format from list of supported formats */
				enc_ctx->pix_fmt = encoder->pix_fmts[0];
				/* video time_base can be set to whatever is handy and supported by encoder */
				enc_ctx->time_base = dec_ctx->time_base;
				enc_ctx->bit_rate = dec_ctx->bit_rate * 8;
			}
			else {
				encoder = avcodec_find_encoder(AV_CODEC_ID_MP3);
				enc_ctx->sample_rate = dec_ctx->sample_rate;
				enc_ctx->channel_layout = dec_ctx->channel_layout;
				enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
				/* take first format from list of supported formats */
				enc_ctx->sample_fmt = encoder->sample_fmts[0];
				enc_ctx->time_base = (AVRational){1, enc_ctx->sample_rate};
			}
			/* Third parameter can be used to pass settings to encoder */
			ret = avcodec_open2(enc_ctx, encoder, NULL);
			if (ret < 0) {
				PHDEBUG <<  "Cannot open video encoder for stream #"<< i;
				return ret;
			}
		}
		else if (dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN) {
			PHDEBUG << "Elementary stream #"<< i << " is of unknown type, cannot proceed";
			return AVERROR_INVALIDDATA;
		}
		else {
			/* if this stream must be remuxed */
			ret = avcodec_copy_context(ofmt_ctx->streams[i]->codec,
			                           ifmt_ctx->streams[i]->codec);
			if (ret < 0) {
				PHDEBUG <<  "Copying stream context failed";
				return ret;
			}
		}
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			enc_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	av_dump_format(ofmt_ctx, 0, filename, 1);
	if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			PHDEBUG <<  "Could not open output file" << filename;
			return ret;
		}
	}
	/* init muxer, write output file header */
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		PHDEBUG <<  "Error occurred when opening output file";
		return ret;
	}
	return 0;
}
int PhVideoEngine::init_filter(FilteringContext* fctx, AVCodecContext *dec_ctx, AVCodecContext *enc_ctx, const char *filter_spec)
{
	char args[512];
	int ret = 0;
	AVFilter *buffersrc = NULL;
	AVFilter *buffersink = NULL;
	AVFilterContext *buffersrc_ctx = NULL;
	AVFilterContext *buffersink_ctx = NULL;
	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs = avfilter_inout_alloc();
	AVFilterGraph *filter_graph = avfilter_graph_alloc();
	if (!outputs || !inputs || !filter_graph) {
		ret = AVERROR(ENOMEM);
		goto end;
	}
	if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
		buffersrc = avfilter_get_by_name("buffer");
		buffersink = avfilter_get_by_name("buffersink");
		if (!buffersrc || !buffersink) {
			PHDEBUG <<  "filtering source or sink element not found";
			ret = AVERROR_UNKNOWN;
			goto end;
		}
		snprintf(args, sizeof(args),
		         "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		         dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
		         dec_ctx->time_base.num, dec_ctx->time_base.den,
		         dec_ctx->sample_aspect_ratio.num,
		         dec_ctx->sample_aspect_ratio.den);
		ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
		                                   args, NULL, filter_graph);
		if (ret < 0) {
			PHDEBUG <<  "Cannot create buffer source";
			goto end;
		}
		ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
		                                   NULL, NULL, filter_graph);
		if (ret < 0) {
			PHDEBUG <<  "Cannot create buffer sink";
			goto end;
		}
		ret = av_opt_set_bin(buffersink_ctx, "pix_fmts",
		                     (uint8_t*)&enc_ctx->pix_fmt, sizeof(enc_ctx->pix_fmt),
		                     AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			PHDEBUG <<  "Cannot set output pixel format";
			goto end;
		}
	}
	else if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
		buffersrc = avfilter_get_by_name("abuffer");
		buffersink = avfilter_get_by_name("abuffersink");
		if (!buffersrc || !buffersink) {
			PHDEBUG <<  "filtering source or sink element not found";
			ret = AVERROR_UNKNOWN;
			goto end;
		}
		if (!dec_ctx->channel_layout)
			dec_ctx->channel_layout =
			    av_get_default_channel_layout(dec_ctx->channels);
		snprintf(args, sizeof(args),
		         "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
		         dec_ctx->time_base.num, dec_ctx->time_base.den, dec_ctx->sample_rate,
		         av_get_sample_fmt_name(dec_ctx->sample_fmt),
		         dec_ctx->channel_layout);
		ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
		                                   args, NULL, filter_graph);
		if (ret < 0) {
			PHDEBUG <<  "Cannot create audio buffer source";
			goto end;
		}
		ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
		                                   NULL, NULL, filter_graph);
		if (ret < 0) {
			PHDEBUG <<  "Cannot create audio buffer sink";
			goto end;
		}
		ret = av_opt_set_bin(buffersink_ctx, "sample_fmts",
		                     (uint8_t*)&enc_ctx->sample_fmt, sizeof(enc_ctx->sample_fmt),
		                     AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			PHDEBUG <<  "Cannot set output sample format";
			goto end;
		}
		ret = av_opt_set_bin(buffersink_ctx, "channel_layouts",
		                     (uint8_t*)&enc_ctx->channel_layout,
		                     sizeof(enc_ctx->channel_layout), AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			PHDEBUG <<  "Cannot set output channel layout";
			goto end;
		}
		ret = av_opt_set_bin(buffersink_ctx, "sample_rates",
		                     (uint8_t*)&enc_ctx->sample_rate, sizeof(enc_ctx->sample_rate),
		                     AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) {
			PHDEBUG <<  "Cannot set output sample rate";
			goto end;
		}
	}
	else {
		ret = AVERROR_UNKNOWN;
		goto end;
	}
	/* Endpoints for the filter graph. */
	outputs->name = av_strdup("in");
	outputs->filter_ctx = buffersrc_ctx;
	outputs->pad_idx = 0;
	outputs->next = NULL;
	inputs->name = av_strdup("out");
	inputs->filter_ctx = buffersink_ctx;
	inputs->pad_idx = 0;
	inputs->next = NULL;
	if (!outputs->name || !inputs->name) {
		ret = AVERROR(ENOMEM);
		goto end;
	}
	if ((ret = avfilter_graph_parse_ptr(filter_graph, filter_spec,
	                                    &inputs, &outputs, NULL)) < 0)
		goto end;
	if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
		goto end;
	/* Fill FilteringContext */
	fctx->buffersrc_ctx = buffersrc_ctx;
	fctx->buffersink_ctx = buffersink_ctx;
	fctx->filter_graph = filter_graph;
end:
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);
	return ret;
}
int PhVideoEngine::init_filters(void)
{
	const char *filter_spec;
	unsigned int i;
	int ret;
	filter_ctx = (FilteringContext *) av_malloc_array(ifmt_ctx->nb_streams, sizeof(*filter_ctx));
	if (!filter_ctx)
		return AVERROR(ENOMEM);
	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		filter_ctx[i].buffersrc_ctx = NULL;
		filter_ctx[i].buffersink_ctx = NULL;
		filter_ctx[i].filter_graph = NULL;
		if (!(ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
		      || ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO))
			continue;
		if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			filter_spec = "null"; /* passthrough (dummy) filter for video */
		else
			filter_spec = "anull"; /* passthrough (dummy) filter for audio */
		ret = init_filter(&filter_ctx[i], ifmt_ctx->streams[i]->codec,
		                  ofmt_ctx->streams[i]->codec, filter_spec);
		if (ret)
			return ret;
	}
	return 0;
}
int PhVideoEngine::encode_write_frame(AVFrame *filt_frame, unsigned int stream_index, int *got_frame) {
	int ret;
	int got_frame_local;
	AVPacket enc_pkt;
	int (*enc_func)(AVCodecContext *, AVPacket *, const AVFrame *, int *) =
	    (ifmt_ctx->streams[stream_index]->codec->codec_type ==
	     AVMEDIA_TYPE_VIDEO) ? avcodec_encode_video2 : avcodec_encode_audio2;
	if (!got_frame)
		got_frame = &got_frame_local;

	/* encode filtered frame */
	enc_pkt.data = NULL;
	enc_pkt.size = 0;
	av_init_packet(&enc_pkt);
	ret = enc_func(ofmt_ctx->streams[stream_index]->codec, &enc_pkt,
	               filt_frame, got_frame);
	av_frame_free(&filt_frame);
	if (ret < 0)
		return ret;
	if (!(*got_frame))
		return 0;
	/* prepare packet for muxing */
	enc_pkt.stream_index = stream_index;
	enc_pkt.dts = av_rescale_q_rnd(enc_pkt.dts,
	                               ofmt_ctx->streams[stream_index]->codec->time_base,
	                               ofmt_ctx->streams[stream_index]->time_base,
	                               (AVRounding) (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
	enc_pkt.pts = av_rescale_q_rnd(enc_pkt.pts,
	                               ofmt_ctx->streams[stream_index]->codec->time_base,
	                               ofmt_ctx->streams[stream_index]->time_base,
	                               (AVRounding) (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
	enc_pkt.duration = av_rescale_q(enc_pkt.duration,
	                                ofmt_ctx->streams[stream_index]->codec->time_base,
	                                ofmt_ctx->streams[stream_index]->time_base);

	/* mux encoded frame */
	ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
	return ret;
}
int PhVideoEngine::filter_encode_write_frame(AVFrame *frame, unsigned int stream_index)
{
	int ret;
	AVFrame *filt_frame;
	//PHDEBUG << AV_LOG_INFO, "Pushing decoded frame to filters";
	/* push the decoded frame into the filtergraph */
	ret = av_buffersrc_add_frame_flags(filter_ctx[stream_index].buffersrc_ctx,
	                                   frame, 0);
	if (ret < 0) {
		PHDEBUG <<  "Error while feeding the filtergraph";
		return ret;
	}
	/* pull filtered frames from the filtergraph */
	while (1) {
		filt_frame = av_frame_alloc();
		if (!filt_frame) {
			ret = AVERROR(ENOMEM);
			break;
		}
		//PHDEBUG << AV_LOG_INFO, "Pulling filtered frame from filters";
		ret = av_buffersink_get_frame(filter_ctx[stream_index].buffersink_ctx,
		                              filt_frame);
		if (ret < 0) {
			/* if no more frames for output - returns AVERROR(EAGAIN)
			 * if flushed and no more frames for output - returns AVERROR_EOF
			 * rewrite retcode to 0 to show it as normal procedure completion
			 */
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				ret = 0;
			av_frame_free(&filt_frame);
			break;
		}
		filt_frame->pict_type = AV_PICTURE_TYPE_NONE;
		ret = encode_write_frame(filt_frame, stream_index, NULL);
		if (ret < 0)
			break;
	}
	return ret;
}
int PhVideoEngine::flush_encoder(unsigned int stream_index)
{
	int ret;
	int got_frame;
	if (!(ofmt_ctx->streams[stream_index]->codec->codec->capabilities &
	      CODEC_CAP_DELAY))
		return 0;
	while (1) {
		PHDEBUG << "Flushing stream #"<< stream_index << " encoder";
		ret = encode_write_frame(NULL, stream_index, &got_frame);
		if (ret < 0)
			break;
		if (!got_frame)
			return 0;
	}
	return ret;
}
