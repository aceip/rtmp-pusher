#ifndef _AUDIO_FRAME_POOL_
#define _AUDIO_FRAME_POOL_

#include "bounded_blocking_queue.h"
#include "mutex.h"

#include "shared_buffer.h"
#include <map>

// ��Ƶ֡�����, MP3֡����AAC֡���Ȼ�����AudioFramePool��Ȼ��ַ��������ն�.

class AudioFramePool
{
public:
	typedef  std::shared_ptr<AudioFramePool> AudioFramePoolPtr;
	typedef  std::shared_ptr<BoundedBlockingQueue<Buffer::BufferPtr>> FrameQueuePtr;
	typedef enum {eAudioMp3 = 0,  eAudioAac}EAudioType;
	explicit AudioFramePool(int round);
	~AudioFramePool();
	int RegisterFramesPool(EAudioType audioType, int maxFrames);			// ע��֡��
	int UnregisterFramesPool(EAudioType audioType);
	int PutFrame(EAudioType audioType, Buffer::BufferPtr &buf);
	int TakeFrame(EAudioType audioType, Buffer::BufferPtr &buf);
private:
	int round_ = 1;				// round_ �Ƿ�ѭ������
	typedef  std::map<EAudioType, std::shared_ptr<BoundedBlockingQueue<Buffer::BufferPtr>>> FramePoolMap;

	FramePoolMap framePoolMap_;
	mutable MutexLock mutex_;
};

#endif
