/*
 * tutorial02.c
 * A pedagogical video player that will stream through every video frame as fast as it can.
 *
 * This tutorial was written by Stephen Dranger (dranger@gmail.com).
 *
 * Code based on FFplay, Copyright (c) 2003 Fabrice Bellard,
 * and a tutorial by Martin Bohme (boehme@inb.uni-luebeckREMOVETHIS.de)
 * Tested on Gentoo, CVS version 5/01/07 compiled with GCC 4.1.1
 *
 * Use the Makefile to build all examples.
 *
 * Run using
 * tutorial02 myvideofile.mpg
 *
 * to play the video stream on your screen.
 */


/* �����ס��ʹ��C++������ʱһ��Ҫʹ��extern "C"�������Ҳ��������ļ� */
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <sys/time.h>


#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

#ifdef __cplusplus
}
#endif


#include "packet_queue.h"

#define AVCODE_MAX_AUDIO_FRAME_SIZE	192000  /* 1 second of 48khz 32bit audio */
#define SDL_AUDIO_BUFFER_SIZE		1024    /*  */


#define ERR_STREAM	stderr
#define OUT_SAMPLE_RATE 44100

AVFrame wanted_frame;


PacketQueue audio_queue;      /* ��Ƶ���� */
void audio_callback( void *userdata, Uint8 *stream, int len );


int audio_decode_frame( AVCodecContext *pAudioCodecCtx, uint8_t *audio_buf, int buf_size );


/*
 * ע��userdata��ǰ���AVCodecContext.
 * len��ֵ��Ϊ2048,��ʾһ�η��Ͷ��١�
 * audio_buf_size��һֱΪ�����������Ĵ�С��wanted_spec.samples.��һ��ÿ�ν�����ô�࣬�ļ���ͬ�����ֵ��ͬ)
 * audio_buf_index�� ��Ƿ��͵������ˡ�
 * ��������Ĺ���ģʽ��:
 * 1. �������ݷŵ�audio_buf, ��С��audio_buf_size��(audio_buf_size = audio_size;������ã�
 * 2. ����һ��callbackֻ�ܷ���len���ֽ�,��ÿ��ȡ�صĽ������ݿ��ܱ�len��һ�η����ꡣ
 * 3. �������ʱ�򣬻�len == 0��������ѭ�����˳���������������callback��������һ�η��͡�
 * 4. �����ϴ�û���꣬��β�ȡ���ݣ����ϴε�ʣ��ģ�audio_buf_size��Ƿ��͵������ˡ�
 * 5. ע�⣬callbackÿ��һ��Ҫ���ҽ���len�����ݣ����򲻻��˳���
 * ���û��������������û���ˣ�����ȡ�������ˣ����˳���������һ�������Դ�ѭ����
 * ����������Ϊstatic����Ϊ�˱����ϴ����ݣ�Ҳ������ȫ�ֱ��������Ǹо��������á�
 */
