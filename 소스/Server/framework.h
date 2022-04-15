﻿#pragma once
#include "stdafx.h"
#include "session.h"
#include "monster.h"

class NetworkFramework
{
public:
	NetworkFramework() : isAccept{ false } { }
	~NetworkFramework() = default;

	int OnInit(SOCKET socket);
	void AcceptThread(SOCKET socket);
	void SendLoginOkPacket(int id);
	void SendPlayerDataPacket();
	void SendMonsterDataPacket();
	void ProcessRecvPacket(int id);
	void Update(FLOAT deltaTime);
	void CollisionCheck();
	void Disconnect(int id);

	CHAR GetNewId();

public:
	bool							isAccept;	// 1명이라도 서버에 들어왔는지
	std::array<Session, MAX_USER>	clients;	// 클라이언트
	std::vector<Monster>			monsters;	// 몬스터
	std::vector<BulletData>			bullets;	// 총알
	std::vector<std::thread>		threads;	// 쓰레드
};
