#include "stdafx.h"
#include "monster.h"
#include "framework.h"
using namespace DirectX;

//constexpr int	TARGET_CLIENT_INDEX{ 0 };	// 몬스터가 쫓아갈 플레이어 인덱스
constexpr float MOB_SPEED{ 50.0f };			// 1초당 움직일 거리
constexpr float HIT_PERIOD{ 0.7f };			// 맞으면 넉백당하는 시간

Monster::Monster() : m_id{}, m_type{}, m_aniType{}, m_position{}, m_velocity{}, m_yaw{}, m_worldMatrix{}, m_hitTimer{}, m_atkTimer{}, m_hp{}, m_target{}, m_wasAttack{}
{
	m_boundingBox = BoundingOrientedBox{ XMFLOAT3{ 0.0f, 8.0f, 0.0f }, XMFLOAT3{ 7.0f, 7.0f, 10.0f }, XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f } };
}

void Monster::OnHit(const BulletData& bullet)
{
	// 총알의 주인을 타겟으로 한다
	SetTargetId(bullet.playerId);
	// DIE로 바꿔서 보내주면 클라가 판단해서 지워줄 예정, 40은 임시 총알 데미지
	SetHp(m_hp - 40);
	if (m_hp <= 0) 
	{
		m_aniType = eMobAnimationType::DIE;
		g_networkFramework.m_killScore++;
		std::cout << "[Score] " << g_networkFramework.m_killScore << " / " << stage1Goal << std::endl;
	}
	else if (m_aniType == eMobAnimationType::RUNNING)
		m_aniType = eMobAnimationType::HIT;
}

void Monster::Attack(const int id)
{
	if (m_aniType == eMobAnimationType::RUNNING)
		m_aniType = eMobAnimationType::ATTACK;
	//g_networkFramework.clients[id].data.hp -= 10;
}

void Monster::Update(FLOAT deltaTime)
{
	if (m_hp <= 0) return;

	XMVECTOR playerPos{ XMLoadFloat3(&g_networkFramework.clients[GetTargetId()].data.pos) };
	XMVECTOR mobPos{ XMLoadFloat3(&m_position) };
	XMVECTOR dir{ XMVector3Normalize(playerPos - mobPos) }; // 몬스터 -> 플레이어 방향 벡터

	// 몹 월드좌표계 속도
	XMFLOAT3 velocity{};
	if (m_aniType == eMobAnimationType::HIT)
	{
		// 피격당한 적은 잠깐 뒤로 밀리도록
		FLOAT value{ std::lerp(0.5f, 0.0f, m_hitTimer / HIT_PERIOD) };
		XMStoreFloat3(&velocity, -dir * MOB_SPEED * value);

		// 몹 로컬좌표계 속도
		// 클라이언트에서 속도는 로컬좌표계 기준이다.
		// 객체 기준 x값만큼 오른쪽으로, y값만큼 위로, z값만큼 앞으로 간다.
		// 몹은 플레이어를 향해 앞으로만 가므로 x, y는 0으로 바꾸고 z는 몹의 이동속도로 설정한다.
		m_velocity.x = 0.0f;
		m_velocity.y = 0.0f;
		m_velocity.z = -MOB_SPEED * value;

		m_hitTimer += deltaTime;
		m_hitTimer = min(m_hitTimer, HIT_PERIOD);
	}
	else if(m_aniType == eMobAnimationType::ATTACK)
	{
		XMStoreFloat3(&velocity, dir * 0);
		m_velocity.x = 0.0f;
		m_velocity.y = 0.0f;
		m_velocity.z = 0.0f;

		m_atkTimer += deltaTime;
		m_atkTimer = min(m_atkTimer, HIT_PERIOD);
	}
	else
	{
		// 피격당한게 아니라면 타겟 플레이어를 향해 이동
		XMStoreFloat3(&velocity, dir * MOB_SPEED);

		m_velocity.x = 0.0f;
		m_velocity.y = 0.0f;
		m_velocity.z = MOB_SPEED;

		m_hitTimer = 0.0f;
	}

	// 몹 위치
	m_position.x += velocity.x * deltaTime;
	m_position.y += velocity.y * deltaTime;
	m_position.z += velocity.z * deltaTime;

	// 몹 각도
	XMVECTOR zAxis{ XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f) };			// +z축
	XMVECTOR radian{ XMVector3AngleBetweenNormals(zAxis, dir) };	// +z축과 몬스터 -> 플레이어 방향 벡터 간의 각도
	XMFLOAT3 angle{}; XMStoreFloat3(&angle, radian);
	angle.x = XMConvertToDegrees(angle.x);

	// x가 0보다 작다는 것은 반시계 방향으로 회전한 것
	XMFLOAT3 direction{}; XMStoreFloat3(&direction, dir);
	m_yaw = direction.x < 0 ? -angle.x : angle.x;

	// 몹 애니메이션
	if (m_aniType == eMobAnimationType::HIT && m_hitTimer < HIT_PERIOD)
		m_aniType = eMobAnimationType::HIT;
	else if (m_aniType == eMobAnimationType::ATTACK && m_atkTimer < HIT_PERIOD)
		m_aniType = eMobAnimationType::ATTACK;
	else {
		m_aniType = eMobAnimationType::RUNNING;
		m_atkTimer = 0.0f;
		m_wasAttack = false;
	}
	// --------------------------------------------------

	XMVECTOR look{ dir };
	XMVECTOR up{ XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };
	XMVECTOR right{ XMVector3Normalize(XMVector3Cross(up, look)) };

	// 월드변환행렬은 1열이 right, 2열이 up, 3열이 look, 4열이 위치. 모두 정규화 되어있어야함.
	XMMATRIX worldMatrix{ XMMatrixIdentity() };
	worldMatrix.r[0] = right;
	worldMatrix.r[1] = up;
	worldMatrix.r[2] = look;
	worldMatrix.r[3] = XMVectorSet(m_position.x, m_position.y, m_position.z, 1.0f);
	XMStoreFloat4x4(&m_worldMatrix, worldMatrix);


	if (GetAnimationType() == eMobAnimationType::ATTACK)
	{
		if (GetAtkTimer() >= 0.3f && m_wasAttack == false)
		{
			float range{ Vector3::Length(Vector3::Sub(g_networkFramework.clients[m_target].data.pos, GetPosition())) };
			if (range < 27.0f)
			{
				// 0.3f, 27.0f, 10은 임의의 상수, 추후 수정필요
				g_networkFramework.SendMonsterAttackPacket(m_target, 10);
				//std::cout << static_cast<int>(m_target) << " is under attack!" << std::endl;
			}
			//else std::cout << static_cast<int>(m_target) << " is dodge!" << std::endl;
			m_wasAttack = true;
		}
	}
	else 
	{
		float range{ Vector3::Length(Vector3::Sub(g_networkFramework.clients[m_target].data.pos, GetPosition())) };
		if (range < 25.0f) m_aniType = eMobAnimationType::ATTACK;
	}
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

