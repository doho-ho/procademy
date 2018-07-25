#include "stdafx.h"

player::player()
{
	// member variables initialization.
	accountNo = 0;

}

player::~player()
{

}




void player::onAuth_clientJoin(void)
{
	return;
}

void player::onAuth_clientLeave(void)
{
	return;
}

void player::onAuth_Packet(Sbuf *_buf)
{
	WORD type;
	*_buf >> type;
	switch (type)
	{
	case en_PACKET_CS_GAME_REQ_LOGIN:
		proc_loginReq(_buf);
		break;

	default:
		disconnect();
		break;
	}
	return;
}

void player::onGame_clientJoin(void)
{
	return;
}

void player::onGame_clientLeave(void)
{
	return;
}

void player::onGame_Packet(Sbuf *_buf)
{
	WORD type;
	*_buf >> type;
	switch (type)
	{
	case en_PACKET_CS_GAME_REQ_ECHO:
		proc_echo(_buf);
		break;

	default:
		disconnect(); 
		break;
	}
	return;
}

void player::onGame_Release(void)
{
	accountNo = 0;
	return;
}


void player::proc_loginReq(Sbuf *_buf)
{
	//		INT64	AccountNo
	//		char	SessionKey[64]
	//		int	Version			// Major 100 + Minor 10  = 1.10
	//						// 현재 최신 버전은		0.01 (1) - 2016.03.28

	*_buf >> accountNo;
	_buf->pop(sessionKey, 64);
	int Version;
	*_buf >> Version;

	BYTE status = 1;

	authTOgame = true;
	Sbuf * buf = packet_loginRes(status);
	sendPacket(buf);
	buf->Free();
}

void player:: proc_echo(Sbuf *_buf)
{
	//		INT64		AccountoNo
	//		LONGLONG	SendTick

	INT64 acNo = -1;
	LONGLONG sendTick;
	*_buf >> acNo;
	*_buf >> sendTick;

	Sbuf *buf = packet_echo(acNo, sendTick);
	sendPacket(buf);
	buf->Free();
}

Sbuf* player::packet_loginRes(BYTE _status)
{
	//		WORD	Type
	//		BYTE	Status (0: 실패 / 1: 성공 / 2: 신규캐릭터 선택 모드 / 3:버전 다름.)
	//		INT64	AccountNo
	Sbuf *buf = Sbuf::Alloc();
	*buf << (WORD)en_PACKET_CS_GAME_RES_LOGIN;
	*buf << _status;
	*buf << accountNo;

	return buf;
}

Sbuf *player::packet_echo(INT64 _acNo, LONGLONG _sendTick)
{
	//		WORD		Type
	//		INT64		AccountoNo
	//		LONGLONG	SendTick
	Sbuf *buf = Sbuf::Alloc();
	*buf << (WORD)en_PACKET_CS_GAME_RES_ECHO;
	buf->push((char*)&_acNo, sizeof(INT64));
	buf->push((char*)&_sendTick, sizeof(LONGLONG));

	return buf;
}