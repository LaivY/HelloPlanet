#pragma once
#include "stdafx.h"
#include "protocol.h"

class Monster
{
public:
	Monster();
	~Monster() = default;

	void OnHit(const BulletData& bullet);
	void Attack(const int id);
	void Update(FLOAT deltaTime);

	void SetId(CHAR id);
	void SetType(CHAR type);
	void SetAnimationType(eMobAnimationType type);
	void SetPosition(const DirectX::XMFLOAT3& position);
	void SetYaw(FLOAT yaw);
	void SetHp(INT hp);
	void SetTargetId(UCHAR id);
	void SetRandomPosition();

	MonsterData GetData() const;
	DirectX::XMFLOAT3 GetPosition() const;
	DirectX::BoundingOrientedBox GetBoundingBox() const;
	INT GetHp() const;
	CHAR GetId() const;
	UCHAR GetTargetId() const;
	eMobAnimationType GetAnimationType() const;
	FLOAT GetAtkTimer() const;
	
private:
	// 클라이언트로 보낼 데이터들
	CHAR							m_id;
	CHAR							m_type;
	eMobAnimationType				m_aniType;
	DirectX::XMFLOAT3				m_position;
	DirectX::XMFLOAT3				m_velocity;
	FLOAT							m_yaw;

	// 서버에서 갖고있어야할 데이터들
	DirectX::XMFLOAT4X4				m_worldMatrix;	// 월드변환행렬
	DirectX::BoundingOrientedBox	m_boundingBox;	// 바운딩박스
	FLOAT							m_hitTimer;		// 피격당한 시점부터 시작되는 타이머
	FLOAT							m_atkTimer;		// 공격한 시점부터 시작되는 타이머
	INT								m_hp;			// 체력
	UCHAR							m_target;		// 타겟
	bool							m_wasAttack;	// 
};