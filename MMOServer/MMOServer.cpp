#include "stdafx.h"

__int64 id = 1;

#define setID(index, clientID) (index << 44) | clientID
#define getID(clientID) (clientID<<16) >>16
#define getIndex(clientID) clientID>>48

void MMOServer::loadConfig(const char *_configData)
{
		// config 파일 읽어와서 멤버 변수에 값 저장.
		rapidjson::Document Doc;
		Doc.Parse(_configData);

		char mIP[16];
		// IP Adress, port, maxCount, threadCount
		// DB 정보
		rapidjson::Value &sys = Doc["NET"];
		strcpy_s(mIP, sys["SERVER_IP"].GetString());
		MultiByteToWideChar(CP_ACP, 0, mIP, 16, ip, 16);
		port = sys["SERVER_PORT"].GetUint();
		nagleOpt = sys["NAGLE"].GetBool();
		workerCount = sys["WORKER_THREAD"].GetUint();
		maxSession = sys["MAX_USER"].GetUint();


		rapidjson::Value &code = sys["BUF_KEY"];
		assert(arry.IsArry());

		bufCode = (char)code[0].GetInt();
		bufKey1 = (char)code[1].GetInt();
		bufKey2 = (char)code[2].GetInt();


	return;
}

bool MMOServer::Start(const char* _buffer)
{
	loadConfig(_buffer);

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		_SYSLOG(L"ERROR", Level::SYS_SYSTEM, L"WSASTARTUP ERROR %d ", WSAGetLastError());
		return false;
	}

	// SOCKET SETTING
	listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSock == INVALID_SOCKET)
	{
		_SYSLOG(L"ERROR", Level::SYS_SYSTEM, L"SOCKET SETTING ERROR %d", WSAGetLastError());
		return false;
	}

	// IOCP 핸들 등록
	IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!IOCPHandle)
	{
		_SYSLOG(L"ERROR", Level::SYS_SYSTEM, L"IOCP CREATE ERROR %d", WSAGetLastError());
		return false;
	}

	// NAGLE SETTING
	if (!nagleOpt)
	{
		bool optVal = TRUE;
		setsockopt(listenSock, IPPROTO_TCP, TCP_NODELAY, (char*)&optVal, sizeof(optVal));
	}

	// BIND
	SOCKADDR_IN serverAddr;
	IN_ADDR addr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);

	int retval = bind(listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
	if (retval == INVALID_SOCKET)
	{
		_SYSLOG(L"ERROR", Level::SYS_SYSTEM, L"BIND ERROR %d", WSAGetLastError());
		return false;
	}

	// LISTEN
	retval = listen(listenSock, SOMAXCONN);
	if (retval == INVALID_SOCKET)
	{
		_SYSLOG(L"ERROR", Level::SYS_SYSTEM, L"LISTEN ERROR %d", WSAGetLastError());
		return false;
	}

	// KEEP ALIVE OPTION SETTING
	tcpKeep.onoff = 1;
	tcpKeep.keepalivetime = 10000;
	tcpKeep.keepaliveinterval = 10;
	WSAIoctl(listenSock, SIO_KEEPALIVE_VALS, (tcp_keepalive*)&tcpKeep, sizeof(tcp_keepalive), NULL, 0, NULL, NULL, NULL);

	// INIT
	acceptTPS = 0;
	acceptTotal = 0;
	recvTPS = 0;
	sendTPS = 0;
	threadFlag = false;
	authCount = 0, gameCount = 0;
	authFrame = 0, gameFrame = 0;

	sessionArry = new player[maxSession];
	int i =(maxSession-1);
	for (i; i >= 0; i--)
	{
		indexStack.push(i);
		sessionArry[i].set_bufCode(bufCode, bufKey1, bufKey2);
	}
	// PDH INIT
	PdhOpenQuery(NULL, NULL, &_pdh_Query);
	PdhAddCounter(_pdh_Query, L"\\Process(MMOServer)\\Working Set", NULL, &_pdh_counter_Handle_GMem);
	cpuHandle = new processCPU();

	// CREATE THREAD
	HANDLE handle;
	handle = (HANDLE)_beginthreadex(NULL, 0, acceptThread, (LPVOID)this, 0, 0);
	CloseHandle(handle);
	handle = (HANDLE)_beginthreadex(NULL, 0, AUTHThread, (LPVOID)this, 0, 0);
	CloseHandle(handle);
	handle = (HANDLE)_beginthreadex(NULL, 0, GameThread, (LPVOID)this, 0, 0);
	CloseHandle(handle);
	handle = (HANDLE)_beginthreadex(NULL, 0, SendThread, (LPVOID)this, 0, 0);
	CloseHandle(handle);

	workerHandle = new HANDLE[workerCount];
	int count = 0;
	for(count;count<workerCount;count++)
		workerHandle[count] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, (LPVOID)this, 0, 0);

	printf("Start MMO SERVER.\n");
	return true;
}

