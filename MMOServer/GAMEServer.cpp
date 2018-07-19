#include "stdafx.h"

GAMEServer::GAMEServer(const char *_configData)
{
	loadConfig(_configData);

	// ÃÊ±âÈ­
	printFlag = false;
	Version = 0.1;
	client = new monitorClient(_configData);

	Start(_configData);

	printHandle = (HANDLE)_beginthreadex(NULL, 0, printThread, (LPVOID)this, 0, 0);
	printf("START GAME SERVER\n");
}

GAMEServer::~GAMEServer()
{
	printFlag = true;
	CloseHandle(printHandle);
}

void GAMEServer::loadConfig(const char *_config)
{

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
		printf("   User Count : [ %03d ] \n", server->sessionCount);
		printf("   AUTH Q : [ %03d ]  / SEND THREAD : [ %d ] \n", server->AUTHQ.getUsedSize(), server->pSendFrame);
		printf("   AUTH Mode : [ %d ]  /  GAME Mode : [ %d ] \n", server->pAuth, server->pGame);
		printf("   AUTH THREAD : [ %d ]  / GAME THREAD : [ %d ]\n", server->pAuthFrame, server->pGameFrame);
		printf("   Sbuf Alloc : [ %d ]  /  Sbuf Used : [ %d ] \n", pool->getAllocCount(), pool->getUsedCount());
		printf("[NET] ==========================================================\n");
		printf("   accept Total : [ %d ] \n", server->acceptTotal);
		printf("   accept TPS   : [ %d ] \n", server->pAcceptTPS);
		printf("   Recv TPS     : [ %d ]  /  Send TPS : [ %d ] \n\n", server->pRecvTPS, server->pSendTPS);

		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_SERVER_ON, true, timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_CPU, server->cpuHandle->ProcessGameTotal(), timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_MEMORY_COMMIT, server->privateMem/1048576.0f, timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_PACKET_POOL, pool->getUsedCount(), timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_AUTH_FPS, server->pAuthFrame, timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_GAME_FPS, server->pGameFrame, timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_SESSION_ALL, server->sessionCount, timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_SESSION_AUTH, server->pAuth, timeStamp);
		client->proc_sendData(dfMONITOR_DATA_TYPE_BATTLE_SESSION_GAME, server->pGame, timeStamp);
		
		Sleep(990);
	}
	printf("printhread closed\n");
	return 0;
}