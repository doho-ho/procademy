#pragma once

class gameServer;

enum playerStatus
{
	NONE, MOVE, ATTACK, ATTACK_MOVE
};


class player : public GameSession
{
public:
	INT64 accountNo;
	char sessionKey[64];

	WCHAR id[dfID_MAX_LEN];
	WCHAR nickName[dfNICK_MAX_LEN];

	// 기타 필요 컨텐츠들 다음에 추가.
public:
	player();
	~player();



	// 로직함수
		// auth
		void proc_loginReq(Sbuf *_buf);

		// game
		void proc_echo(Sbuf *_buf);

	// 패킷 함수
	Sbuf* packet_loginRes(BYTE _status);
	Sbuf *packet_echo(INT64 _acNo, LONGLONG _sendTick);

	// 가상함수
	void onAuth_clientJoin(void);
	void onAuth_clientLeave(void);
	void onAuth_Packet(Sbuf *_buf);

	void onGame_clientJoin(void);
	void onGame_clientLeave(void);
	void onGame_Packet(Sbuf *_buf);

	void onGame_Release(void);

	friend class gameServer;
};