void MMOServer::Stop(void)
{

}

bool MMOServer::connectRequest(SOCKADDR_IN _sockAddr)
{
	// 로그인 세션 인증해주세요. 
	return true;
}

void MMOServer::recvPost(player *_ss)
{
	DWORD recvVal, flag;
	int retval, err;
	winBuffer *recvQ = &_ss->recvQ;
	ZeroMemory(&(_ss->recvOver), sizeof(_ss->recvOver));
	WSABUF buf[2];
	ZeroMemory(&buf, sizeof(WSABUF) * 2);

	// 만약 recvQ->getFreeSize() == 0 일 경우 잘못된 상황입니다. 
	// 원형큐 버퍼 사이즈가 1만인데 1만의 데이터를 다 빼주지 못했다는건 중대한 오류입니다. 연결 끊어요.
	if (recvQ->getFreeSize() == 0)
		_ss->disconnect();

	buf[0].buf = recvQ->getRearPosPtr();
	buf[0].len = recvQ->getNotBrokenFreeSize();
	if (recvQ->getFreeSize() > recvQ->getNotBrokenFreeSize())
	{
		buf[1].buf = recvQ->getBufferPtr();
		buf[1].len = (recvQ->getFreeSize() - recvQ->getNotBrokenFreeSize());
	}

	recvVal = 0, flag = 0;
	InterlockedIncrement(&_ss->IOCount);
	retval = WSARecv(_ss->sock, buf, 2, &recvVal, &flag, &(_ss->recvOver), NULL);
	if (retval == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		if (err == WSA_IO_PENDING)
			return;
		if (err != WSAECONNRESET && err != WSAESHUTDOWN && err != WSAECONNABORTED)
			_SYSLOG(L"ERROR", Level::SYS_ERROR, L"RECV POST ERROR [accountNo : %d] %d", _ss->accountNo,err);
		_ss->disconnect();
		if (InterlockedDecrement(&_ss->IOCount) == 0)
			_ss->logoutFlag = true;		
	}
	return;
}

void MMOServer::sendPost(player *_ss)
{
	DWORD sendVal;
	int count, size, retval, err;
	Sbuf *node = NULL;
	WSABUF buf[maxWSABUF];
	size = _ss->sendQ.getUsedSize();
	if (size <= 0)
		return;
	if (FALSE == (BOOL)InterlockedCompareExchange(&(_ss->sendFlag), TRUE, FALSE))
	{
		count = 0;
		retval = 0;
		ZeroMemory(&(_ss->sendOver), sizeof(_ss->sendOver));
		lockFreeQueue<Sbuf*> *sendQ = &_ss->sendQ;
		do
		{
			for (count; count < maxWSABUF; count++)
			{
				node = NULL;
				retval = sendQ->peek(&node, count);
				if (retval == -1 || !node) break;
				buf[count].buf = node->getHeaderPtr();
				buf[count].len = node->getPacketSize();
			}
			if (count == maxWSABUF)
				break;
		} while (sendQ->getUsedSize() > count);
		_ss->sendCount = count;
		sendCount += count;
		if (count == 0)
		{
			InterlockedCompareExchange(&(_ss->sendFlag), FALSE, TRUE);
			return;
		}

		InterlockedIncrement(&_ss->IOCount);
		retval = WSASend(_ss->sock, buf, _ss->sendCount, &sendVal, 0, &_ss->sendOver, NULL);
		if (retval == SOCKET_ERROR)
		{
			err = WSAGetLastError();
			if (err == WSA_IO_PENDING)
				return;
			if (err != WSAECONNRESET && err != WSAESHUTDOWN && err != WSAECONNABORTED)
				_SYSLOG(L"ERROR", Level::SYS_ERROR, L"SEND POST ERROR [accountNo : %d] %d", _ss->accountNo, err);
			_ss->disconnect();
			InterlockedCompareExchange(&(_ss->sendFlag), FALSE, TRUE);
			if (InterlockedDecrement(&_ss->IOCount) == 0)
				_ss->logoutFlag = true;
		}
	}
	return;
}

