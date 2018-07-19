#pragma once

class GAMEServer : public MMOServer
{
private:
	HANDLE printHandle;
	bool		printFlag;
	int			Version;

public:
	monitorClient *client;
	
private:
	void loadConfig(const char *_configData);

public:
	GAMEServer(const char *_configData);
	~GAMEServer();

	void onAuth_Update();
	void onGame_Update();

	int getVersion();

	static unsigned __stdcall printThread(LPVOID _data);
};