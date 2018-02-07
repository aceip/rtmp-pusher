#include "terminal_stream_observer.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>

TerminalStreamObserver::TerminalStreamObserver(AudioFramePool::AudioFramePoolPtr audioFramePool)
	:m_thread(boost::bind(&TerminalStreamObserver::Loop, this)),
	audioFramePool_(audioFramePool),
	m_exiting(false)
{

}
TerminalStreamObserver::~TerminalStreamObserver()
{
	m_exiting = true;
  	m_thread.join();
}
int TerminalStreamObserver::RegisterTerminal(int id, Terminal *terminal)		// ע��֡�� �۲���ģʽ
{
	int ret = 0;
	printf("RegisterTerminal terminal = 0x%x, size = %d\n", terminal, terminalMap_.size());
	MutexLockGuard lock(mutex_);
	// �Ȳ����Ƿ��Ѿ�ע��
	if (terminalMap_.find(id) != terminalMap_.end())  
	{
		// ˵���Ѿ�ע�����
		printf("RegisterTerminal have register1 \n");
		return 1;
	}
	else
	{
		if(terminal != nullptr)
		{
			terminalMap_.insert(std::make_pair(id, terminal));  
			ret = 0;
		}
		else
		{
			printf("RegisterTerminal have register2 \n");
			ret = -1;
		}
	}	
	printf("RegisterTerminal ret = %d\n", ret);
	return ret;	

}
int TerminalStreamObserver::UnregisterTerminal(int id)
{
	MutexLockGuard lock(mutex_);
	if(terminalMap_.erase(id) == 1)		// ɾ���ɹ�
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

void TerminalStreamObserver::NotifyAll(Terminal::ETerminalType terminalType)
{
	MutexLockGuard lock(mutex_);

	// �ն˵����Լ��ĺ�����������
	 //printf("NotifyAll1 \n");
	// ÿ���ն˱����Լ������ݣ���ʹ���̳߳�����������
	for (std::map<int, Terminal *>::iterator mapItem = terminalMap_.begin(); mapItem != terminalMap_.end(); ++mapItem) 
	{  
		Terminal::ETerminalType type  = mapItem->second->GetTerminalType();
		//printf("NotifyAll2 terminalType = %d \n", type);
		if(terminalType == type)
        {
        	//printf("NotifyAll2 \n");
        	 mapItem->second->Send(frameBuf_);
        }	
    }  
}

void TerminalStreamObserver::Loop()
{
	int ret;
	while(!m_exiting)
	{
		ret = audioFramePool_->TakeFrame(AudioFramePool::eAudioMp3, frameBuf_);
		if(ret == 0)
		{
			NotifyAll(Terminal::eTerminalMp3);
			frameBuf_.reset();
		}
		else
		{
			 //printf("ret1 = %d \n", ret);
		}
		ret = audioFramePool_->TakeFrame(AudioFramePool::eAudioAac, frameBuf_);
		if(ret == 0)
		{
			NotifyAll(Terminal::eTerminalAac);
			frameBuf_.reset();
		}
		else
		{
			// printf("ret2 = %d \n", ret);
		}
		usleep(22000);
	}
}

int TerminalStreamObserver::startLoop()
{
  	assert(!m_thread.started());
  	m_thread.start();
}



