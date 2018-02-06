#ifndef _BOUNDED_BLOCKING_QUEUE_H_
#define _BOUNDED_BLOCKING_QUEUE_H_

#include <vector>
#include <assert.h>
#include <memory>
#include "mutex.h"
#include "condition.h"
using namespace std;

template<typename T>
class BoundedBlockingQueue
{
public:
	typedef std::shared_ptr<BoundedBlockingQueue> QueuePtr;
	explicit BoundedBlockingQueue(size_t maxSize):m_mutex(), m_capacity(maxSize), m_notEmpty(m_mutex), m_notFull(m_mutex)
	{
		m_queue.reserve(m_capacity);
	}
	
	int put(const T& x)
	{
		{
			MutexLockGuard lock(m_mutex);
			while(m_queue.size() == m_capacity)
			{
				m_notFull.wait();
			}
			assert(!(m_queue.size() == m_capacity));
			m_queue.push_back(x);
		}
		
		m_notEmpty.notify();
		return 0;
	}

	// ���round������0,����������򸲸�, ���øú���������洢������shared_ptr�����ᵼ���ڴ�й©�����Ϊshared_ptr���
	// �Զ����ö�������������ͷ���Դ.
	int put(int round, const T& x)
	{
		{
			MutexLockGuard lock(m_mutex);
			if(round == 0)
			{
				while(m_queue.size() == m_capacity)
				{
					m_notFull.wait();
				}
				assert(!(m_queue.size() == m_capacity));
			}
			else
			{
				if(m_queue.size() == m_capacity)
				{
					T front(m_queue.front());
					m_queue.erase(m_queue.begin());		// ȡ��Ԫ��
				}
			}
			m_queue.push_back(x);
		}
		
		m_notEmpty.notify();
		return 0;
	}
	
	T take()
	{
		MutexLockGuard lock(m_mutex);
		while(m_queue.empty())
		{
			m_notEmpty.wait();
		}
		assert(!m_queue.empty());
		T front(m_queue.front());
		m_queue.erase(m_queue.begin());
		
		m_notFull.notify();
		return front;
	}
	
	bool empty() const
	{
		MutexLockGuard lock(m_mutex);
		return m_queue.empty();
	}
	
	bool full() const
	{
		MutexLockGuard lock(m_mutex);
		return m_queue.size() == m_capacity;
	}
	
	size_t size() const
	{
		MutexLockGuard lock(m_mutex);
		return m_queue.size();
	}
	
	size_t capacity() const
	{
		MutexLockGuard lock(m_mutex);
		return m_queue.capacity();
	}
	
private:
	mutable MutexLock m_mutex;
	size_t m_capacity;
	Condition m_notEmpty;
	Condition m_notFull;
	std::vector<T> m_queue;
};

#endif

