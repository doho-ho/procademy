#include "stdafx.h"

GAMEServer::GAMEServer(const char *_configData)
{
	loadConfig(_configData);

	// 초기화
	printFlag = false;
	Version = 0.1;
	client = new monitorClient(_configData);

	playerArray = new player[maxUser];
	setSessionArry(playerArray, maxUser);

	Start(_configData);

	printHandle = (HANDLE)_beginthreadex(NULL, 0, printThread, (LPVOID)this, 0, 0);
	printf("START GAME SERVER\n");
}

GAMEServer::~GAMEServer()
{
	printFlag = true;
	CloseHandle(printHandle);
}

void GAMEServer::loadConfig(const char *_configData)
{
	// config 파일 읽어와서 멤버 변수에 값 저장.
	rapidjson::Document Doc;
	Doc.Parse(_configData);

	char mIP[16];

	rapidjson::Value &sys = Doc["NET"];
	maxUser = sys["MAX_USER"].GetUint();

	rapidjson::Value &code = sys["BUF_KEY"];

	bufCode = (char)code[0].GetInt();
	bufKey1 = (char)code[1].GetInt();
	bufKey2 = (char)code[2].GetInt();
}

void GAMEServer::onAuth_Update()
{

}

void GAMEServer::onGame_Update()
{

}

int GAMEServer::getVersion()
{
	return Version;
}

unsigned __stdcall GAMEServer::printThread(LPVOID data)
{
	GAMEServer *server = (GAMEServer*)data;
	monitorClient *client = server->client;

	memoryPool<Sbuf> *pool = Sbuf::pool;
	time_t timeStamp=0;
	client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON,true, timeStamp);
	while (1)
	{
		if (server->printFlag)
			break;
		time(&timeStamp);
		server->monitoring();
		printf("=================================================================\n");
		printf("\t\t\t\t*** GameServer ***\n\n");
		printf("[GAME] ========================================================\n");
		printf("   User Count : [ %01d ] \n", server->sessionCount);
		printf("   AUTH Q : [ %03d ]  / SEND THREAD : [ %d ] \n", server->AUTHQ.getUsedSize(), server->pSendFrame);
		printf("   AUTH Mode : [ %d ]  /  GAME Mode : [ %d ] \n", server->pAuth, server->pGame);
		printf("   AUTH THREAD : [ %d ]  / GAME THREAD : [ %d ]\n", server->pAuthFrame, server->pGameFrame);
		printf("   Sbuf Alloc : [ %d ]  /  Sbuf Used : [ %d ] \n", pool->getAllocCount(), pool->getUsedCount());
		printf("   COMQ USED   : [ %d ]  /  sendQ USED  : [ %d ]  \n", server->pcomQ, server->psdQ);
		printf("[NET] ==========================================================\n");
		printf("   accept Total : [ %d ] \n", server->acceptTotal);
		printf("   accept TPS   : [ %d ] \n", server->pAcceptTPS);
		printf("   Recv TPS     : [ %d ]  /  Send TPS : [ %d ]\n", server->pRecvTPS, server->pSendTPS);
		printf("   PACKET PROC : [ %d ]   /  SEND PROC  : [ %d ] \n\n", server->pProcPacket, server->pSendCount);

		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON, true, timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_CPU, server->cpuHandle->ProcessGameTotal(), timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT, server->privateMem/1048576.0f, timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL, pool->getUsedCount(), timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS, server->pAuthFrame, timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS, server->pGameFrame, timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL, server->sessionCount, timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH, server->pAuth, timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME, server->pGame, timeStamp);
		
		Sleep(900);
	}
	printf("printhread closed\n");
	return 0;
}

void GAMEServer::check_completeRecvQ(void)
{
	printf("\t//** COMPLETE RECV Q 88//\t\n");
	int count = 0; 
	for (count; count < maxUser; count++)
	{
		if (sessionArry[count]->completeRecvQ.getUsedSize() > 0)
			printf(" %d \t", count);
	}
}

void GAMEServer::check_sendQ(void)
{
	printf("\t//** SEND Q 88//\t\n");
	int count = 0;
	for (count; count < maxUser; count++)
	{
		if (sessionArry[count]->sendQ.getUsedSize() > 0)
			printf(" %d \t", count);
	}
}