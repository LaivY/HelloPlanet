#pragma once
#include "stdafx.h"

class Monster abstract
{
public:
	Monster();
	virtual ~Monster() = default;

	virtual void Update(FLOAT deltaTime);
	virtual void OnHit(const BulletData& bullet);

	void SetId(CHAR id);
	void SetType(eMobType type);
	void SetAnimationType(eMobAnimationType type);
	void SetPosition(const DirectX::XMFLOAT3& position);
	void SetYaw(FLOAT yaw);
	void SetHp(INT hp);
	void SetTargetId(UCHAR id);
	void SetRandomPosition();

	MonsterData GetData() const;
	CHAR GetId() const;
	DirectX::XMFLOAT3 GetPosition() const;
	INT GetHp() const;
	DirectX::BoundingOrientedBox GetBoundingBox() const;
	UCHAR GetTargetId() const;
	
protected:
	void UpdatePosition(FLOAT deltaTime, DirectX::XMVECTOR& look);
	void UpdateRotation(const DirectX::XMVECTOR& look);
	void UpdateWorldMatrix(const DirectX::XMVECTOR& look);
	DirectX::XMVECTOR GetPlayerVector(UCHAR playerId);

protected:
	// struct MonsterData 멤버 변수들
	CHAR							m_id;
	eMobType						m_mobType;
	eMobAnimationType				m_aniType;
	DirectX::XMFLOAT3				m_position;
	DirectX::XMFLOAT3				m_velocity;
	FLOAT							m_yaw;

	// 몬스터 스탯 변수들
	INT								m_hp;			// 체력
	INT								m_damage;		// 데미지
	FLOAT							m_speed;		// 이동속도
	FLOAT							m_knockbackTime;// 피격 시 뒤로 밀리는 시간

	// 서버 계산에 필요한 변수들
	DirectX::XMFLOAT4X4				m_worldMatrix;	// 월드변환행렬
	DirectX::BoundingOrientedBox	m_boundingBox;	// 바운딩박스
	FLOAT							m_hitTimer;		// 피격당한 시점부터 시작되는 타이머
	FLOAT							m_atkTimer;		// 공격한 시점부터 시작되는 타이머
	UCHAR							m_target;		// 공격 대상 플레이어 id
	bool							m_wasAttack;	// 공격 애니메이션 때 공격했는지
};

class GarooMonster : public Monster
{
public:
	GarooMonster();
	~GarooMonster() = default;

	virtual void Update(FLOAT deltaTime);
	virtual void OnHit(const BulletData& bullet);

private:
	void UpdateAnimation();
	void CalcAttack();
};

class SerpentMonster : public Monster
{
public:
	SerpentMonster();
	~SerpentMonster() = default;

	virtual void Update(FLOAT deltaTime);
	virtual void OnHit(const BulletData& bullet);

private:
	void UpdateAnimation();
	void CalcAttack();
};

class HorrorMonster : public Monster
{
public:
	HorrorMonster();
	~HorrorMonster() = default;

	virtual void Update(FLOAT deltaTime);
	virtual void OnHit(const BulletData& bullet);

private:
	void UpdateAnimation();
	void CalcAttack();
};

class UlifoMonster : public Monster
{
public:
	UlifoMonster();
	~UlifoMonster() = default;

	virtual void Update(FLOAT deltaTime);
	virtual void OnHit(const BulletData& bullet);

private:
	void UpdateAnimation();
	void CalcAttack();

	void UpdateAppear(FLOAT deltaTime);

private:
	BOOL m_isAppear; // 등장 연출
};