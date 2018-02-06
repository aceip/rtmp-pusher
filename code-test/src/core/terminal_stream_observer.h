#ifndef _TERMINAL_STREAM_OBSERVER_H_
#define _TERMINAL_STREAM_OBSERVER_H_
#include <memory>
#include "thread.h"
#include "audio_frame_pool.h"
#include "terminal.h"
#include "mutex.h"
using namespace darren;
using namespace std;

// ����������
class TerminalStreamObserver// : public Thread
{
public:
	TerminalStreamObserver(AudioFramePool::AudioFramePoolPtr &audioFramePool);
	virtual ~TerminalStreamObserver();
	int RegisterTerminal(int id, std::shared_ptr<Terminal> &terminal);			// ע��֡�� �۲���ģʽ
	int UnregisterTerminal(int id);
	void NotifyAll(Terminal::ETerminalType terminalType);
	void Loop();
	int startLoop();

private:
	AudioFramePool::AudioFramePoolPtr audioFramePool_; 
	
	std::map<int, std::shared_ptr<Terminal>> terminalMap_;		// ͨ��IDȥ��ΪΨһ��ʶ
	mutable MutexLock mutex_;
	Buffer::BufferPtr frameBuf_;
	Thread m_thread;
	bool m_exiting;
};
#endif
