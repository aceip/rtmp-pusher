#ifndef _AAC_DECODER_H_
#define _AAC_DECODER_H_


/*
1. dir2/foo2.h (����λ��, ��������)
2. C ϵͳ�ļ�
3. C++ ϵͳ�ļ�
4. ������� .h �ļ�
5. ����Ŀ�� .h �ļ�

*/
#include "audio_decoder.h"
#include <fdk-aac/aacdecoder_lib.h>

namespace AudioCode
{
class AacDecoder : public AudioDecoder
{
public:
	AacDecoder();
	virtual ~AacDecoder();
	virtual int Init(const int withHead);
	virtual int Init(const int withHead, const unsigned char *header, int headerLength);
	virtual int Decode(const unsigned char *inputData, int inputDataLength, unsigned char *outputData, int &outputDataLength);
private:
	HANDLE_AACDECODER decoderHandle_;
	int withHead_;			// 0 ����ʱ���ݲ���adts header�� 1���������ݴ�adts header
};
}
#endif