void MMOServer::completeRecv(DWORD _trans, player *_ss)
{
	if (_trans == 0)
	{
		_ss->disconnect();
		return;
	}
	int retval, usedSize;
	int headerSize = sizeof(header);
	header head;

	winBuffer *recvQ = &_ss->recvQ;
	Sbuf *buf = NULL;
	recvQ->moveRearPos(_trans);
	while (1)
	{
		usedSize = recvQ->getUsedSize();	
		retval= recvQ->peek((char*)&head, headerSize);
		if (retval < headerSize || (usedSize - headerSize) < head.len)
			break;
		InterlockedIncrement(&recvTPS);
		try
		{
			buf = Sbuf::Alloc();
			recvQ->dequeue(buf->getBufPtr(), headerSize + head.len);
			buf->moveRearPos(head.len);
			if (buf->Decode(bufCode, bufKey1, bufKey2))
				_ss->completeRecvQ.enqueue(buf);
			else
				throw 4900;
		}
		catch (int num)
		{
			if(num != 4900)
				_SYSLOG(L"SYSTEM", Level::SYS_ERROR, L"SBUF ERROR [accountNo : %d] %d",_ss->accountNo, num);
			_ss->disconnect();
			return;
		}
	}
	recvPost(_ss);
	return;
}

void MMOServer::completeSend(DWORD _trans, player *_ss)
{
	if (_trans == 0)
	{
		InterlockedDecrement(&_ss->sendFlag);
		_ss->disconnect();
		return;
	}
	Sbuf *buf = NULL;
	int sendCount, count;
	lockFreeQueue<Sbuf*> *sendQ = &_ss->sendQ;
	sendCount = _ss->sendCount;
	count = 0;
	for (count; count < sendCount;)
	{
		buf = NULL;
		sendQ->dequeue(&buf);
		if (!buf) continue;
		buf->Free();
		count++;
		InterlockedIncrement(&sendTPS);
	}
	InterlockedDecrement(&_ss->sendFlag);
	// 보내고 끊기 기능
	if (_ss->sendQ.getUsedSize() == 0 && _ss->disconnectFlag == 1)
	{
		if (true == InterlockedCompareExchange(&(_ss->disconnectFlag), true, true))
			_ss->disconnect();
		return;
	}
	sendPost(_ss);
	return;
}

void MMOServer::updatePDH(void)
{
	// pdh 값 갱신 및 저장.
	PdhCollectQueryData(_pdh_Query);

	// 데이터 뽑기
	PDH_FMT_COUNTERVALUE counterVal;

	PDH_STATUS status = PdhGetFormattedCounterValue(_pdh_counter_Handle_GMem, PDH_FMT_DOUBLE, NULL, &counterVal);
	if (status == 0)
		privateMem = counterVal.doubleValue;
}

unsigned __stdcall MMOServer::acceptThread(LPVOID _data)
{
	MMOServer *node = (MMOServer*)_data;
	
	SOCKET clientSock;
	SOCKADDR_IN clientAddr;
	SOCKET listenSock = node->listenSock;
	int len;
	while(1)
	{
		len = sizeof(clientAddr);
		clientSock = accept(listenSock, (SOCKADDR*)&clientAddr, &len);
		if (clientSock == INVALID_SOCKET) break;
		node->acceptTPS++;
		node->acceptTotal++;
		if (node->maxSession == node->sessionCount)
		{
			shutdown(clientSock, SD_BOTH);
			closesocket(clientSock);
			continue;
		}
		InterlockedIncrement(&node->sessionCount);
		AUTH_DATA *data = new AUTH_DATA;
		data->sock = clientSock;
		data->sockAddr = clientAddr;
		WSAIoctl(clientSock, SIO_KEEPALIVE_VALS, (tcp_keepalive*)&node->tcpKeep, sizeof(tcp_keepalive), NULL, 0, NULL, NULL, NULL);
		node->AUTHQ.enqueue(data);
	}
	node->threadFlag = true;
	return 0;
}

unsigned __stdcall MMOServer::AUTHThread(LPVOID _data)
{
	MMOServer *node = (MMOServer*)_data;

	while (1)
	{
		if (node->threadFlag)
			break;
		node->acceptProcess();		// 접속처리 파트
		node->checkProcess();		// 인증처리 파트
		node->AUTHMODE();			// 모드 변경 파트
	//	node->onAuth_Update();
		node->authFrame++;
		Sleep(10);	// 2 -10ms 단위
	}
	return 0;
}

