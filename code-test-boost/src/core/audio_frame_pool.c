#include "audio_frame_pool.h"


AudioFramePool::AudioFramePool(int round)
	: round_(round)
{
	
}
AudioFramePool::~AudioFramePool()
{
	// 销毁队列
	MutexLockGuard lock(mutex_);
	std::map<EAudioType, BoundedQueue *>::iterator
		mapItem = framePoolMap_.find(eAudioMp3);
	if(mapItem != framePoolMap_.end())
	{
		delete (mapItem->second);
		printf("~AudioFramePool delete eAudioMp3 framequeue\n");
	}
	
	mapItem = framePoolMap_.find(eAudioAac);
	if(mapItem != framePoolMap_.end())
	{
		delete (mapItem->second);
		printf("~AudioFramePool delete eAudioAac framequeue\n");
	}
}
int AudioFramePool::RegisterFramesPool(EAudioType audioType, int maxFrames)	// 注册帧池
{
	int ret = 0;
	MutexLockGuard lock(mutex_);
	// 先查找是否已经注册
	if (framePoolMap_.find(audioType) != framePoolMap_.end())  
	{
		// 说明已经注册过了
		return 1;
	}
	else
	{
		//形式1
		//FrameQueuePtr frameQueue(new BoundedQueue<Buffer::BufferPtr>(maxFrames)); 
		// 形式2
		BoundedQueue *frameQueue = new BoundedQueue(maxFrames, false); 	
		if(frameQueue != nullptr)
		{
			framePoolMap_.insert(std::make_pair(audioType, frameQueue));  
			ret = 0;
		}
		else
		{
			ret = -1;
		}
	}	
	return ret;	
}
int AudioFramePool::UnregisterFramesPool(EAudioType audioType)
{
	MutexLockGuard lock(mutex_);
	if(framePoolMap_.erase(audioType) == 1)		// 删除成功
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int AudioFramePool::PutFrame(EAudioType audioType, Buffer::BufferPtr &buf)
{
 	MutexLockGuard lock(mutex_);
	std::map<EAudioType, BoundedQueue *>::iterator
		mapItem = framePoolMap_.find(audioType);
	if(mapItem != framePoolMap_.end())
	{
		if(BoundedQueue::eOk == mapItem->second->put(round_, buf))
		{
			return 0;
		}
		else 
		{
			return 1;				
		}
	}
	else
	{
		return -1;		// 不存在
	}
}

int AudioFramePool::TakeFrame(EAudioType audioType, Buffer::BufferPtr &buf)
{
	MutexLockGuard lock(mutex_);
	std::map<EAudioType, BoundedQueue *>::iterator  
		mapItem = framePoolMap_.find(audioType);
	if(mapItem != framePoolMap_.end())
	{
		Buffer::BufferPtr tmp;
		//buf = mapItem->second->take();	
		if(BoundedQueue::eOk == mapItem->second->take(buf))
		{
			return 0;
		}
		else 
		{
			return 1;				
		}
	}
	else
	{
		return -1;		// 不存在
	}
}


