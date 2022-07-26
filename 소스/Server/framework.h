#pragma once
#include <d3d12.h>
#include <wrl/client.h>

#include "session.h"
#include "monster.h"
#include "database.h"

constexpr FLOAT g_spawnCooldown = 2.0f;


struct BulletDataFrame
{
	BulletData	data;
	BOOL		isBulletCast;		// 총알 송신 여부
	BOOL		isCollisionCheck;	// 충돌체크 여부
};

class NetworkFramework
{
public:
	NetworkFramework();
	~NetworkFramework() = default;
	
	int OnInit();
	void WorkThreads();
	void ProcessRecvPacket(const int id, char* p);

	void SendLoginOkPacket(const Session& player) const;
	void SendSelectWeaponPacket(const Session& player) const;
	void SendReadyCheckPacket(const Session& player) const;
	void SendChangeScenePacket(const eSceneType sceneType) const;
	void SendPlayerDataPacket();
	void SendBulletDataPacket();
	void SendBulletHitPacket();
	void SendMonsterDataPacket();
	void SendMonsterAttackPacket(const int id, const int mobId, const int damage) const;
	void SendRoundResultPacket(const eRoundResult result) const;
	void SendPacket2AllPlayer(void* packet, int bufSize) const;

	void Reset();
	void Update(FLOAT deltaTime);
	void SpawnMonsters(FLOAT deltaTime);
	void CollisionCheck();
	void RoundClearCheck();
	void GameOverCheck();

	void Disconnect(const int id);

	UCHAR DetectPlayer(const DirectX::XMFLOAT3& pos) const;
	CHAR GetNewId() const;

	void LoadMapObjects(const std::string& mapFile);

public:
	// 통신 관련 변수
	BOOL										isAccept;			// 1명이라도 서버에 들어왔는지
	INT											readyCount;			// 준비한 인원 수
	INT											disconnectCount;	// 연결 끊긴 인원 수
	std::vector<std::thread>					worker_threads;		// 쓰레드

	// 게임 관련 변수
	BOOL										isGameOver;			// 게임오버 여부
	BOOL										doSpawnMonster;		// 몬스터 생성 여부

	INT											round;				// 현재 라운드(1 ~ 4)
	const INT									roundGoal[4];		// 라운드 별 목표 처치 수(0 ~ 3)
	INT											roundMobCount[4];	// 라운드 별 생성할 남은 몬스터 수(0 ~ 3)
	FLOAT										spawnCooldown;		// 스폰 쿨다운
	CHAR										lastMobId;			// 다음 몬스터에 부여할 ID
	INT											killCount;			// 처치한 몬스터 수

	std::array<Session, MAX_USER>				clients;			// 클라이언트
	std::vector<BulletDataFrame>				bullets;			// 총알
	std::vector<BulletHitData>					bulletHits;			// 총알을 맞춘 정보
	std::vector<std::unique_ptr<Monster>>		monsters;			// 몬스터
	std::vector<DirectX::BoundingOrientedBox>	hitboxes;			// 맵 오브젝트 히트박스
	std::mutex									g_mutex{};
};