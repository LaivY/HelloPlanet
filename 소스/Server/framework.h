﻿#pragma once
#include "session.h"

class NetworkFramework
{
public:
	NetworkFramework() : isAccept{ false } { }
	~NetworkFramework() = default;

	void AcceptThread(SOCKET socket);
	void SendLoginOkPacket(int id);
	void SendPlayerDataPacket();

	void ProcessRecvPacket(int id);

	void Disconnect(int id);

	CHAR GetNewId();

public:
	bool							isAccept; // 1명이라도 서버에 들어왔는지
	std::array<Session, MAX_USER>	clients;
	std::vector<std::thread>		threads;
};