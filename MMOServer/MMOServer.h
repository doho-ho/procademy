#pragma once

struct AUTH_DATA
{
	SOCKET sock;
	SOCKADDR_IN sockAddr;
};

class MMOServer
{
private:
	// CONFIG VALUE
	bool nagleOpt;
	WCHAR ip[16];
	unsigned short port;
	unsigned char workerCount;

	// SOCKET
	SOCKET listenSock;
	tcp_keepalive tcpKeep;

	// HANDLE
	HANDLE *workerHandle;
	HANDLE IOCPHandle;

	bool threadFlag;

	LONG acceptTPS;

	// Monitoring
	LONG recvTPS, sendTPS;				// TPS
	LONG authCount, gameCount;			// 유저수
	LONG sendCount;
	LONG logoutCount;
	LONG authFrame, gameFrame, sendFrame;	// 프레임수

	PDH_HQUERY _pdh_Query;
	PDH_HCOUNTER _pdh_counter_Handle_GMem;

	// test
	unsigned __int64 procPacket;

protected:
	BYTE bufCode;
	BYTE bufKey1;
	BYTE bufKey2;

	unsigned int maxSession;
	GameSession	 **sessionArry;
	lockFreeStack<unsigned int> indexStack;

	lockFreeQueue<AUTH_DATA*> AUTHQ;

	LONG	 sessionCount;
	ULONG64 acceptTotal;

	LONG	pAcceptTPS, pRecvTPS, pSendTPS, pSendCount;
	LONG	pAuth, pGame, pAuthFrame, pGameFrame, pSendFrame;
	ULONGLONG pProcPacket;
public:
	processCPU *cpuHandle;
	double privateMem;

	LONG comQ, pcomQ;
	LONG sdQ, psdQ;

private:
	void loadConfig(const char* _configData);

	bool connectRequest(SOCKADDR_IN _sockAddr);
	void recvPost(GameSession *_ss);
	void sendPost(GameSession *_ss);
	void completeSend(DWORD _trans, GameSession *_ss);
	void completeRecv(DWORD _trans, GameSession *_ss);

	void updatePDH(void);

public:
	MMOServer() {};
	~MMOServer() {};

	void setSessionArry(player *_array, unsigned int _maxSession);
	// 스레드
	static unsigned __stdcall acceptThread(LPVOID _data);

	static unsigned __stdcall AUTHThread(LPVOID _data);
	void			acceptProcess(void);
	void			checkProcess(void);
	void			AUTHMODE(void);

	static unsigned __stdcall GameThread(LPVOID _data);
	void			GAMEMODE(void);
	void			gamePacket(void);
	void			logoutProcess(void);
	void			Release(void);

	
	static unsigned __stdcall SendThread(LPVOID _data);

	static unsigned __stdcall WorkerThread(LPVOID _data);


	// 함수
	bool Start(const char *_buffer);
	void Stop(void);

	void monitoring(void);

	// 가상함수
//	virtual void onAuth_Update(void) = 0;
//	virtual void onGame_Update(void) = 0;

	friend class player;

};

