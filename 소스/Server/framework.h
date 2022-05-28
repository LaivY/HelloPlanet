#pragma once
#include "session.h"
#include "monster.h"

constexpr INT stage1Goal = 1;
constexpr FLOAT g_spawnCooldown = 2.0f;

class NetworkFramework
{
public:
	NetworkFramework();
	~NetworkFramework() = default;

	int OnInit(SOCKET socket);
	void AcceptThread(SOCKET socket);
	void ProcessRecvPacket(const int id);

	void SendLoginOkPacket(const Session& player) const;
	void SendSelectWeaponPacket(const Session& player) const;
	void SendReadyCheckPacket(const Session& player) const;
	void SendChangeScenePacket(const eSceneType sceneType) const;
	void SendPlayerDataPacket();
	void SendBulletHitPacket();
	void SendMonsterDataPacket();
	void SendMonsterAttackPacket(const int id, const int mobId, const int damage) const;
	void SendRoundResultPacket(const eRoundResult result) const;
	void SendPacket2AllPlayer(const void* packet, int bufSize) const;

	void Update(FLOAT deltaTime);
	void SpawnMonsters(FLOAT deltaTime);
	void CollisionCheck();
	void Disconnect(const int id);

	UCHAR DetectPlayer(const DirectX::XMFLOAT3& pos) const;
	CHAR GetNewId() const;

public:
	BOOL							isAccept;			// 1명이라도 서버에 들어왔는지
	BOOL							isClearStage1;
	BOOL							isInGame;
	INT								readyCount;			// 레디한 인원
	std::array<Session, MAX_USER>	clients;			// 클라이언트
	std::vector<Monster>			monsters;			// 몬스터
	std::vector<BulletData>			bullets;			// 총알
	std::vector<BulletHitData>		bulletHits;			// 총알을 맞춘 정보
	std::vector<std::thread>		threads;			// 쓰레드
	FLOAT							m_spawnCooldown;	// 스폰 쿨다운
	CHAR							m_lastMobId;		// 몬스터 ID
	INT								m_killScore;		// 잡은 몬스터 점수
};
