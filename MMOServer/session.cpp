#include "stdafx.h"

GameSession::GameSession()
{
	Mode = en_SESSION_MODE::MODE_NONE;
	arryIndex = 0;
	sendCount = 0;
	sendFlag = 0;
	IOCount = 0;
	logoutFlag = false;
	authTOgame = false;
	shutFlag = false;

	completeRecvQ = new lockFreeQueue<Sbuf*>;
	sendQ = new lockFreeQueue<Sbuf*>;
}

GameSession::~GameSession()
{

}

void GameSession::sendPacket(Sbuf *_buf, bool _type)
{
	// sendQ에 enqueue만 하고 끝.
	if (Mode == MODE_NONE) return;
	_buf->Encode(bufCode, bufKey1, bufKey2);
	_buf->addRef();
	sendQ->enqueue(_buf);

	if (_type)
	{
		InterlockedCompareExchange(&disconnectFlag, true, false);
		if (sendQ->getUsedSize() == 0)
			disconnect();
	}
	return;
}

void GameSession::disconnect(void)
{
	shutFlag = true;
	shutdown(sock,2);		// both
	return;
}