void MMOServer::acceptProcess(void)
{
	// AUTHQ에서 값 빼서 처리.
	AUTH_DATA *data = NULL;
	unsigned int index = 0;
	int count = 0;
	while (1)
	{
		if (count == AUTH_ACCEPT_DELAY || AUTHQ.getUsedSize() == 0)
			break;
		data = NULL;
		AUTHQ.dequeue(&data);
		if (!data)
			break;
		if (!connectRequest(data->sockAddr))
		{
			delete data;
			continue;
		}
		if (indexStack.pop(&index) == -1)
			CCrashDump::Crash();
		
		sessionArry[index].arryIndex = index;
		sessionArry[index].clientID = setID(index, id);
		id++;
		sessionArry[index].setSession(data->sock);
		CreateIoCompletionPort((HANDLE)data->sock, IOCPHandle, (ULONG_PTR)&sessionArry[index], 0);
		InterlockedIncrement(&sessionArry[index].IOCount);

		sessionArry[index].onAuth_clientJoin();
		recvPost(&sessionArry[index]);
		sessionArry[index].Mode = MODE_AUTH;
		authCount++;
		if (InterlockedDecrement(&sessionArry[index].IOCount) == 0)
			sessionArry[index].logoutFlag = true;
		count++;
		delete data;
	}
	return;
}

void MMOServer::checkProcess(void)
{
	// 세션을 모두 돌면서 completeRecvQ의 데이터 뽑아서 처리.
	int count = 0;
	int delayCount = 0;
	Sbuf *buf = NULL;
	for (count; count < maxSession; count++)
	{
		if (sessionArry[count].Mode == MODE_AUTH)
		{
			while (delayCount < AUTH_PACKET_DELAY)
			{
				buf = NULL;

				sessionArry[count].completeRecvQ.dequeue(&buf);
				if (!buf)
					break;

				delayCount++;
				sessionArry[count].onAuth_Packet(buf);
				buf->Free();
			}
			if (sessionArry[count].logoutFlag == true && sessionArry[count].sendFlag == false)
				sessionArry[count].Mode = MODE_LOGOUT_IN_AUTH;
		}
	}
		
	return;
}

void MMOServer::AUTHMODE(void)
{
	int count = 0;
	for (count; count < maxSession; count++)
	{
		if (sessionArry[count].Mode == MODE_LOGOUT_IN_AUTH)
		{
			if (sessionArry[count].sendFlag == false && sessionArry[count].sendQ.getUsedSize() == 0)
			{
				authCount--;
				InterlockedIncrement(&logoutCount);
				sessionArry[count].Mode = WAIT_LOGOUT;
			}
		}

		else if (sessionArry[count].authTOgame == true)
		{
			authCount--;
			sessionArry[count].Mode = MODE_AUTH_TO_GAME;
			sessionArry[count].authTOgame = false;
		}
		else
			continue;
		sessionArry[count].onAuth_clientLeave();
	}
	return;
}

unsigned __stdcall MMOServer::GameThread(LPVOID _data)
{
	MMOServer *node = (MMOServer*)_data;
	while (1)
	{
		if (node->threadFlag)
			break;
		node->GAMEMODE();
		node->gamePacket();
	//	node->onGame_Update();
		node->logoutProcess();
		node->Release();
		node->gameFrame++;
		Sleep(1);
	}
	return 0;
}

void MMOServer::GAMEMODE(void)
{
	int count = 0;
	int dealy = 0;
	for (count; count < maxSession; count++)
	{
		if (dealy == GAME_MODE_DEALY)break;
		if (sessionArry[count].Mode == MODE_AUTH_TO_GAME)
		{
			gameCount++; 
			sessionArry[count].Mode = MODE_GAME;
			sessionArry[count].onGame_clientJoin();
			dealy++;
		}
		if (sessionArry[count].logoutFlag == true && sessionArry[count].sendFlag == false)
			sessionArry[count].Mode = MODE_LOGOUT_IN_GAME;
	}
	return;
}

void MMOServer::gamePacket(void)
{
	// 세션을 모두 돌면서 completeRecvQ의 데이터 뽑아서 처리.
	int count = 0, loop = 0;
	Sbuf *buf = NULL;
	for (count; count < maxSession; count++)
	{
		if (sessionArry[count].Mode == MODE_GAME)
		{
			loop = 0;
			while (loop < GAME_PACKET_DEALY)
			{
				buf = NULL;
				sessionArry[count].completeRecvQ.dequeue(&buf);
				if (!buf)
					break;
				sessionArry[count].onGame_Packet(buf);
				buf->Free();
				loop++;
			}
			procPacket += loop;
		}
	}
}

