#pragma once

class GAMEServer : public MMOServer
{
private:
	HANDLE printHandle;
	bool		printFlag;
	int			Version;

	unsigned int maxUser;

public:
	monitorClient *client;
	player *playerArray;
	
private:
	void loadConfig(const char *_configData);

public:
	GAMEServer(const char *_configData);
	~GAMEServer();

	void onAuth_Update();
	void onGame_Update();

	int getVersion();

	static unsigned __stdcall printThread(LPVOID _data);

	// test function
	void check_completeRecvQ(void);
	void check_sendQ(void);
};