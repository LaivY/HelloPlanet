#include "monster.h"
#include "framework.h"
using namespace DirectX;

Monster::Monster() : m_id{}, m_type{}, m_aniType{}, m_position{}, m_velocity{}, m_yaw{}, m_boundingBox{}, m_hitTimer{}
{

}

void Monster::Update(FLOAT deltaTime)
{
	constexpr int TARGET_CLIENT_INDEX{ 0 };	// 몬스터가 쫓아갈 플레이어 인덱스
	constexpr float MOB_SPEED{ 50.0f };		// 1초당 움직일 거리

	XMVECTOR playerPos{ XMLoadFloat3(&g_networkFramework.clients[TARGET_CLIENT_INDEX].data.pos) };
	XMVECTOR mobPos{ XMLoadFloat3(&m_position) };
	XMVECTOR dir{ XMVector3Normalize(playerPos - mobPos) }; // 몬스터 -> 플레이어 방향 벡터

	// 몹 이동속도
	XMFLOAT3 velocity{};
	XMStoreFloat3(&velocity, dir * MOB_SPEED);

	// 몹 위치
	m_position.x += velocity.x * deltaTime;
	m_position.y += velocity.y * deltaTime;
	m_position.z += velocity.z * deltaTime;

	// 몹 속도
	// 클라이언트에서 속도는 로컬좌표계 기준이다.
	// 객체 기준 x값만큼 오른쪽으로, y값만큼 위로, z값만큼 앞으로 간다.
	// 몹은 플레이어를 향해 앞으로만 가므로 x, y는 0으로 바꾸고 z는 몹의 이동속도로 설정한다.
	m_velocity.x = 0.0f;
	m_velocity.y = 0.0f;
	m_velocity.z = MOB_SPEED;

	// 몹 각도
	XMVECTOR zAxis{ XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f) };			// +z축
	XMVECTOR radian{ XMVector3AngleBetweenNormals(zAxis, dir) };	// +z축과 몬스터 -> 플레이어 방향 벡터 간의 각도
	XMFLOAT3 angle{}; XMStoreFloat3(&angle, radian);
	angle.x = XMConvertToDegrees(angle.x);

	// x가 0보다 작다는 것은 반시계 방향으로 회전한 것
	XMFLOAT3 direction{}; XMStoreFloat3(&direction, dir);
	m_yaw = direction.x < 0 ? -angle.x : angle.x;

	// 몹 애니메이션
	m_aniType = eMobAnimationType::RUNNING;
}

void Monster::SetId(CHAR id)
{
	m_id = id;
}

void Monster::SetType(CHAR type)
{
	m_type = type;
}

void Monster::SetAnimationType(eMobAnimationType type)
{
	m_aniType = type;
}

void Monster::SetPosition(DirectX::XMFLOAT3 position)
{
	m_position = position;
}

void Monster::SetYaw(FLOAT yaw)
{
	m_yaw = yaw;
}

MonsterData Monster::GetData() const
{
	return MonsterData{ m_id, m_type, m_aniType, m_position, m_velocity, m_yaw };
}

DirectX::XMFLOAT3 Monster::GetPosition() const
{
	return m_position;
}

DirectX::BoundingOrientedBox Monster::GetBoundingBox()
{
	return m_boundingBox;
}