void audio_callback( void *userdata, Uint8 *stream, int len )
{
	AVCodecContext	*pAudioCodecCtx = (AVCodecContext *) userdata;
	int		len1		= 0;
	int		audio_size	= 0;

	/*
	 * ע����static
	 * ΪʲôҪ����ô��
	 */
	static uint8_t		audio_buf[(AVCODE_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	static unsigned int	audio_buf_size	= 0;
	static unsigned int	audio_buf_index = 0;

	/* ��ʼ��stream��ÿ�ζ�Ҫ�� */
	SDL_memset( stream, 0, len );

	while ( len > 0 )
	{
		if ( audio_buf_index >= audio_buf_size )
		{
			/*
			 * ����ȫ�����ͣ���ȥ��ȡ
			 * �Զ����һ������
			 */
			audio_size = audio_decode_frame( pAudioCodecCtx, audio_buf, sizeof(audio_buf) );
			if ( audio_size < 0 )
			{
				/* �������� */
				audio_buf_size = 1024;
				memset( audio_buf, 0, audio_buf_size );
			}else  {
				audio_buf_size = audio_size;
			}
			audio_buf_index = 0;    /* �ص���������ͷ */
		}

		len1 = audio_buf_size - audio_buf_index;
/*          printf("len1 = %d\n", len1); */
		if ( len1 > len )               /* len1����len�󣬵���һ��callbackֻ�ܷ�len�� */
		{
			len1 = len;
		}

		/*
		 * �³����� SDL_MixAudioFormat()����
		 * �����Ƶ�� ��һ������dst�� �ڶ�����src��audio_buf_sizeÿ�ζ��ڱ仯
		 */
		SDL_MixAudio( stream, (uint8_t *) audio_buf + audio_buf_index, len, SDL_MIX_MAXVOLUME );
		/*
		 *
		 * memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
		 */
		len		-= len1;
		stream		+= len1;
		audio_buf_index += len1;
	}
}


/* ������Ƶ��˵��һ��packet���棬���ܺ��ж�֡(frame)���ݡ� */

int audio_decode_frame( AVCodecContext *pAudioCodecCtx,
			uint8_t *audio_buf, int buf_size )
{
	AVPacket	packet;
	AVFrame		*frame;
	int		got_frame;
	int		pkt_size = 0;
/*     uint8_t    *pkt_data = NULL; */
	int		decode_len;
	int		try_again	= 0;
	long long	audio_buf_index = 0;
	long long	data_size	= 0;
	SwrContext	*swr_ctx	= NULL;
	int		convert_len	= 0;
	int		convert_all	= 0;

	if ( packet_queue_get( &audio_queue, &packet, 1 ) < 0 )
	{
		fprintf( ERR_STREAM, "Get queue packet error\n" );
		return(-1);
	}

/*     pkt_data = packet.data; */
	pkt_size = packet.size;
	/* fprintf(ERR_STREAM, "pkt_size = %d\n", pkt_size); */

	frame = av_frame_alloc();
	while ( pkt_size > 0 )
	{
/*          memset(frame, 0, sizeof(AVFrame)); */
		/*
		 * pAudioCodecCtx:��������Ϣ
		 * frame:����������ݵ�frame
		 * got_frame:�����0������frameȡ�ˣ�����ζ�����˴���
		 * packet:���룬ȡ���ݽ��롣
		 */
		decode_len = avcodec_decode_audio4( pAudioCodecCtx,
						    frame, &got_frame, &packet );
		if ( decode_len < 0 ) /* ������� */
		{
			/* �ؽ�, �������һֱ<0�أ� */
			fprintf( ERR_STREAM, "Couldn't decode frame\n" );
			if ( try_again == 0 )
			{
				try_again++;
				continue;
			}
			try_again = 0;
		}


		if ( got_frame )
		{
			/*              //�ö�����Ƶ������ȡ������������С
			 *            data_size = av_samples_get_buffer_size(NULL,
			 *                    pAudioCodecCtx->channels, frame->nb_samples,
			 *                    pAudioCodecCtx->sample_fmt, 1);
			 *
			 *            assert(data_size <= buf_size);
			 * //               memcpy(audio_buf + audio_buf_index, frame->data[0], data_size);
			 */
			/*
			 * chnanels: ͨ������, ��������Ƶ
			 * channel_layout: ͨ�����֡�
			 * ����Ƶͨ��������һ��ͨ�����ֿ��Ծ����������������.ͨ����������������
			 * ���ָ���ǵ�����(mono)������������stereo), ������֮��İɣ�
			 * ���Դ�뼰��https://xdsnet.gitbooks.io/other-doc-cn-ffmpeg/content/ffmpeg-doc-cn-07.html#%E9%80%9A%E9%81%93%E5%B8%83%E5%B1%80
			 */


			if ( frame->channels > 0 && frame->channel_layout == 0 )
			{
				/* ��ȡĬ�ϲ��֣�Ĭ��Ӧ����stereo�ɣ� */
				frame->channel_layout = av_get_default_channel_layout( frame->channels );
			}else if ( frame->channels == 0 && frame->channel_layout > 0 )
			{
				frame->channels = av_get_channel_layout_nb_channels( frame->channel_layout );
			}


			if ( swr_ctx != NULL )
			{
				swr_free( &swr_ctx );
				swr_ctx = NULL;
			}

			/*
			 * ����common parameters
			 * 2,3,4��output������4,5,6��input������
			 * �ز���
			 */
			swr_ctx = swr_alloc_set_opts( NULL,
						      wanted_frame.channel_layout, (enum AVSampleFormat) (wanted_frame.format), wanted_frame.sample_rate,
						      frame->channel_layout, (enum AVSampleFormat) (frame->format), frame->sample_rate,
						      0, NULL );
			/* ��ʼ�� */
			if ( swr_ctx == NULL || swr_init( swr_ctx ) < 0 )
			{
				fprintf( ERR_STREAM, "swr_init error\n" );
				break;
			}
			/*
			 * av_rescale_rnd(): ��ָ���ķ�ʽ��64bit������������(rnd:rounding),
			 * ʹ��a*b/c֮��Ĳ������������
			 * swr_get_delay(): ���� �ڶ���������֮һ�������ǣ�1/frame->sample_rate)
			 * AVRouding��һ��enum��1����˼��round away from zero.
			 */


			/*
			 *        int dst_nb_samples = av_rescale_rnd(
			 *                swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
			 *                wanted_frame.sample_rate, wanted_frame.format,
			 *                AVRounding(1));
			 */

			/*
			 * ת����Ƶ����frame�е���Ƶת����ŵ�audio_buf�С�
			 * ��2��3����Ϊoutput�� ��4��5Ϊinput��
			 * ����ʹ��#define AVCODE_MAX_AUDIO_FRAME_SIZE 192000
			 * ��dst_nb_samples�滻��, ���Ĳ���Ƶ����192kHz.
			 */
			convert_len = swr_convert( swr_ctx,
						   &audio_buf + audio_buf_index,
						   AVCODE_MAX_AUDIO_FRAME_SIZE,
						   (const uint8_t * *) frame->data,
						   frame->nb_samples );

			/*
			 * printf("decode len = %d, convert_len = %d\n", decode_len, convert_len);
			 * �����˶��٣����뵽������
			 *          pkt_data += decode_len;
			 */
			pkt_size -= decode_len;
			/* ת�������Ч���ݴ浽�������audio_buf_index��� */
			audio_buf_index += convert_len; /* data_size; */
			/* ��������ת�������Ч���ݵĳ��� */
			convert_all += convert_len;
		}
	}
	return(wanted_frame.channels * convert_all * av_get_bytes_per_sample( (enum AVSampleFormat) (wanted_frame.format) ) );
/*     return audio_buf_index; */
}


uint64_t getNowTime()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}


/*
 * ���뷽ʽ
 * gcc -w -o 3-1-main 3-1-main.c packet_queue.c -lSDL2 -lavcodec -lswscale -lavutil     \
 * -lavformat -lswresample -I/usr/local/ffmpeg/include -L/usr/local/ffmpeg/lib
 */
int main( int argc, char *argv[] )
{
	AVFormatContext *pFormatCtx = NULL;
	int		i;
	int		audioStream;
	int		videoStream;

	/* ��Ƶ���� */
	AVCodecContext	*pVideoCodecCtx = NULL;         /* ��Ƶ������������ */
	AVCodec		*pVideoCodec	= NULL;         /* ��Ƶ������ */
	AVFrame		*pVideoFrame	= NULL;

	/* ��Ƶ���� */
	AVCodecContext	*pAudioCodecCtx		= NULL; /* ��Ƶ������������ */
	AVCodec		*pAudioCodec		= NULL; /* ��Ƶ������ */
	AVFrame		*pAudioFrame		= NULL;
	int		firstGotAudioFrame	= 0;
	int64_t		audioFirstPts;

	/* ���ŵ���ʼʱ�� */
	uint64_t playStartTime;

	AVPacket	packet;
	int		frameFinished; /* �Ƿ��ȡ֡ */

	AVDictionary		*optionsDict	= NULL;
	struct SwsContext	*sws_ctx	= NULL;


	/* ��Ƶ��� */
	SDL_Texture	*bmp	= NULL;
	SDL_Window	*screen = NULL;
	SDL_Rect	rect;


	/*
	 * ��Ƶ���
	 * SDL
	 */
	SDL_AudioSpec	wanted_spec;
	SDL_AudioSpec	spec;

	/* �����¼� */
	SDL_Event event;


	if ( argc < 2 )
	{
		fprintf( stderr, "Usage: test <file>\n" );
		exit( 1 );
	}
	/* Register all formats and codecs */
	av_register_all();

	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER ) )
	{
		fprintf( stderr, "Could not initialize SDL - %s\n", SDL_GetError() );
		exit( 1 );
	}

	/* Open video file */
	if ( avformat_open_input( &pFormatCtx, argv[1], NULL, NULL ) != 0 )
		return(-1);     /* Couldn't open file */

	/* Retrieve stream information */
	if ( avformat_find_stream_info( pFormatCtx, NULL ) < 0 )
		return(-1);     /* Couldn't find stream information */

	/* Dump information about file onto standard error */
	av_dump_format( pFormatCtx, 0, argv[1], 0 );

	/* ������Ƶ/��Ƶ�� */
	audioStream	= -1;
	videoStream	= -1;
	for ( i = 0; i < pFormatCtx->nb_streams; i++ )
	{
		if ( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
		{
			audioStream = i;
		}else if ( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
		{
			videoStream = i;
		}
	}


	if ( audioStream == -1 )
	{
		fprintf( stderr, "Can't find the audio stream\n" );
		return(-1);
	}
	if ( videoStream == -1 )
	{
		fprintf( stderr, "Can't find the video stream\n" );
		return(-1);
	}

	/*
	 * ������Ƶ
	 * ��ȡ��Ƶ������������
	 */
	pAudioCodecCtx = pFormatCtx->streams[audioStream]->codec;

	/* ͨ��������ID����һ�������� */
	pAudioCodec = avcodec_find_decoder( pAudioCodecCtx->codec_id );
	if ( pAudioCodec == NULL )
	{
		fprintf( stderr, "Unsupported audio codec!\n" );
		return(-1);                                     /* Codec not found */
	}

	if ( avcodec_open2( pAudioCodecCtx, pAudioCodec, NULL ) < 0 )
		return(-1);                                     /* Could not open codec */

	/* ������Ƶ��Ϣ, ��������Ƶ�豸�� */
	wanted_spec.freq	= pAudioCodecCtx->sample_rate;
	wanted_spec.format	= AUDIO_S16SYS;
	wanted_spec.channels	= pAudioCodecCtx->channels;     /* ͨ���� */
	wanted_spec.silence	= 0;                            /* ���þ���ֵ */
	wanted_spec.samples	= SDL_AUDIO_BUFFER_SIZE;        /* ��ȡ��һ֡�����? */
	wanted_spec.callback	= audio_callback;
	wanted_spec.userdata	= pAudioCodecCtx;

	/*
	 * wanted_spec:��Ҫ�򿪵�
	 * spec: ʵ�ʴ򿪵�,���Բ��������������ֱ����NULL�������õ�spec����wanted_spec���档
	 * ����Ὺһ���̣߳�����callback��
	 * SDL_OpenAudio->open_audio_device(���߳�)->SDL_RunAudio->fill(ָ��callback������
	 * ������SDL_OpenAudioDevice()����
	 */
	if ( SDL_OpenAudio( &wanted_spec, &spec ) < 0 )
	{
		fprintf( ERR_STREAM, "Couldn't open Audio:%s\n", SDL_GetError() );
		exit( -1 );
	}

	/* ���ò�����������ʱ����, swr_alloc_set_opts��in���ֲ��� */
	wanted_frame.format		= AV_SAMPLE_FMT_S16;
	wanted_frame.sample_rate	= spec.freq;
	wanted_frame.channel_layout	= av_get_default_channel_layout( spec.channels );
	wanted_frame.channels		= spec.channels;


	/*
	 * ������Ƶ
	 * ��ȡ��Ƶ������������
	 */
	pVideoCodecCtx = pFormatCtx->streams[videoStream]->codec;

	/* ͨ��������ID����һ�������� */
	pVideoCodec = avcodec_find_decoder( pVideoCodecCtx->codec_id );
	if ( pVideoCodec == NULL )
	{
		fprintf( stderr, "Unsupported video codec!\n" );
		return(-1); /* Codec not found */
	}


	/* ��ʼ����Ƶ���� */
	packet_queue_init( &audio_queue );

	/*
	 * ������SDL_PauseAudioDevice()����,Ŀǰ�õ����Ӧ���Ǿɵġ�
	 * 0�ǲ���ͣ����������ͣ
	 * ���û������ͷŲ�������
	 */
	SDL_PauseAudio( 0 );


	/* �򿪽����� */
	if ( avcodec_open2( pVideoCodecCtx, pVideoCodec, &optionsDict ) < 0 )
		return(-1);  /* Could not open codec */

	/* Allocate video frame */
	pVideoFrame = av_frame_alloc();

	AVFrame* pFrameYUV = av_frame_alloc();
	if ( pFrameYUV == NULL )
		return(-1);


	printf( "width = %d, height = %d\n", pVideoCodecCtx->width, pVideoCodecCtx->height );
	screen = SDL_CreateWindow( "My Game Window",
				   SDL_WINDOWPOS_UNDEFINED,
				   SDL_WINDOWPOS_UNDEFINED,
				   800, 480,
				   SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL );


	if ( !screen )
	{
		fprintf( stderr, "SDL: could not set video mode - exiting\n" );
		exit( 1 );
	}
	SDL_Renderer		*renderer; /* = SDL_CreateRenderer(screen, -1, 0); */
	SDL_RendererInfo	info;
	renderer = SDL_CreateRenderer( screen, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
	if ( !renderer )
	{
		av_log( NULL, AV_LOG_WARNING, "Failed to initialize a hardware accelerated renderer: %s\n", SDL_GetError() );
		renderer = SDL_CreateRenderer( screen, -1, 0 );
	}
	if ( renderer )
	{
		if ( !SDL_GetRendererInfo( renderer, &info ) )
			av_log( NULL, AV_LOG_VERBOSE, "Initialized %s renderer.\n", info.name );
	}
	/* Allocate a place to put our YUV image on that screen */
	bmp = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, pVideoCodecCtx->width, pVideoCodecCtx->height );
	/* SDL_SetTextureBlendMode(bmp,SDL_BLENDMODE_BLEND ); */

	sws_ctx = sws_getContext
		  (
		pVideoCodecCtx->width,
		pVideoCodecCtx->height,
		pVideoCodecCtx->pix_fmt,
		pVideoCodecCtx->width,
		pVideoCodecCtx->height,
		AV_PIX_FMT_YUV420P,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
		  );

	int numBytes = avpicture_get_size( AV_PIX_FMT_YUV420P, pVideoCodecCtx->width,
					   pVideoCodecCtx->height );
	uint8_t* buffer = (uint8_t *) av_malloc( numBytes * sizeof(uint8_t) );

	avpicture_fill( (AVPicture *) pFrameYUV, buffer, AV_PIX_FMT_YUV420P,
			pVideoCodecCtx->width, pVideoCodecCtx->height );

	/* Read frames and save first five frames to disk */
	i = 0;

	rect.x	= 0;
	rect.y	= 0;
	rect.w	= pVideoCodecCtx->width;
	rect.h	= pVideoCodecCtx->height;

	playStartTime = getNowTime();
	/* ʹ����ƵPTSȥ���ƶ�ȡ�ٶȣ���Ƶ����5֡�ٲ��ţ���Ƶ��������ֱ�Ӳ��� */
	while ( av_read_frame( pFormatCtx, &packet ) >= 0 )
	{
		if ( packet.stream_index == videoStream ) /* ��һ����Ƶ֡ */
		{
			/* Decode video frame */
			avcodec_decode_video2( pVideoCodecCtx, pVideoFrame, &frameFinished,
					       &packet );

			/* Did we get a video frame? */
			if ( frameFinished )
			{
				sws_scale
				(
					sws_ctx,
					(uint8_t const * const *) pVideoFrame->data,
					pVideoFrame->linesize,
					0,
					pVideoCodecCtx->height,
					pFrameYUV->data,
					pFrameYUV->linesize
				);
				/*
				 * ��Ƶֱ֡����ʾ
				 * //iPitch ����yuvһ������ռ���ֽ���
				 */
				SDL_UpdateTexture( bmp, &rect, pFrameYUV->data[0], pFrameYUV->linesize[0] );
				SDL_RenderClear( renderer );
				SDL_RenderCopy( renderer, bmp, &rect, &rect );
				SDL_RenderPresent( renderer );
			}
			/* Free the packet that was allocated by av_read_frame */
			av_free_packet( &packet );
			/* SDL_Delay(15); */
		}else if ( packet.stream_index == audioStream )         /* ��һ����Ƶ֡ */
		{
			packet_queue_put( &audio_queue, &packet );      /* ���Ϊ��Ƶ֡��������� */
			if ( 0 == firstGotAudioFrame )
			{
				firstGotAudioFrame	= 1;
				audioFirstPts		= packet.pts;
			}
			if ( 1 == firstGotAudioFrame )                  /* ͨ��ptsȥ����������ȡ���� */
			{
				while ( (packet.pts - audioFirstPts) > (getNowTime() - playStartTime) + 100 )
				{
					printf( " (packet.pts - audioFirstPts) = %u,  (getNowTime() - playStartTime) = %u\n",
						(packet.pts - audioFirstPts), (getNowTime() - playStartTime) );
					SDL_Delay( 30 );
				}
			}
		}else  {
			av_free_packet( &packet );
		}


		SDL_PollEvent( &event );
		switch ( event.type )
		{
		case SDL_QUIT:
			SDL_Quit();
			exit( 0 );
			break;
		default:
			break;
		}
	}

	SDL_DestroyTexture( bmp );

	/* Free the YUV frame */
	av_free( pVideoFrame );
	av_free( pFrameYUV );
	/* Close the codec */
	avcodec_close( pVideoCodecCtx );

	/* Close the video file */
	avformat_close_input( &pFormatCtx );

	return(0);
}