void MMOServer::logoutProcess(void)
{
	int count = 0;
	int dealy = 0;
	for (count; count < maxSession; count++)
	{
		if (dealy == GAME_LOGOUT_DEALY) break;

		if (sessionArry[count].Mode == MODE_LOGOUT_IN_GAME && sessionArry[count].sendFlag == false)
		{
			sessionArry[count].onGame_clientLeave();
			sessionArry[count].Mode = WAIT_LOGOUT;
			gameCount--;
			dealy++;
		}
	}
}

void MMOServer::Release(void)
{
	Sbuf *buf = NULL;
	int count = 0;
	int dealy = 0;
	SOCKET dummySock = INVALID_SOCKET;
	for (count; count < maxSession; count++)
	{
		if (dealy == GAME_RELEASE_DEALY) break;
		if (sessionArry[count].Mode == WAIT_LOGOUT)
		{
			sessionArry[count].onGame_Release();

			while (1)
			{
				buf = NULL;
				sessionArry[count].sendQ.dequeue(&buf);
				if (!buf) break;
				buf->Free();
			}
			while (1)
			{
				buf = NULL;
				sessionArry[count].completeRecvQ.dequeue(&buf);
				if (!buf) break;
				buf->Free();
			}

			dummySock = sessionArry[count].sock;
			sessionArry[count].sock = INVALID_SOCKET;
			sessionArry[count].accountNo = 0;
			sessionArry[count].recvQ.clearBuffer();
			sessionArry[count].logoutFlag = false;
			sessionArry[count].disconnectFlag = false;
			sessionArry[count].authTOgame = false;
			sessionArry[count].shutFlag = false;
			sessionArry[count].Mode = MODE_NONE;
			indexStack.push(sessionArry[count].arryIndex);
			closesocket(dummySock);
			if (InterlockedDecrement(&sessionCount) < 0)
				CCrashDump::Crash();
			dealy++;
		}
	}
}


unsigned __stdcall MMOServer::SendThread(LPVOID _data)
{
	MMOServer *node = (MMOServer*)_data;
	unsigned int maxSession = node->maxSession;
	player* session = NULL;
	while (!node->threadFlag)
	{
		session = node->sessionArry;
		int count = 0;
		for (count; count < maxSession; count++, session++)
		{
			if (session->Mode != MODE_AUTH && session->Mode != MODE_GAME)
				continue;
			if (session->sendFlag == false || session->sendQ.getUsedSize() > 0)
				node->sendPost(session);
		}
		node->sendFrame++;
		Sleep(5);
	}
	return 0;
}

unsigned __stdcall MMOServer::WorkerThread(LPVOID _data)
{
	MMOServer *node = (MMOServer*)_data;
	int retval = 1;
	DWORD trans = 0;
	OVERLAPPED *over = NULL;
	player *_ss = NULL;

	srand(GetCurrentThreadId());
	while (1)
	{
		trans = 0;
		over = NULL, _ss = NULL;
		retval = GetQueuedCompletionStatus(node->IOCPHandle, &trans, (PULONG_PTR)&_ss, (LPOVERLAPPED*)&over, INFINITE);
		if (!over)
		{
			if (retval == false)
				break;
			if (trans == 0 && !_ss)		// 종료 신호
				break;
		}
		else
		{
			if (&(_ss->recvOver) == over)
				node->completeRecv(trans, _ss);

			if (&(_ss->sendOver) == over)
				node->completeSend(trans, _ss);

			if (0 == InterlockedDecrement(&_ss->IOCount))
				_ss->logoutFlag = true;
		}
	}
	return 0;
}

void MMOServer::monitoring(void)
{
	pAcceptTPS = acceptTPS;
	pRecvTPS = recvTPS;
	pSendTPS = sendTPS;
	pAuth = authCount;
	pGame = gameCount;
	pSendCount = sendCount;
	pAuthFrame = authFrame;
	pGameFrame = gameFrame;
	pSendFrame = sendFrame;
	pProcPacket = procPacket;

	acceptTPS = 0;
	recvTPS = 0;
	sendTPS = 0;
	sendCount = 0;
	authFrame = 0;
	gameFrame = 0;
	sendFrame = 0;

	procPacket = 0;

	updatePDH();
	cpuHandle->UpdateCpuTime();
}
