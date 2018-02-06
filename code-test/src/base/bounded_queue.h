#ifndef _BOUNDED_QUEUE_H_
#define _BOUNDED_QUEUE_H_

#include <vector>
#include <assert.h>
#include <memory>
#include "mutex.h"
#include "condition.h"
using namespace std;

template<typename T>
class BoundedQueue
{
public:
	typedef std::shared_ptr<BoundedQueue> QueuePtr;
	typedef enum {eOk = 0, eFull, eEmpty}ERetCode;
	typedef enum {eAudioMp3 = 0,  eAudioAac}EAudioType;
	explicit BoundedQueue(size_t maxSize, bool block = true)
		: mutex_(), 
		capacity_(maxSize), 
		notEmpty_(mutex_), 
		notFull_(mutex_),
		block_(block)
	{
		queue_.reserve(capacity_);
	}
	
	ERetCode put(const T& x)
	{
		{
			MutexLockGuard lock(mutex_);
			while(queue_.size() == capacity_)
			{
				if(!block_)
				{
					return eFull;
				}
				notFull_.wait();
			}
			assert(!(queue_.size() == capacity_));
			queue_.push_back(x);
		}
		
		notEmpty_.notify();
		return eOk;
	}

	// ���round������0,����������򸲸�, ���øú���������洢������shared_ptr�����ᵼ���ڴ�й©�����Ϊshared_ptr���
	// �Զ����ö�������������ͷ���Դ.
	ERetCode put(int round, const T& x)
	{
		{
			MutexLockGuard lock(mutex_);
			if(round == 0)
			{
				while(queue_.size() == capacity_)
				{
					if(!block_)
					{
						return eFull;
					}
					notFull_.wait();
				}
				assert(!(queue_.size() == capacity_));
			}
			else
			{
				if(queue_.size() == capacity_)
				{
					T front(queue_.front());
					queue_.erase(queue_.begin());		// ȡ��Ԫ��
				}
			}
			queue_.push_back(x);
		}
		
		notEmpty_.notify();
		return eOk;
	}
	
	ERetCode take(T& x)
	{
		MutexLockGuard lock(mutex_);
		while(queue_.empty())
		{
			if(!block_)
			{
				return eEmpty;
			}
			notEmpty_.wait();
		}
		assert(!queue_.empty());
		T front(queue_.front());
		queue_.erase(queue_.begin());
		x = front;
		notFull_.notify();
		
		return eOk;
	}
	
	
	bool empty() const
	{
		MutexLockGuard lock(mutex_);
		return queue_.empty();
	}
	
	bool full() const
	{
		MutexLockGuard lock(mutex_);
		return queue_.size() == capacity_;
	}
	
	size_t size() const
	{
		MutexLockGuard lock(mutex_);
		return queue_.size();
	}
	
	size_t capacity() const
	{
		MutexLockGuard lock(mutex_);
		return queue_.capacity();
	}
	
private:
	mutable MutexLock mutex_;
	size_t capacity_;
	bool block_;				// �Ƿ�����
	Condition notEmpty_;
	Condition notFull_;
	std::vector<T> queue_;
};

#endif

