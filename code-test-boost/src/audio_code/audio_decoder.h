//����ͷ�ļ���Ӧ��ʹ�� #define ��ֹͷ�ļ������ذ���, ������ʽ����: <PROJECT>_<PATH>_<FILE>_H_
//����ǰ�������ĵط�������ʹ�� #include.
#ifndef _AUDIO_DECODER_
#define _AUDIO_DECODER_

#define nullptr NULL

namespace AudioCode
{
class AudioDecoder
{
public:
	AudioDecoder() {}
	virtual ~AudioDecoder() {}
	virtual int Init(const int withHead) = 0;
	virtual int Init(const int withHead, const unsigned char *header, int headerLength)  = 0;
	virtual int Decode(const unsigned char *inputData, int inputDataLength, unsigned char *outputData, int &outputDataLength) = 0;
	
	virtual unsigned char GetChannels() const 
	{
		return channels_;
	}
	
	virtual unsigned int GetSampleRate() const 
	{
		return sampleRate_;
	}
	
	virtual unsigned char GetSampleBit() const 
	{
		return sampleBit_;
	}
	virtual unsigned char GetBitRate() const 
	{
		return bitRate_;
	}
		
protected:
	unsigned int sampleRate_;
	unsigned int bitRate_;
	unsigned char channels_;
	unsigned char sampleBit_;
};
}

#endif
