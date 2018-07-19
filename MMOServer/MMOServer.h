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
	LONG recvTPS;
	LONG sendTPS;
	LONG authCount;
	LONG gameCount;
	LONG sendCount;
	LONG logoutCount;
	LONG authFrame;
	LONG gameFrame;
	LONG sendFrame;

	BYTE bufCode;
	BYTE bufKey1;
	BYTE bufKey2;

	PDH_HQUERY _pdh_Query;
	PDH_HCOUNTER _pdh_counter_Handle_GMem;


protected:
	unsigned int maxSession;
	lockFreeStack<unsigned int> indexStack;
	player	 *sessionArry;

	lockFreeQueue<AUTH_DATA*> AUTHQ;

	LONG	 sessionCount;
	ULONG64 acceptTotal;

	LONG	pAcceptTPS, pRecvTPS, pSendTPS, pSendCount;
	LONG	pAuth, pGame, pAuthFrame, pGameFrame, pSendFrame;

public:
	processCPU *cpuHandle;
	double privateMem;

private:
	void loadConfig(const char* _configData);

	bool connectRequest(SOCKADDR_IN _sockAddr);
	void recvPost(player *_ss);
	void sendPost(player *_ss);
	void completeSend(DWORD _trans, player *_ss);
	void completeRecv(DWORD _trans, player *_ss);

	void updatePDH(void);

public:
	MMOServer() {};
	~MMOServer() {};
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

