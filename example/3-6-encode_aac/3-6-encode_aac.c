#ifdef __cplusplus
extern "C"
{
#endif
#include <math.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>

#ifdef __cplusplus
}
#endif

void rearrangePcm(uint8_t *leftData, uint8_t *rightData, uint8_t *inputData, int sampleNum)
{
	/** 
	*	������д���ļ�ʱ������һЩ����������ԭ��ġ� 
	*	�������˼�ǣ�LRLRLR...�ķ�ʽд���ļ���ÿ��д��4096���ֽ� 
	*/	
	for(int i = 0; i < sampleNum; i++)
	{
		(*(uint32_t *)(leftData + i *4)) = (*(uint32_t *)(inputData + i * 2*4));
		(*(uint32_t *)(rightData + i *4)) = (*(uint32_t *)(inputData + i * 2* 4 + 4));
	}

}

int main(int argc, char **argv){
    AVFrame *frame;
    AVCodec *codec = NULL;
    AVPacket packet;
    AVCodecContext *codecContext;
    int readSize=0;
    int ret=0,getPacket;
    FILE * fileIn,*fileOut;
    int frameCount=0;
    /* register all the codecs */
    av_register_all();
	char *padts = (char *)malloc(sizeof(char) * 7); 
	 int profile = 2;                                            //AAC LC  
    int freqIdx = 4;                                            //44.1KHz  
    int chanCfg = 2;            //MPEG-4 Audio Channel Configuration. 1 Channel front-center  
    padts[0] = (char)0xFF;      // 11111111     = syncword  
    padts[1] = (char)0xF1;      // 1111 1 00 1  = syncword MPEG-2 Layer CRC  
    padts[2] = (char)(((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));  
    padts[6] = (char)0xFC;  
	
    if(argc!=3){
        fprintf(stdout,"usage:./a.out xxx.pcm xxx.aac\n");
        return -1;
    }

    //1.������Ҫ��һ֡һ֡�����ݣ�������ҪAVFrame�ṹ
    //������һ֡���ݱ�����AVFrame�С�
    frame  = av_frame_alloc();
    // AV_SAMPLE_FMT_FLTP ��Ƶ����format�ֺܶ������ͣ�16bit,32bit�ȣ���2016 ffmpegֻ֧�����µ�AAC��ʽ��32bit��Ҳ����AV_SAMPLE_FMT_FLTP��
	//���ԣ����PCM���б������ȷ��PCM��AV_SAMPLE_FMT_FLTP���͵ġ�
    frame->format = AV_SAMPLE_FMT_FLTP;	
    frame->nb_samples = 1024;
    frame->sample_rate = 44100;
    frame->channels = 2;
    fileIn =fopen(argv[1],"r+");
    frame->data[0] = av_malloc(1024*4);
	frame->data[1] = av_malloc(1024*4);
	uint8_t pcmBuff[1024*4*2];
    //2.�����������ݱ�����AVPacket�У���ˣ����ǻ���ҪAVPacket�ṹ��
    //��ʼ��packet
    memset(&packet, 0, sizeof(AVPacket));  
    av_init_packet(&packet);


    //3.�����������ݣ�������Ҫ���룬�����Ҫ������
    //����ĺ����ҵ�h.264���͵ı�����
    /* find the mpeg1 video encoder */
    codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec){
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }

    //���˱����������ǻ���Ҫ�������������Ļ������������Ʊ���Ĺ���
    codecContext = avcodec_alloc_context3(codec);//����AVCodecContextʵ��
    if (!codecContext){
        fprintf(stderr, "Could not allocate video codec context\n");
        return -1;
    }
    /* put sample parameters */  
    codecContext->sample_rate = frame->sample_rate;  
    codecContext->channels = frame->channels;  
    codecContext->sample_fmt = frame->format;  
    /* select other audio parameters supported by the encoder */  
    codecContext->channel_layout = AV_CH_LAYOUT_STEREO;
    //׼�����˱������ͱ����������Ļ��������ڿ��Դ򿪱�������
    //���ݱ����������Ĵ򿪱�����
 
    if (avcodec_open2(codecContext, codec, NULL) < 0){
        fprintf(stderr, "Could not open codec\n");
        return -1;
    }
    //4.׼������ļ�
    fileOut= fopen(argv[2],"w+");
    //���濪ʼ����
    printf("L:%d \n", __LINE__);
    while(1)
    {
    	//printf("L:%d \n", __LINE__);
    	// ��һ֡�����ݶ�ȡ��������LRLR..�ĸ�ʽ����Ϊ LL .... RR...
        //��һ֡���ݳ���
        readSize = fread(pcmBuff,1,1024*2*4,fileIn);
        
        if(readSize == 0){
            fprintf(stdout,"end of file\n");
            frameCount++;
            break;
        }
        rearrangePcm(frame->data[0], frame->data[1], pcmBuff, readSize/2/4);
        //frame->linesize[0] = 4096;
        //frame->linesize[1] = 4096;
        //printf("L:%d readSize = %d\n", __LINE__, readSize);
        //��ʼ��packet
        av_init_packet(&packet);
        /* encode the image */
        frame->pts = frameCount;
        //printf("L:%d \n", __LINE__);
        ret = avcodec_encode_audio2(codecContext, &packet, frame, &getPacket); //��AVFrame�е�������Ϣ����ΪAVPacket�е�����
        //printf("L:%d \n", __LINE__);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            goto out;
        }

        if (getPacket) {
            frameCount++;
            //���һ�������ı���֡
            padts[3] = (char)(((chanCfg & 3) << 6) + ((7 + packet.size) >> 11));  
            padts[4] = (char)(((7 + packet.size) & 0x7FF) >> 3);  
            padts[5] = (char)((((7 + packet.size) & 7) << 5) + 0x1F);  
            fwrite(padts, 7, 1, fileOut); 
            
            //printf("Write frame %3d (size=%5d), %x %x %x\n", frameCount, packet.size, packet.data[0], packet.data[1], packet.data[2]);
            fwrite(packet.data, 1,packet.size, fileOut);
            av_packet_unref(&packet);
        }

    }
	
    /* flush buffer */
    for ( getPacket= 1; getPacket; frameCount++){
        frame->pts = frameCount;
        ret = avcodec_encode_audio2(codecContext, &packet, NULL, &getPacket);       //�����������ʣ�������
        if (ret < 0){
            fprintf(stderr, "Error encoding frame\n");
            goto out;
        }
        if (getPacket){
        //���һ�������ı���֡
            padts[3] = (char)(((chanCfg & 3) << 6) + ((7 + packet.size) >> 11));  
            padts[4] = (char)(((7 + packet.size) & 0x7FF) >> 3);  
            padts[5] = (char)((((7 + packet.size) & 7) << 5) + 0x1F);  
            fwrite(padts, 7, 1, fileOut); 
            printf("flush buffer Write frame %3d (size=%5d)\n", frameCount, packet.size);
            fwrite(packet.data, 1, packet.size, fileOut);
            av_packet_unref(&packet);
        }
    } //for (got_output = 1; got_output; frameIdx++) 
	//printf("L:%d \n", __LINE__);
out:
	//printf("L:%d \n", __LINE__);
    fclose(fileIn);
    fclose(fileOut);
    av_frame_free(&frame);
    avcodec_close(codecContext);
    av_free(codecContext);
    av_free(padts);
    return 0;
}

