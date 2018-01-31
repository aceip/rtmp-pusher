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
 * to play the video stream on your pWindow.
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

#include "sdl_display.h"
#include "sdl_audio_out.h"


#ifdef __cplusplus
}
#endif





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

	// ��Ƶ���� 
	AVCodecContext	*pVideoCodecCtx = NULL;         // ��Ƶ������������ 
	AVCodec		*pVideoCodec	= NULL;         	// ��Ƶ������ 
	AVFrame		*pVideoFrame	= NULL;

	// ��Ƶ����
	AVCodecContext	*pAudioCodecCtx		= NULL; 	// ��Ƶ������������
	AVCodec		*pAudioCodec		= NULL; 		// ��Ƶ������ 
	AVFrame		*pAudioFrame		= NULL;
	int			firstGotAudioFrame	= 0;
	int64_t		audioFirstPts;
	int 		audioBufferFrames  = 0;

	// ���ŵ���ʼʱ��
	uint64_t 	playStartTime;

	AVPacket	packet;
	int			frameFinished; // �Ƿ��ȡ֡ 

	AVDictionary *optionsDict	= NULL;
	

	// �����¼�
	SDL_Event event;


	if ( argc < 2 )
	{
		fprintf( stderr, "Usage: test <file>\n" );
		exit( 1 );
	}
	
	// ע�Ḵ����,��������
	av_register_all();

	// ��ʼ��SDLģ��
	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER ) )
	{
		fprintf( stderr, "Could not initialize SDL - %s\n", SDL_GetError() );
		exit( 1 );
	}

	// �򿪶�ý���ļ�
	if ( avformat_open_input( &pFormatCtx, argv[1], NULL, NULL ) != 0 )
	{
		fprintf( stderr, "avformat_open_input failed\n" );
		return(-1);     
	}
	// ��ȡ��ý������Ϣ
	if ( avformat_find_stream_info( pFormatCtx, NULL ) < 0 )
	{
		fprintf( stderr, "avformat_find_stream_info failed\n" );
		return(-1);     
	}
	
	// ��ӡ�й�����������ʽ����ϸ��Ϣ, �ú�����Ҫ����debug
	av_dump_format( pFormatCtx, 0, argv[1], 0 );

	// ������Ƶ/��Ƶ�� 
	audioStream	= -1;
	videoStream	= -1;
	for ( i = 0; i < pFormatCtx->nb_streams; i++ )
	{
		if ( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO )
		{
			audioStream = i;
		}
		else if ( pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
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

	// ----------------------������Ƶ ----------------------------------
	// ��ȡ��Ƶ������������
	pAudioCodecCtx = pFormatCtx->streams[audioStream]->codec;

	// ͨ��������ID����һ��������
	pAudioCodec = avcodec_find_decoder( pAudioCodecCtx->codec_id );
	if ( pAudioCodec == NULL )
	{
		fprintf( stderr, "Unsupported audio codec!\n" );
		return(-1);                                    
	}
	// ʹ�� AVCodec��ʼ��AVCodecContext 
	if ( avcodec_open2( pAudioCodecCtx, pAudioCodec, NULL ) < 0 )
	{
		fprintf( stderr, "avcodec_open2 audio  failed!\n" );
		return(-1);                                    
	}

	// ----------------------������Ƶ ----------------------------------
	// ��ȡ��Ƶ������������
	pVideoCodecCtx = pFormatCtx->streams[videoStream]->codec;

	// ͨ��������ID����һ�������� 
	pVideoCodec = avcodec_find_decoder( pVideoCodecCtx->codec_id );
	if ( pVideoCodec == NULL )
	{
		fprintf( stderr, "Unsupported video codec!\n" );
		return(-1);
	}

	// ʹ�� AVCodec��ʼ��AVCodecContext 
	if ( avcodec_open2( pVideoCodecCtx, pVideoCodec, &optionsDict ) < 0 )
	{
		fprintf( stderr, "avcodec_open2 video  failed!\n" );
		return(-1);  
	}
	// ������Ƶ֡
	pVideoFrame = av_frame_alloc();
	if(!pVideoFrame)
	{
		fprintf( stderr, "av_frame_alloc pVideoFrame  failed!\n" );
		return(-1);
	}
	
	//��ʼ��audio ���
	SDL2AudioOutInit(pAudioCodecCtx);
	printf( "width = %d, height = %d\n", pVideoCodecCtx->width, pVideoCodecCtx->height );
	SDL2DisplayInit(pVideoCodecCtx->width, pVideoCodecCtx->height, pVideoCodecCtx->pix_fmt);

	

	playStartTime = getNowTime();
	/* ʹ����ƵPTSȥ���ƶ�ȡ�ٶȣ���Ƶ����5֡�ٲ��ţ���Ƶ��������ֱ�Ӳ��� */
	while ( av_read_frame( pFormatCtx, &packet ) >= 0 )
	{
		if ( packet.stream_index == videoStream ) /* ��һ����Ƶ֡ */
		{
			// ����Ƶ֡
			avcodec_decode_video2( pVideoCodecCtx, pVideoFrame, &frameFinished, &packet );
			// ȷ���Ѿ���ȡ����Ƶ֡
			if ( frameFinished )
			{
				SDL2Display(pVideoFrame, pVideoCodecCtx->height);
			}
			// �ͷ�packetռ�õ��ڴ� 
			av_free_packet( &packet );
		}
		else if ( packet.stream_index == audioStream )         /* ��һ����Ƶ֡ */
		{
			/* ���Ϊ��Ƶ֡��������� */
			SDL2AudioPushPacket( &packet);
			if ( 1 == firstGotAudioFrame )                  /* ͨ��ptsȥ����������ȡ���� */
			{
				while ( (packet.pts - audioFirstPts) > (getNowTime() - playStartTime) + 500 )
				{
					/*
					printf( " (packet.pts - audioFirstPts) = %u,  (getNowTime() - playStartTime) = %u,dif = %ld\n",
					 	(packet.pts - audioFirstPts), (getNowTime() - playStartTime),
					 	(packet.pts - audioFirstPts) - (getNowTime() - playStartTime) );
					 	*/
					SDL_Delay( 30 );
				}
			}
			//��audio packet�����
			if ( 0 == firstGotAudioFrame )
			{
				firstGotAudioFrame	= 1;
				audioFirstPts		= packet.pts;
									
			}
			if(audioBufferFrames < 10)
			{
				if(audioBufferFrames++ >8)
				{
					audioBufferFrames = 10;				//����һЩ֡�ٿ�����Ƶ
					SDL2AudioOutPause(0);	
				}
			}
		}
		else  
		{
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

	

	/* Free the YUV frame */
	av_free( pVideoFrame );
	
	/* Close the codec */
	avcodec_close( pVideoCodecCtx );

	/* Close the video file */
	avformat_close_input( &pFormatCtx );
	SDL2DisplayDestory();
	return(0);
}


