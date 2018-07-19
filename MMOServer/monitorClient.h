#pragma once

class gameServer;

class monitorClient : public LanClient
{
private:
	gameServer *server;
	int serverNo;

	BYTE bufCode;
	BYTE bufKey1;
	BYTE bufKey2;
public:
	bool loginFlag;

private:
	void loadConfig(const char* _buffer);

	void proc_clientLogin();


	Sbuf* packet_clientLogin();
	Sbuf* packet_sendData(BYTE _dataType, int _dataValue, int _timeStamp);
	
public:
	monitorClient(const char *_configData);
	~monitorClient();

	// 로직함수
	void shutdownServer(void);

	void proc_sendData(BYTE _dataType, int _dataValue, int _timeStamp);

	// 가상함수
	virtual void OnClientJoin(void);
	virtual void OnClientLeave(void);
	virtual bool OnConnectionRequest(void); // accept 후 [false : 클라이언트 거부 / true : 접속 허용]
	virtual void OnRecv(Sbuf *_buf);		// 수신 완료 후
	virtual void OnSend(int _sendSize);	// 송신 완료 후
	virtual void OnError(int _errorCode, WCHAR *_string);		// 오류메세지 전송
	virtual void OnTPS(void);
};
