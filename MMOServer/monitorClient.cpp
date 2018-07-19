#include "stdafx.h"


monitorClient::monitorClient(const char* _configData)
{
	loadConfig(_configData);
	serverNo = 2;
	Start(bufCode, bufKey1,bufKey2);
}

monitorClient::~monitorClient(void)
{
	Stop();
}

void monitorClient::proc_clientLogin()
{
	Sbuf *buf = packet_clientLogin();
	SendPacket(buf);
	buf->Free();
}

void monitorClient::proc_sendData(BYTE _dataType, int _dataValue, int _timeStamp)
{
	Sbuf *buf = packet_sendData(_dataType, _dataValue, _timeStamp);
	SendPacket(buf);
	buf->Free();
}

Sbuf* monitorClient::packet_clientLogin()
{
	//		WORD	Type
	//
	//		int		ServerNo		// 서버 타입 없이 각 서버마다 고유 번호를 부여하여 사용

	Sbuf *buf = Sbuf::Alloc();
	*buf << (WORD)en_PACKET_SS_MONITOR_LOGIN;
	*buf << serverNo;
	 
	return buf;
}

Sbuf* monitorClient::packet_sendData(BYTE _dataType, int _dataValue, int _timeStamp)
{
	//		WORD	Type
	//
	//		BYTE	DataType				// 모니터링 데이터 Type 하단 Define 됨.
	//		int		DataValue				// 해당 데이터 수치.
	//		int		TimeStamp				// 해당 데이터를 얻은 시간 TIMESTAMP  (time() 함수)

	Sbuf *buf = Sbuf::Alloc();
	*buf << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
	*buf << _dataType;
	*buf << _dataValue;
	*buf << _timeStamp;

	return buf;
}

void monitorClient::OnClientJoin(void) 
{
	proc_clientLogin();
}

void monitorClient::OnClientLeave(void)
{
	loginFlag = false;
}

void monitorClient::OnRecv(Sbuf *_buf) {  }
bool monitorClient::OnConnectionRequest(void) { return true; }
void monitorClient::OnSend(int _sendSize) {  }
void monitorClient::OnError(int _errorCode, WCHAR *_string) {  }
void monitorClient::OnTPS(void) {  }

void monitorClient::loadConfig(const char *_buffer)
{
	// config 파일 읽어와서 멤버 변수에 값 저장.
	rapidjson::Document Doc;
	Doc.Parse(_buffer);

	// IP Adress, port, maxCount, threadCount
	// DB 정보
	char dummyIP[16];
	rapidjson::Value &sev = Doc["MONITOR_CLIENT"];
	strcpy_s(dummyIP, sev["SERVER_IP"].GetString());
	MultiByteToWideChar(CP_ACP, 0, dummyIP, 16, ip, 16);
	port = sev["SERVER_PORT"].GetUint();
	nagleOpt = sev["NAGLE"].GetBool();
	threadCount = sev["WORKER_THREAD"].GetUint();
	serverNo = sev["SERVER_NO"].GetInt();

	rapidjson::Value &code = sev["BUF_KEY"];
	assert(arry.IsArry());

	bufCode = (char)code[0].GetInt();
	bufKey1 = (char)code[1].GetInt();
	bufKey2 = (char)code[2].GetInt();
}

void monitorClient::shutdownServer(void)
{
	printf("recv shotdown server");
	return;
}

