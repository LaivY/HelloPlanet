﻿#pragma once
#include "session.h"
#include "monster.h"
#include "database.h"

//constexpr INT stage1Goal = 10;
constexpr FLOAT g_spawnCooldown = 2.0f;

class NetworkFramework
{
public:
	NetworkFramework();
	~NetworkFramework() = default;
	
	int OnInit();
	void AcceptThread(SOCKET socket);
	void WorkThreads();
	void ProcessRecvPacket(const int id, char* p);

	void SendLoginOkPacket(const Session& player) const;
	void SendSelectWeaponPacket(const Session& player) const;
	void SendReadyCheckPacket(const Session& player) const;
	void SendChangeScenePacket(const eSceneType sceneType) const;
	void SendPlayerDataPacket();
	void SendBulletHitPacket();
	void SendMonsterDataPacket();
	void SendMonsterAttackPacket(const int id, const int mobId, const int damage) const;
	void SendRoundResultPacket(const eRoundResult result) const;
	void SendPacket2AllPlayer(void* packet, int bufSize) const;

	void Update(FLOAT deltaTime);
	void SpawnMonsters(FLOAT deltaTime);
	void CollisionCheck();
	void Disconnect(const int id);

	UCHAR DetectPlayer(const DirectX::XMFLOAT3& pos) const;
	CHAR GetNewId() const;

public:
	// 통신 관련 변수
	BOOL									isAccept;		// 1명이라도 서버에 들어왔는지
	INT										readyCount;		// 레디한 인원
	std::vector<std::thread>				worker_threads;	// 쓰레드

	// 게임 관련 변수
	INT										round;			// 현재 라운드(1 ~ 4)
	const INT								roundGoal[4];	// 라운드 별 목표 처치 수(0 ~ 3)
	//BOOL									isClearStage1;
	std::array<Session, MAX_USER>			clients;		// 클라이언트
	std::vector<BulletData>					bullets;		// 총알
	std::vector<BulletHitData>				bulletHits;		// 총알을 맞춘 정보
	std::vector<std::unique_ptr<Monster>>	monsters;		// 몬스터
	BOOL									doSpawnMonster;	// 몬스터 생성 여부
	FLOAT									spawnCooldown;	// 스폰 쿨다운
	CHAR									lastMobId;		// 다음 몬스터에 부여할 ID
	INT										killCount;		// 처치한 몬스터 수

	// 사용되지 않는 변수들
	std::vector<std::thread>				threads;
	BOOL									isInGame;
};
