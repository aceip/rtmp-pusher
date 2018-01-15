#ifdef __cplusplus
extern "C"
{
#endif

#include <math.h>
#include <stdlib.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/frame.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif


int main( int argc, char **argv )
{
	AVFrame		*frame;
	AVCodec		*codec = NULL;
	AVPacket	packet;
	AVCodecContext	*codecContext;
	int		readSize	= 0;
	int		ret		= 0, getPacket;
	FILE		* fileIn, *fileOut;
	int		frameCount = 0;
	/* register all the codecs */
	av_register_all();

	if (argc != 4)
	{
		fprintf( stdout, "usage:./encodec.bin xxx.yuv width height\n" );
		return(-1);
	}

	/*
	 * 1.������Ҫ��һ֡һ֡�����ݣ�������ҪAVFrame�ṹ
	 * ������һ֡���ݱ�����AVFrame�С�
	 */
	frame		= av_frame_alloc();
	frame->width	= atoi( argv[2] );			//����֡���
	frame->height	= atoi( argv[3] );			//����֡�߶�
	fprintf( stdout, "width=%d,height=%d\n", frame->width, frame->height );
	frame->format = AV_PIX_FMT_YUV420P;			//����Դ�ļ������ݸ�ʽ
	av_image_alloc( frame->data, frame->linesize, frame->width, frame->height, (enum AVPixelFormat) frame->format, 32 );
	fileIn = fopen( argv[1], "r+" );

	/*
	 * 2.�����������ݱ�����AVPacket�У���ˣ����ǻ���ҪAVPacket�ṹ��
	 * ��ʼ��packet
	 */
	av_init_packet( &packet );


	/*
	 * 3.�����������ݣ�������Ҫ���룬�����Ҫ������
	 * ��ĺ����ҵ�h.264���͵ı�����
	 */
	/* find the mpeg1 video encoder */
	codec = avcodec_find_encoder( AV_CODEC_ID_H264 );
	if ( !codec )
	{
		fprintf( stderr, "Codec not found\n" );
		exit( 1 );
	}

	/*���˱����������ǻ���Ҫ�������������Ļ������������Ʊ���Ĺ��� */
	codecContext = avcodec_alloc_context3( codec ); /* ����AVCodecContextʵ�� */
	if ( !codecContext )
	{
		fprintf( stderr, "Could not allocate video codec context\n" );
		return(-1);
	}
	/* put sample parameters */
	codecContext->bit_rate = 400000;
	/* resolution must be a multiple of two */
	codecContext->width	= 300;
	codecContext->height	= 200;
	/* frames per second */
	codecContext->time_base = (AVRational) { 1, 25 };


	/* emit one intra frame every ten frames
	 * check frame pict_type before passing frame
	 * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	 * then gop_size is ignored and the output of encoder
	 * will always be I frame irrespective to gop_size
	 */
	codecContext->gop_size		= 10;
	codecContext->max_b_frames	= 1;
	codecContext->pix_fmt		= AV_PIX_FMT_YUV420P;
	av_opt_set( codecContext->priv_data, "preset", "slow", 0 );

	/* ׼�����˱������ͱ����������Ļ��������ڿ��Դ򿪱������� */
	if ( avcodec_open2( codecContext, codec, NULL ) < 0 ) /* ���ݱ����������Ĵ򿪱����� */
	{
		fprintf( stderr, "Could not open codec\n" );
		return(-1);
	}

	/* 4.׼������ļ� */
	fileOut = fopen( "test.h264", "w+" );
	/*
	YUV420 planar���ݣ� ��720��488��Сͼ��YUV420 planarΪ����

	��洢��ʽ�ǣ� ����СΪ(720��480��3>>1)�ֽڣ�
	��Ϊ��������:Y,U��V
		Y������    	(720��480)	  	���ֽ�  
		U(Cb)������(720��480>>2)		���ֽ�
		V(Cr)������(720��480>>2)		���ֽ�
	*/
	/*���濪ʼ���� */
	while ( 1 )
	{
		/* ��һ֡���ݳ��� */
		readSize = fread( frame->data[0], 1, frame->linesize[0] * frame->height, fileIn );
		if ( readSize == 0 )
		{
			fprintf( stdout, "end of file\n" );
			frameCount++;
			break;
		}
		readSize	= fread( frame->data[1], 1, frame->linesize[1] * frame->height / 2, fileIn );
		readSize	= fread( frame->data[2], 1, frame->linesize[2] * frame->height / 2, fileIn );
		/* ��ʼ��packet */
		av_init_packet( &packet );
		/* encode the image */
		frame->pts	= frameCount;
		ret		= avcodec_encode_video2( codecContext, &packet, frame, &getPacket ); /* ��AVFrame�е�������Ϣ����ΪAVPacket�е����� */
		if ( ret < 0 )
		{
			fprintf( stderr, "Error encoding frame\n" );
			return(-1);
		}

		if ( getPacket )
		{
			frameCount++;
			/* ���һ�������ı���֡ */
			printf( "Write frame %3d (size=%5d)\n", frameCount, packet.size );
			fwrite( packet.data, 1, packet.size, fileOut );
			av_packet_unref( &packet );
		}
	}

	/* flush buffer */
	for ( getPacket = 1; getPacket; frameCount++ )
	{
		fflush( stdout );
		frame->pts	= frameCount;
		ret		= avcodec_encode_video2( codecContext, &packet, NULL, &getPacket ); /* �����������ʣ������� */
		if ( ret < 0 )
		{
			fprintf( stderr, "Error encoding frame\n" );
			return(-1);
		}

		if ( getPacket )
		{
			printf( "Write frame %3d (size=%5d)\n", frameCount, packet.size );
			fwrite( packet.data, 1, packet.size, fileOut );
			av_packet_unref( &packet );
		}
	} /* for (got_output = 1; got_output; frameIdx++) */

	fclose( fileIn );
	fclose( fileOut );
	av_frame_free( &frame );
	avcodec_close( codecContext );
	av_free( codecContext );

	return(0);
}