void Monster::SetPosition(const DirectX::XMFLOAT3& position)
{
	m_position = position;
}

void Monster::SetYaw(FLOAT yaw)
{
	m_yaw = yaw;
}

void Monster::SetHp(INT hp)
{
	m_hp = hp;
}

void Monster::SetTargetId(UCHAR id)
{
	m_target = id;
}

void Monster::SetRandomPosition()
{
	std::random_device rd;
	const std::uniform_int_distribution<int> areasDistribution(0, 7);
	std::mt19937 generator(rd());
	const DirectX::XMFLOAT3 mobSpawnAreas[8]{ {0.0f, 0.0f, 400.0f},{0.0f, 0.0f, -400.0f},{400.0f, 0.0f, 0.0f},{-400.0f, 0.0f, 0.0f},
		{300.0f, 0.0f, 300.0f}, {-300.0f, 0.0f, 300.0f}, {-300.0f, 0.0f, -300.0f}, {300.0f, 0.0f, -300.0f} };
	SetPosition(mobSpawnAreas[areasDistribution(generator)]);
}




MonsterData Monster::GetData() const
{
	return MonsterData{ m_id, m_type, m_aniType, m_position, m_velocity, m_yaw };
}

DirectX::XMFLOAT3 Monster::GetPosition() const
{
	return m_position;
}

BoundingOrientedBox Monster::GetBoundingBox() const
{
	BoundingOrientedBox result{};
	m_boundingBox.Transform(result, XMLoadFloat4x4(&m_worldMatrix));
	return result;
}

INT Monster::GetHp() const
{
	return m_hp;
}

CHAR Monster::GetId() const
{
	return m_id;
}

UCHAR Monster::GetTargetId() const
{
	return m_target;
}

eMobAnimationType Monster::GetAnimationType() const
{
	return m_aniType;
}

FLOAT Monster::GetAtkTimer() const
{
	return m_atkTimer;
}