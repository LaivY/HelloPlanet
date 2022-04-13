#pragma once
#include "stdafx.h"
#include "protocol.h"

class Monster
{
public:
	Monster();
	~Monster() = default;

	void Update(FLOAT deltaTime);

	void SetId(CHAR id);
	void SetType(CHAR type);
	void SetAnimationType(eMobAnimationType type);
	void SetPosition(DirectX::XMFLOAT3 position);
	void SetYaw(FLOAT yaw);

	MonsterData GetData() const;
	DirectX::XMFLOAT3 GetPosition() const;
	DirectX::BoundingOrientedBox GetBoundingBox();

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
};