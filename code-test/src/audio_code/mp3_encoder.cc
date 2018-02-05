#include "mp3_encoder.h"


namespace AudioCode
{

Mp3Encoder::Mp3Encoder()
{
	
}

Mp3Encoder::~Mp3Encoder()
{
	lame_close(mp3Handle_);
}

int Mp3Encoder::Init(const unsigned char *headData, const int dataLength)
{
	mp3Handle_ = lame_init ();
	
	lame_set_in_samplerate (mp3Handle_, 44100);
	lame_set_VBR (mp3Handle_, vbr_off);
	lame_init_params (mp3Handle_);
	return 0;
}

int Mp3Encoder::Init(const int sampleRate, const unsigned char channels, const int bitRate)
{
/*
���ñ����ʿ���ģʽ��Ĭ����CBR������ͨ�����Ƕ�������VBR��
������ö�٣�vbr_off����CBR��vbr_abr����ABR����ΪABR�����������Ա��Ĳ���ABR�����⣩vbr_mtrh����VBR��

*/
	mp3Handle_ = lame_init ();
	
	lame_set_in_samplerate (mp3Handle_, sampleRate);		// ��������mp3��������������Ĳ����ʣ����������������������һ����
	
	lame_set_num_channels(mp3Handle_,channels);//Ĭ��Ϊ2˫ͨ��
 	lame_set_mode(mp3Handle_, STEREO);			// ������
	lame_set_quality(mp3Handle_,5); /* 2=high 5 = medium 7=low ����*/
	lame_set_VBR(mp3Handle_, vbr_off);
	lame_set_brate(mp3Handle_,bitRate/1000);// ��khzΪ��λ, ����CBR�ı����ʣ�ֻ����CBRģʽ�²���Ч��
											//lame_set_VBR_mean_bitrate_kbps������VBR�ı����ʣ�ֻ����VBRģʽ�²���Ч��
	lame_init_params (mp3Handle_);

	return 0;
}

// ��ʽLRLRLR...������LLL...RRR...
int Mp3Encoder::Encode(const unsigned char *inputData, int inputDataLength, 
							unsigned char *outputData, int &outputDataLength)
{
	int size;
	int bufSize = outputDataLength;

	size = lame_encode_buffer_interleaved (mp3Handle_, (short int *)inputData, MP3_FRAME_SAMPLES, outputData, bufSize);
	outputDataLength = size;

	if(size > 0)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

}// end of namespace AudioCodee
