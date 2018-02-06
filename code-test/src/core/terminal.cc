#include "terminal.h"
#include <cstring>

Terminal::Terminal(int id, ETerminalType terminalType)		// ����һ��ʼ��ָ��ID���ն�����
	:id_(id_), 
	terminalType_(terminalType)
{

}
Terminal::~Terminal()
{

}
void Terminal::SetSocketHandle(int socketHandle)					// ����socket���
{
	socketHandle_ = socketHandle;
}
int Terminal::GetSocketHandle()
{
	return socketHandle_;
}


void Terminal::SetId(int id)
{
	id_ = id;
}
int Terminal::GetId()
{
	return id_;
}

void Terminal::Send(Buffer::BufferPtr &buf)
{
	// ͨ��socket�����ݷ��ͳ�ȥ
	// ���������ֱ�ӷ��ͣ���������ʹ�������̳߳�������
	if(!buf)
	{
		if(buf->Size() < BUFFER_LENGTH)
			memcpy(&buffer_[0], buf->Data(), buf->Size());		// ����ֻ��Ϊ�˲�������
		
		printf("id = %d, type = %d, send buffer len = %d\n", id_, terminalType_, buf->Size());
		buf.reset();		// �ͷ��ڴ�
	}
}

Terminal::ETerminalType Terminal::GetTerminalType()
{
	return terminalType_;
}

void Terminal::SetTerminalType(ETerminalType terminalType)
{
	terminalType_ = terminalType;
}


