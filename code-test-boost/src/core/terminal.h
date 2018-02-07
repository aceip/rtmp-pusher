#ifndef _TERMINAL_H_
#define _TERMINAL_H_
#include "shared_buffer.h"
#include "audio_frame_pool.h"
const int BUFFER_LENGTH = 2048*10;

using namespace std;
class Terminal
{
public:
	typedef  std::shared_ptr<Terminal> TerminalPtr;
	typedef enum {eTerminalUnknown, eTerminalMp3, eTerminalAac,} ETerminalType;
	explicit Terminal(int id, ETerminalType terminalType);		// ����һ��ʼ��ָ��ID���ն�����
	virtual ~Terminal();
	void SetSocketHandle(int socketHandle_);					// ����socket���
	int GetSocketHandle();			
	void SetId(int id);
	int GetId();
	void Send(Buffer::BufferPtr buf); 
	ETerminalType GetTerminalType();
	void SetTerminalType(ETerminalType terminalType);
private:
	ETerminalType  terminalType_ = eTerminalMp3;
	int id_ = -1;					// �ն�ID����ʶΨһ			
	int socketHandle_ = -1;					// ������
	unsigned char buffer_[BUFFER_LENGTH];
};
#endif
