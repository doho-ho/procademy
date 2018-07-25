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
}

GameSession::~GameSession()
{

}

void GameSession::set_session(SOCKET _sock)
{
	sock = _sock;
	return;
}

void GameSession::sendPacket(Sbuf *_buf, bool _type)
{
	// sendQ에 enqueue만 하고 끝.
	if (Mode == MODE_NONE) return;
	if (disconnectFlag == true) return;
	_buf->Encode(bufCode, bufKey1, bufKey2);
	_buf->addRef();
	sendQ.enqueue(_buf);
	InterlockedIncrement(&server->sdQ);

	if (_type)
	{
		InterlockedCompareExchange(&disconnectFlag, true, false);
		if (sendQ.getUsedSize() == 0)
			disconnect();
	}
	return;
}

void GameSession::disconnect(void)
{
	shutdown(sock,2);		// both
	return;
}

void GameSession::set_server(MMOServer *_server)
{
	server = _server;
}

void GameSession::set_bufCode(BYTE _bufCode, BYTE _bufKey1, BYTE _bufKey2)
{
	bufCode = _bufCode;
	bufKey1 = _bufKey1;
	bufKey2 = _bufKey2;
}