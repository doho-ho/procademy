#pragma once

#define AUTH_ACCEPT_DELAY  100
#define AUTH_PACKET_DELAY 1000
#define GAME_PACKET_DEALY 5000
#define GAME_RELEASE_DEALY 100
#define GAME_MODE_DEALY 100
#define GAME_LOGOUT_DEALY 100

#define dfID_MAX_LEN 10
#define dfNICK_MAX_LEN 10
#define maxWSABUF 100

typedef ULONG64 SESSION_ID;

struct sessionLock
{
	LONG flag;
	LONG IOCP_count;
};

struct session
{
	session()
	{
		lock.IOCP_count = 0;
		lock.flag = false;

		id = 0;
		sock = INVALID_SOCKET;
		IOCP_count = &lock.IOCP_count;
		sendCount = 0;
		sendFlag = 0;
		sendPostCount = 0;
		disconnectFlag = false;
	}

	SESSION_ID id;

	SOCKET sock;

	winBuffer recvQ;
	lockFreeQueue<Sbuf*> sendQ;
	OVERLAPPED recvOver, sendOver;
	LONG *IOCP_count;
	LONG sendCount;
	LONG sendPostCount;
	LONG disconnectFlag;		// 보내고 끊기 플래그. true 면 보내고 끊어라.
	volatile LONG sendFlag;
	volatile LONG recvFlag;

	sessionLock lock;
};

#pragma pack(push, 1)
struct header
{
	BYTE code;
	WORD len;
	BYTE randCode;
	BYTE checkSum;
};

struct LanHeader
{
	WORD len;
};
#pragma pack(pop)

