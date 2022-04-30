#pragma once
#include "stdafx.h"
#include "session.h"
#include "monster.h"

class NetworkFramework
{
public:
	NetworkFramework();
	~NetworkFramework() = default;

	int OnInit(SOCKET socket);
	void AcceptThread(SOCKET socket);
	void SendLoginOkPacket(const int id, const char* name) const;
	void SendReadyToPlayPacket(const int id, const eWeaponType weaponType) const;
	void SendPlayerDataPacket();
	void SendBulletHitPacket();
	void SendMonsterDataPacket();
	void ProcessRecvPacket(const int id);
	void Update(FLOAT deltaTime);
	void SpawnMonsters(FLOAT deltaTime);
	void CollisionCheck();
	void Disconnect(const int id);

	UCHAR DetectPlayer(const DirectX::XMFLOAT3& pos) const;
	CHAR GetNewId() const;

public:
	bool							isAccept;			// 1명이라도 서버에 들어왔는지
	int								readyCount;			// 레디한 인원
	std::array<Session, MAX_USER>	clients;			// 클라이언트
	std::vector<Monster>			monsters;			// 몬스터
	std::vector<BulletData>			bullets;			// 총알
	std::vector<BulletHitData>		bulletHits;			// 총알을 맞춘 정보
	std::vector<std::thread>		threads;			// 쓰레드
	FLOAT							m_spawnCooldown;	// 스폰 쿨다운
	CHAR							m_lastMobId;		// 몬스터 ID

};