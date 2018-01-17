/*
 * ffmpeg_test
 * 2016.2.28
 */


/*
 * FFmpeg�������ã�
 * ���ð���Ŀ¼����Ŀ¼������������
 * ���dll������debug�ļ���
 */


/*
 * libavcodec        encoding/decoding library
 * libavfilter        graph-based frame editing library
 * libavformat        I/O and muxing/demuxing library
 * libavdevice        special devices muxing/demuxing library
 * libavutil        common utility library
 * libswresample    audio resampling, format conversion and mixing
 * libpostproc        post processing library
 * libswscale        color conversion and scaling library
 */


#include <stdio.h>


/* FFmpeg libraries */
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>



/*
 * ///////////////////////////////////////////////////////////////////////////////////////
 * ���װ������Ƶ����
 */

int main( int argc, char* argv[] )
{
	AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx_audio = NULL, *ofmt_ctx_video = NULL;
	AVPacket	pkt;
	AVOutputFormat	*ofmt_audio = NULL, *ofmt_video = NULL;

	int	videoindex	= -1, audioindex = -1;
	int	frame_index	= 0;

	const char	*in_filename		= "source.200kbps.768x320.flv";
	const char	*out_filename_video	= "source.200kbps.768x320.h264";
	const char	*out_filename_audio	= "source.200kbps.768x320.aac";

	av_register_all();

	if ( avformat_open_input( &ifmt_ctx, in_filename, NULL, NULL ) != 0 )
	{
		printf( "Couldn't open input file.\n" );
		return(-1);
	}
	if ( avformat_find_stream_info( ifmt_ctx, NULL ) < 0 )
	{
		printf( "Couldn't find stream information.\n" );
		return(-1);
	}

	/* Allocate an AVFormatContext for an output format */
	avformat_alloc_output_context2( &ofmt_ctx_video, NULL, NULL, out_filename_video );
	if ( !ofmt_ctx_video )
	{
		printf( "Couldn't create output context.\n" );
		return(-1);
	}
	ofmt_video = ofmt_ctx_video->oformat;

	avformat_alloc_output_context2( &ofmt_ctx_audio, NULL, NULL, out_filename_audio );
	if ( !ofmt_ctx_audio )
	{
		printf( "Couldn't create output context.\n" );
		return(-1);
	}
	ofmt_audio = ofmt_ctx_audio->oformat;

	/* һ�������nb_streamsֻ��������������streams[0],streams[1]���ֱ�����Ƶ����Ƶ��������˳�򲻶� */
	for ( int i = 0; i < ifmt_ctx->nb_streams; i++ )
	{
		AVFormatContext *ofmt_ctx;
		AVStream	*in_stream	= ifmt_ctx->streams[i];
		AVStream	*out_stream	= NULL;

		/* ��������Ƶ���ͣ�������������������� */
		if ( ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
		{
			videoindex	= i;
			out_stream	= avformat_new_stream( ofmt_ctx_video, in_stream->codec->codec );
			ofmt_ctx	= ofmt_ctx_video;
		}else if ( ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
		{
			audioindex	= i;
			out_stream	= avformat_new_stream( ofmt_ctx_audio, in_stream->codec->codec );
			ofmt_ctx	= ofmt_ctx_audio;
		}else
			break;

		if ( !out_stream )
		{
			printf( "Failed allocating output stream.\n" );
			return(-1);
		}
		/* ���Ƶ������ */
		if ( avcodec_copy_context( out_stream->codec, in_stream->codec ) < 0 )
		{
			printf( "Failed to copy context from input to output stream codec context.\n" );
			return(-1);
		}
		out_stream->codec->codec_tag = 0;

		if ( ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER )
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

	/* Print detailed information about the input or output format */
	printf( "\n==============Input Video=============\n" );
	av_dump_format( ifmt_ctx, 0, in_filename, 0 );
	printf( "\n==============Output Video============\n" );
	av_dump_format( ofmt_ctx_video, 0, out_filename_video, 1 );
	printf( "\n==============Output Audio============\n" );
	av_dump_format( ofmt_ctx_audio, 0, out_filename_audio, 1 );
	printf( "\n======================================\n" );

	/* ������ļ� */
	if ( !(ofmt_video->flags & AVFMT_NOFILE) )
	{
		if ( avio_open( &ofmt_ctx_video->pb, out_filename_video, AVIO_FLAG_WRITE ) < 0 )
		{
			printf( "Couldn't open output file '%s'", out_filename_video );
			return(-1);
		}
	}
	if ( !(ofmt_audio->flags & AVFMT_NOFILE) )
	{
		if ( avio_open( &ofmt_ctx_audio->pb, out_filename_audio, AVIO_FLAG_WRITE ) < 0 )
		{
			printf( "Couldn't open output file '%s'", out_filename_audio );
			return(-1);
		}
	}

	/* д�ļ�ͷ�� */
	if ( avformat_write_header( ofmt_ctx_video, NULL ) < 0 )
	{
		printf( "Error occurred when opening video output file\n" );
		return(-1);
	}
	if ( avformat_write_header( ofmt_ctx_audio, NULL ) < 0 )
	{
		printf( "Error occurred when opening video output file\n" );
		return(-1);
	}

	/*
	 * ����ĳЩ��װ��ʽ������MP4/FLV/MKV�ȣ��е�H.264��ʱ����Ҫ����д��SPS��PPS������ᵼ�·������������
	 * û��SPS��PPS���޷����š�ʹ��ffmpeg������Ϊ��h264_mp4toannexb����bitstream filter����
	 */
	AVBitStreamFilterContext *h264bsfc = av_bitstream_filter_init( "h264_mp4toannexb" );

	while ( 1 )
	{
		AVFormatContext *ofmt_ctx;
		AVStream	*in_stream, *out_stream;

		/* Get an AVPacket */
		if ( av_read_frame( ifmt_ctx, &pkt ) < 0 )
			break;
		in_stream = ifmt_ctx->streams[pkt.stream_index];
		/* stream_index��ʶ��AVPacket��������Ƶ/��Ƶ�� */
		if ( pkt.stream_index == videoindex )
		{
			/*
			 * ǰ���Ѿ�ͨ��avcodec_copy_context()������������Ƶ/��Ƶ�Ĳ��������������Ƶ/��Ƶ��AVCodecContext�ṹ��
			 * ����ʹ�õľ���ofmt_ctx_video�ĵ�һ����streams[0]
			 */
			out_stream	= ofmt_ctx_video->streams[0];
			ofmt_ctx	= ofmt_ctx_video;
			printf( "Write Video Packet. size:%d\tpts:%lld\n", pkt.size, pkt.pts );
			av_bitstream_filter_filter( h264bsfc, in_stream->codec, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0 );
		}
		else if ( pkt.stream_index == audioindex )
		{
			out_stream	= ofmt_ctx_audio->streams[0];
			ofmt_ctx	= ofmt_ctx_audio;
			printf( "Write Audio Packet. size:%d\tpts:%lld\n", pkt.size, pkt.pts );
		}else
			continue;

		/*
		 * DTS��Decoding Time Stamp������ʱ���
		 * PTS��Presentation Time Stamp����ʾʱ���
		 * ת��PTS/DTS
		 */
		pkt.pts			= av_rescale_q_rnd( pkt.pts, in_stream->time_base, out_stream->time_base, (enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX) );
		pkt.dts			= av_rescale_q_rnd( pkt.dts, in_stream->time_base, out_stream->time_base, (enum AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX) );
		pkt.duration		= av_rescale_q( pkt.duration, in_stream->time_base, out_stream->time_base );
		pkt.pos			= -1;
		pkt.stream_index	= 0;

		/* д */
		if ( av_interleaved_write_frame( ofmt_ctx, &pkt ) < 0 )
		{
			printf( "Error muxing packet\n" );
			break;
		}

		printf( "Write %8d frames to output file\n", frame_index );
		av_free_packet( &pkt );
		frame_index++;
	}

	av_bitstream_filter_close( h264bsfc );

	/* д�ļ�β�� */
	av_write_trailer( ofmt_ctx_video );
	av_write_trailer( ofmt_ctx_audio );


	avformat_close_input( &ifmt_ctx );
	if ( ofmt_ctx_video && !(ofmt_video->flags & AVFMT_NOFILE) )
		avio_close( ofmt_ctx_video->pb );

	if ( ofmt_ctx_audio && !(ofmt_audio->flags & AVFMT_NOFILE) )
		avio_close( ofmt_ctx_audio->pb );

	avformat_free_context( ofmt_ctx_video );
	avformat_free_context( ofmt_ctx_audio );

	return(0);
}

