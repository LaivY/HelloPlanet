#pragma once
#include "session.h"

class NetFramework
{
	static NetFramework* instance;
public:
	NetFramework()
	{
		assert(instance == nullptr);
		instance = this;
	}
	//~NetFramework();
	CHAR GetNewId();
	void SendLoginOkPacket(int id);
	void Disconnect(int id);
	void ProcessRecvPacket(int id);
	void AcceptThread(SOCKET socket);
	void SendPlayerDataPacket();
	static NetFramework* GetInstance();

	std::array <Session, MAX_USER> clients;
	std::vector<std::thread> threads;
	bool m_isAccept = false; // 1명이라도 서버에 들어왔는지
};