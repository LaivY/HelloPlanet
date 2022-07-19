#include "stdafx.h"
#include "monster.h"
#include "framework.h"
using namespace DirectX;

Monster::Monster() : 
	m_id{}, m_mobType{}, m_aniType{ eMobAnimationType::IDLE }, m_position{}, m_velocity{}, m_yaw{},
	m_worldMatrix{}, m_boundingBox{}, m_hitTimer{}, m_atkTimer{}, m_target{},
	m_hp{}, m_damage{}, m_speed{}, m_knockbackTime{}, m_atkRange{}, m_atkAniFrame{}
{

}

void Monster::Update(FLOAT deltaTime) { }
void Monster::OnHit(const BulletData& bullet) { }

void Monster::SetId(CHAR id)
{
	m_id = id;
}

void Monster::SetType(eMobType type)
{
	m_mobType = type;
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
	constexpr DirectX::XMFLOAT3 mobSpawnAreas[]
	{ 
		{ 0.0f,	  0.0f, 400.0f }, { 0.0f,	 0.0f, -400.0f },	{ 400.0f,  0.0f, 0.0f },	{ -400.0f, 0.0f, 0.0f },
		{ 300.0f, 0.0f, 300.0f }, { -300.0f, 0.0f,  300.0f },	{ -300.0f, 0.0f, -300.0f }, {  300.0f, 0.0f, -300.0f }
	};
	const std::uniform_int_distribution<int> areasDistribution{ 0, static_cast<int>(std::size(mobSpawnAreas) - 1) };
	SetPosition(mobSpawnAreas[areasDistribution(g_randomEngine)]);
}

MonsterData Monster::GetData() const
{
	return MonsterData{ m_id, m_mobType, m_aniType, m_position, m_velocity, m_yaw };
}

CHAR Monster::GetId() const
{
	return m_id;
}

DirectX::XMFLOAT3 Monster::GetPosition() const
{
	return m_position;
}

INT Monster::GetHp() const
{
	return m_hp;
}

BoundingOrientedBox Monster::GetBoundingBox() const
{
	BoundingOrientedBox result{};
	m_boundingBox.Transform(result, XMLoadFloat4x4(&m_worldMatrix));
	return result;
}

UCHAR Monster::GetTargetId() const
{
	return m_target;
}

void Monster::UpdatePosition(FLOAT deltaTime, XMVECTOR& look)
{
	// 몹 속도
	// 클라이언트에서 몹 속도는 로컬좌표계 기준이다. 객체 기준 x값만큼 오른쪽으로, y값만큼 위로, z값만큼 앞으로 간다.
	// 이 밑에서 사용하는 velocity는 월드 좌표계 기준, m_velocity는 로컬 좌표계 기준이다.
	XMFLOAT3 velocity{};
	switch (m_aniType)
	{
	case eMobAnimationType::ATTACK:
	case eMobAnimationType::LEGATK:
		XMStoreFloat3(&velocity, look * 0);
		m_velocity = XMFLOAT3{ 0.0f, 0.0f, 0.0f };
		break;
	case eMobAnimationType::HIT:
	{
		// 피격당한 적은 잠깐 뒤로 밀리도록
		FLOAT value{ std::lerp(0.5f, 0.0f, m_hitTimer / m_knockbackTime) };
		XMStoreFloat3(&velocity, -look * value * m_speed);
		m_velocity = XMFLOAT3{ 0.0f, 0.0f, -m_speed * value };
		break;
	}
	default:
		// 보스는 오브젝트와 충돌하지 않음
		if (m_mobType == eMobType::ULIFO)
		{
			XMStoreFloat3(&velocity, look * m_speed);
			m_velocity = XMFLOAT3{ 0.0f, 0.0f, m_speed };
			break;
		}

		// 이동한 뒤 부딪힌다면 경로수정
		bool isCollision{ false };
		// 몬스터 바운딩박스의 모서리를 담을 배열
		XMFLOAT3 temp[8]{};
		// 로컬좌표인 바운딩박스를 월드좌표로 변환: GetBoundingBox()
		auto monsterBoundingBox{ GetBoundingBox() };
		monsterBoundingBox.GetCorners(temp);

		XMFLOAT3 corner[4]{};
		corner[0] = XMFLOAT3{ temp[0].x, 0.0f, temp[0].z };
		corner[1] = XMFLOAT3{ temp[1].x, 0.0f, temp[1].z };
		corner[2] = XMFLOAT3{ temp[5].x, 0.0f, temp[5].z };
		corner[3] = XMFLOAT3{ temp[4].x, 0.0f, temp[4].z };

		for (const auto& objectBoundingBox : g_networkFramework.hitboxes)
		{
			// 충돌 검사
			if (!monsterBoundingBox.Intersects(objectBoundingBox)) continue;
			// 충돌 되었다면...
			objectBoundingBox.GetCorners(temp);

			XMFLOAT3 oCorner[4]{};
			oCorner[0] = XMFLOAT3{ temp[0].x, 0.0f, temp[0].z };
			oCorner[1] = XMFLOAT3{ temp[1].x, 0.0f, temp[1].z };
			oCorner[2] = XMFLOAT3{ temp[5].x, 0.0f, temp[5].z };
			oCorner[3] = XMFLOAT3{ temp[4].x, 0.0f, temp[4].z };
			for (const auto& pc : corner)
			{
				if (!objectBoundingBox.Contains(XMLoadFloat3(&pc))) continue;
				float dis[4]{};
				dis[0] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&oCorner[0]), XMLoadFloat3(&oCorner[1]), XMLoadFloat3(&pc)));
				dis[1] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&oCorner[1]), XMLoadFloat3(&oCorner[2]), XMLoadFloat3(&pc)));
				dis[2] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&oCorner[2]), XMLoadFloat3(&oCorner[3]), XMLoadFloat3(&pc)));
				dis[3] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&oCorner[3]), XMLoadFloat3(&oCorner[0]), XMLoadFloat3(&pc)));

				float* minDis{std::ranges::min_element(dis) };
				XMFLOAT3 v{};
				if (*minDis == dis[0])
				{
					v = Vector3::Normalize(Vector3::Sub(oCorner[0], pc));
				}
				else if (*minDis == dis[1])
				{
					v = Vector3::Normalize(Vector3::Sub(oCorner[1], pc));
				}
				else if (*minDis == dis[2])
				{
					v = Vector3::Normalize(Vector3::Sub(oCorner[2], pc));
				}
				else if (*minDis == dis[3])
				{
					v = Vector3::Normalize(Vector3::Sub(oCorner[3], pc));
				}
				look = XMLoadFloat3(&v);
				XMStoreFloat3(&velocity, XMLoadFloat3(&v) * m_speed );
				isCollision = true;
			}
		}
		if (!isCollision) {
			// 피격당한게 아니라면 타겟 플레이어를 향해 이동
			XMStoreFloat3(&velocity, look * m_speed);
			m_velocity = XMFLOAT3{ 0.0f, 0.0f, m_speed };
			m_hitTimer = 0.0f;
			break;
		}
	}

	// 몹 이동
	m_position.x += velocity.x * deltaTime;
	m_position.y += velocity.y * deltaTime;
	m_position.z += velocity.z * deltaTime;
}

void Monster::UpdateRotation(const XMVECTOR& look)
{
	// 몹이 look 방향을 보도록 회전
	XMVECTOR zAxis{ XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f) };
	XMVECTOR radian{ XMVector3AngleBetweenNormals(zAxis, look) };
	FLOAT angle{ XMConvertToDegrees(XMVectorGetX(radian)) };

	// x가 0보다 작다는 것은 반시계 방향으로 회전한 것
	m_yaw = XMVectorGetX(look) < 0 ? -angle : angle;
}

void Monster::UpdateWorldMatrix(const XMVECTOR& look)
{
	XMVECTOR up{ XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };
	XMVECTOR right{ XMVector3Normalize(XMVector3Cross(up, look)) };

	// 월드변환행렬은 1열이 right, 2열이 up, 3열이 look, 4열이 위치. 모두 정규화 되어있어야함.
	XMMATRIX worldMatrix{ XMMatrixIdentity() };
	worldMatrix.r[0] = right;
	worldMatrix.r[1] = up;
	worldMatrix.r[2] = look;
	worldMatrix.r[3] = XMVectorSet(m_position.x, m_position.y, m_position.z, 1.0f);
	XMStoreFloat4x4(&m_worldMatrix, worldMatrix);
}

XMVECTOR Monster::GetPlayerVector(UCHAR playerId)
{
	XMVECTOR playerPos{ XMLoadFloat3(&g_networkFramework.clients[playerId].data.pos) };
	XMVECTOR mobPos{ XMLoadFloat3(&m_position) };
	return XMVector3Normalize(playerPos - mobPos);
}

GarooMonster::GarooMonster() : Monster{}
{
	m_mobType = eMobType::GAROO;
	m_hp = 150;
	m_damage = 20;
	m_speed = 70.0f;
	m_knockbackTime = 0.7f;
	m_atkAniFrame = 26;
	m_boundingBox = BoundingOrientedBox{ XMFLOAT3{ -0.5f, 11.5f, -1.0f }, XMFLOAT3{ 5.0f, 4.0f, 7.0f }, XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f } };
}

void GarooMonster::Update(FLOAT deltaTime)
{
	if (m_hp <= 0) return;

	// 타겟 유효성 검사
	UpdateTarget();

	// 몹 -> 플레이어 벡터
	XMVECTOR look{ GetPlayerVector(m_target) };

	// 플레이어 쪽으로 이동
	UpdatePosition(deltaTime, look);

	// 플레이어를 보도록 회전
	UpdateRotation(look);

	// 월드 행렬 최신화
	UpdateWorldMatrix(look);

	// 애니메이션 최신화
	UpdateAnimation(deltaTime);
	
	// 공격 계산
	CalcAttack();
}

void GarooMonster::OnHit(const BulletData& bullet)
{
	SetTargetId(bullet.playerId);
	SetHp(m_hp - bullet.damage);
	if (m_hp <= 0)
	{
		m_aniType = eMobAnimationType::DIE;
		g_networkFramework.killCount++;
		return;
	}
	if (m_aniType == eMobAnimationType::RUNNING)
		m_aniType = eMobAnimationType::HIT;
}

void GarooMonster::UpdateTarget()
{
	if (!g_networkFramework.clients[m_target].isAlive)
		m_target = g_networkFramework.DetectPlayer(GetPosition());
}

void GarooMonster::UpdateAnimation(FLOAT deltaTime)
{
	switch (m_aniType)
	{
	case eMobAnimationType::IDLE:
		m_aniType = eMobAnimationType::RUNNING;
		break;
	case eMobAnimationType::ATTACK:
		if (m_atkTimer > m_atkAniFrame * 1.0f / 30.0f)
		{
			m_aniType = eMobAnimationType::RUNNING;
			m_atkTimer = 0.0f;
			break;
		}
		m_atkTimer += deltaTime;
		break;
	case eMobAnimationType::HIT:
		if (m_hitTimer > m_knockbackTime)
		{
			m_aniType = eMobAnimationType::RUNNING;
			m_hitTimer = 0.0f;
			break;
		}
		m_hitTimer += deltaTime;
		break;
	}
}

void GarooMonster::CalcAttack()
{
	float range{ Vector3::Length(Vector3::Sub(g_networkFramework.clients[m_target].data.pos, m_position)) };
	if (range < 25.0f)
		m_aniType = eMobAnimationType::ATTACK;
}

SerpentMonster::SerpentMonster()
{
	m_mobType = eMobType::SERPENT;
	m_hp = 250;
	m_damage = 30;
	m_speed = 60.0f;
	m_knockbackTime = 0.5f;
	m_atkAniFrame = 25; // 원래 31인데 적당히 줄임
	m_boundingBox = BoundingOrientedBox{ XMFLOAT3{ 0.0f, 22.0f, 10.0f }, XMFLOAT3{ 9.0f, 22.0f, 10.0f }, XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f } };
}

void SerpentMonster::Update(FLOAT deltaTime)
{
	if (m_hp <= 0) return;
	UpdateTarget();
	XMVECTOR look{ GetPlayerVector(m_target) };
	UpdatePosition(deltaTime, look);
	UpdateRotation(look);
	UpdateWorldMatrix(look);
	UpdateAnimation(deltaTime);
	CalcAttack();
}

void SerpentMonster::OnHit(const BulletData& bullet)
{
	SetTargetId(bullet.playerId);
	SetHp(m_hp - bullet.damage);
	if (m_hp <= 0)
	{
		m_aniType = eMobAnimationType::DIE;
		g_networkFramework.killCount++;
		return;
	}
	if (m_aniType == eMobAnimationType::WALKING)
		m_aniType = eMobAnimationType::HIT;
}

void SerpentMonster::UpdateTarget()
{
	if (!g_networkFramework.clients[m_target].isAlive)
		m_target = g_networkFramework.DetectPlayer(GetPosition());
}

void SerpentMonster::UpdateAnimation(FLOAT deltaTime)
{
	switch (m_aniType)
	{
	case eMobAnimationType::IDLE:
		m_aniType = eMobAnimationType::WALKING;
		break;
	case eMobAnimationType::ATTACK:
		if (m_atkTimer >= m_atkAniFrame * 1.0f / 30.0f)
		{
			m_aniType = eMobAnimationType::WALKING;
			m_atkTimer = 0.0f;
			break;
		}
		m_atkTimer += deltaTime;
		break;
	case eMobAnimationType::HIT:
		if (m_hitTimer > m_knockbackTime)
		{
			m_aniType = eMobAnimationType::WALKING;
			m_hitTimer = 0.0f;
			break;
		}
		m_hitTimer += deltaTime;
		break;
	}
}

void SerpentMonster::CalcAttack()
{
	float range{ Vector3::Length(Vector3::Sub(g_networkFramework.clients[m_target].data.pos, m_position)) };
	if (range < 83.0f)
		m_aniType = eMobAnimationType::ATTACK;
}

HorrorMonster::HorrorMonster()
{
	m_mobType = eMobType::HORROR;
	m_hp = 350;
	m_damage = 50;
	m_speed = 50.0f;
	m_knockbackTime = 0.3f;
	m_atkAniFrame = 25;
	m_boundingBox = BoundingOrientedBox{ XMFLOAT3{ -3.0f, 26.0f, 5.0f }, XMFLOAT3{ 15.0f, 7.0f, 22.0f }, XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f } };
}

void HorrorMonster::Update(FLOAT deltaTime)
{
	if (m_hp <= 0) return;
	UpdateTarget();
	XMVECTOR look{ GetPlayerVector(m_target) };
	UpdatePosition(deltaTime, look);
	UpdateRotation(look);
	UpdateWorldMatrix(look);
	UpdateAnimation(deltaTime);
	CalcAttack();
}

void HorrorMonster::OnHit(const BulletData& bullet)
{
	SetTargetId(bullet.playerId);
	SetHp(m_hp - bullet.damage);
	if (m_hp <= 0)
	{
		m_aniType = eMobAnimationType::DIE;
		g_networkFramework.killCount++;
		return;
	}
	if (m_aniType == eMobAnimationType::WALKING)
		m_aniType = eMobAnimationType::HIT;
}

void HorrorMonster::UpdateTarget()
{
	if (!g_networkFramework.clients[m_target].isAlive)
		m_target = g_networkFramework.DetectPlayer(GetPosition());
}

void HorrorMonster::UpdateAnimation(FLOAT deltaTime)
{
	switch (m_aniType)
	{
	case eMobAnimationType::IDLE:
		m_aniType = eMobAnimationType::WALKING;
		break;
	case eMobAnimationType::ATTACK:
		if (m_atkTimer > m_atkAniFrame * 1.0f / 30.0f)
		{
			m_aniType = eMobAnimationType::WALKING;
			m_atkTimer = 0.0f;
			break;
		}
		m_atkTimer += deltaTime;
		break;
	case eMobAnimationType::HIT:
		if (m_hitTimer > m_knockbackTime)
		{
			m_aniType = eMobAnimationType::WALKING;
			m_hitTimer = 0.0f;
			break;
		}
		m_hitTimer += deltaTime;
		break;
	}
}

void HorrorMonster::CalcAttack()
{
	float range{ Vector3::Length(Vector3::Sub(g_networkFramework.clients[m_target].data.pos, m_position)) };
	if (range < 88.0f)
		m_aniType = eMobAnimationType::ATTACK;
}

UlifoMonster::UlifoMonster() : m_pattern{ ePattern::APPEAR }, m_order{ 0 }, m_timer{ 0.0f }, m_smashCoolDown{ 5.0f }, m_jumpAtkCoolDown{ 10.0f }
{
	m_mobType = eMobType::ULIFO;
	m_hp = 10000;
	m_damage = 70;
	m_speed = 40.0f;
	m_knockbackTime = 0.2f;
	m_atkAniFrame = 38;
	m_boundingBox = BoundingOrientedBox{ XMFLOAT3{ -1.0f, 125.0f, 0.0f }, XMFLOAT3{ 28.0f, 17.0f, 30.0f }, XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f } };
}

void UlifoMonster::Update(FLOAT deltaTime)
{
	if (m_hp <= 0) return;
	switch (m_pattern)
	{
	case ePattern::NONE:
	{
		static const std::uniform_int_distribution<int> dis{ 0, 2 };
		int pattern{ dis(g_randomEngine) };
		if (pattern == 0 && m_smashCoolDown <= 0.0f)
		{
			m_pattern = ePattern::SMASH;
			m_smashCoolDown = 5.0f;
		}
		else if (pattern == 1 && m_jumpAtkCoolDown <= 0.0f)
		{
			m_pattern = ePattern::JUMPATK;
			m_jumpAtkCoolDown = 10.0f;
		}
		break;
	}
	case ePattern::APPEAR:
		Appear(deltaTime);
		return;
	case ePattern::SMASH:
		Smash(deltaTime);
		return;
	case ePattern::JUMPATK:
		JumpAttack(deltaTime);
		return;
	}

	UpdateTarget();
	XMVECTOR look{ GetPlayerVector(m_target) };
	UpdatePosition(deltaTime, look);
	UpdateRotation(look);
	UpdateWorldMatrix(look);
	UpdateAnimation(deltaTime);
	CalcAttack();

	m_smashCoolDown = max(0.0f, m_smashCoolDown - deltaTime);
	m_jumpAtkCoolDown = max(0.0f, m_jumpAtkCoolDown - deltaTime);
}

void UlifoMonster::OnHit(const BulletData& bullet)
{
	//SetTargetId(bullet.playerId);
	SetHp(m_hp - bullet.damage);
	if (m_hp <= 0)
	{
		m_aniType = eMobAnimationType::DIE;
		g_networkFramework.killCount++;
		return;
	}
}

void UlifoMonster::UpdateTarget()
{
	if (!g_networkFramework.clients[m_target].isAlive)
		m_target = g_networkFramework.DetectPlayer(GetPosition());
}

void UlifoMonster::UpdateAnimation(FLOAT deltaTime)
{
	switch (m_aniType)
	{
	case eMobAnimationType::IDLE:
		m_aniType = eMobAnimationType::WALKING;
		break;
	case eMobAnimationType::LEGATK:
		if (m_atkTimer > m_atkAniFrame * 1.0f / 30.0f)
		{
			m_aniType = eMobAnimationType::WALKING;
			m_atkTimer = 0.0f;
			break;
		}
		m_atkTimer += deltaTime;
		break;
	case eMobAnimationType::HIT:
		if (m_hitTimer > m_knockbackTime)
		{
			m_aniType = eMobAnimationType::WALKING;
			m_hitTimer = 0.0f;
			break;
		}
		m_hitTimer += deltaTime;
		break;
	}
}

void UlifoMonster::CalcAttack()
{
	float range{ Vector3::Length(Vector3::Sub(g_networkFramework.clients[m_target].data.pos, m_position)) };
	if (range <= 10.0f)
	{
		m_pattern = ePattern::SMASH;
		m_order = 2;
		m_timer = 0.0f;
		m_smashCoolDown = max(3.0f, m_smashCoolDown);
	}
	else if (115.0f <= range && range <= 120.0f)
		m_aniType = eMobAnimationType::LEGATK;
}

void UlifoMonster::Appear(FLOAT deltaTime)
{
	/*
	0 : 하늘에서 idle 상태로 떨어져서 착지하면 down 애니메이션을 재생(0.6초)
	1 : standup 애니메이션 재생(0.6초)
	2 : roar 애니메이션 재생(1.0초)
	3 : 대기(2.0초)
	*/

	switch (m_order)
	{
	case 0:
		m_velocity = XMFLOAT3{ 0.0f, -1000.0f, 0.0f };
		m_position.y = max(0.0f, m_position.y - 1000.0f * deltaTime);
		if (m_position.y <= 500.0f)
		{
			if (m_position.y == 0.0f)
				m_velocity = XMFLOAT3{};
			if (m_timer >= 0.6f)
			{
				m_order = 1;
				m_timer = 0.0f;
				break;
			}
			SetAnimationType(eMobAnimationType::DOWN);
			m_timer += deltaTime;
		}
		break;
	case 1:
		if (m_timer >= 0.6f)
		{
			m_order = 2;
			m_timer = 0.0f;
			break;
		}
		SetAnimationType(eMobAnimationType::STANDUP);
		m_timer += deltaTime;
		break;
	case 2:
		if (m_timer >= 1.0f)
		{
			m_order = 3;
			m_timer = 0.0f;
			break;
		}
		SetAnimationType(eMobAnimationType::ROAR);
		m_timer += deltaTime;
		break;
	case 3:
		if (m_timer >= 2.0f)
		{
			m_pattern = ePattern::NONE;
			m_order = 0;
			m_timer = 0.0f;
			break;
		}
		SetAnimationType(eMobAnimationType::IDLE);
		m_timer += deltaTime;
		break;
	}
}

void UlifoMonster::Smash(FLOAT deltaTime)
{
	/*
	0. 랜덤으로 타겟 한 명을 지정함
	1. 타겟과의 거리가 5이하가 될 때까지 빠른 속도로 타겟에게 달려감. 
	2. 대기(1.0초)
	3. 내려찍음(0.5초)
	4. 일어남(0.5초)
	5. 대기(1.0초)
	*/
	
	switch (m_order)
	{
	case 0:
	{
		std::uniform_int_distribution<int> dis{ 0, MAX_USER - 1 };
		do
		{
			m_target = dis(g_randomEngine);
		} while (!g_networkFramework.clients[m_target].isAlive);
		m_order = 1;
		break;
	}
	case 1:
	{
		m_speed = 300.0f;
		auto look{ GetPlayerVector(m_target) };
		UpdatePosition(deltaTime, look);
		UpdateRotation(look);
		UpdateWorldMatrix(look);
		SetAnimationType(eMobAnimationType::WALKING);

		float range{ Vector3::Length(Vector3::Sub(g_networkFramework.clients[m_target].data.pos, m_position)) };
		if (range <= 5.0f)
		{
			m_order = 2;
			m_speed = 40.0f;
		}
		break;
	}
	case 2:
	{
		if (m_timer >= 0.5f)
		{
			m_order = 3;
			m_timer = 0.0f;
			break;
		}
		SetAnimationType(eMobAnimationType::IDLE);
		m_velocity = XMFLOAT3{};
		m_timer += deltaTime;
		break;
	}
	case 3:
	{
		static int count{ 0 };
		if (m_timer >= 30.0f * 1.0f / 30.0f)
		{
			m_order = 4;
			m_timer = 0.0f;
			count = 0;
			break;
		}
		if (count < 2)
		{
			SetAnimationType(eMobAnimationType::DOWN);
			++count;
		}
		else
			SetAnimationType(eMobAnimationType::NONE);
		m_velocity = XMFLOAT3{};
		m_timer += deltaTime;
		break;
	}
	case 4:
	{
		static int count{ 0 };
		if (m_timer >= 20.0f * 1.0f / 30.0f)
		{
			m_order = 5;
			m_timer = 0.0f;
			count = 0;
			break;
		}
		if (count < 2)
		{
			SetAnimationType(eMobAnimationType::STANDUP);
			++count;
		}
		else
			SetAnimationType(eMobAnimationType::NONE);
		m_velocity = XMFLOAT3{};
		m_timer += deltaTime;
		break;
	}
	case 5:
	{
		if (m_timer >= 1.0f)
		{
			m_pattern = ePattern::NONE;
			m_order = 0;
			m_timer = 0.0f;
			break;
		}
		SetAnimationType(eMobAnimationType::IDLE);
		m_velocity = XMFLOAT3{};
		m_timer += deltaTime;
		break;
	}
	}
}

void UlifoMonster::JumpAttack(FLOAT deltaTime)
{
	/*
	0. 랜덤으로 타겟 한 명을 지정
	1. 점프 준비
	2. 점프
	3. 대기(3.0초)
	4. 타겟에 낙하
	5. 대기(1.0초)
	*/

	static XMFLOAT3 targetPosition{};
	switch (m_order)
	{
	case 0:
	{
		std::uniform_int_distribution<int> dis{ 0, MAX_USER - 1 };
		do
		{
			m_target = dis(g_randomEngine);
			targetPosition = g_networkFramework.clients[m_target].data.pos;
		} while (!g_networkFramework.clients[m_target].isAlive);
		m_order = 1;
		break;
	}
	case 1:
		if (m_timer >= 1.0f)
		{
			m_order = 2;
			m_timer = 0.0f;
			break;
		}
		SetAnimationType(eMobAnimationType::DOWN);
		auto look{ GetPlayerVector(m_target) };
		UpdateRotation(look);
		UpdateWorldMatrix(look);
		m_timer += deltaTime;
		break;
	case 2:
	{
		if (m_timer >= 1.0f)
		{
			m_order = 3;
			m_timer = 0.0f;
			break;
		}
		SetAnimationType(eMobAnimationType::JUMPATK);
		if (m_timer >= 0.2f)
		{
			m_velocity = XMFLOAT3{ 0.0f, 3000.0f, 0.0f };
			m_position.y += 3000.0f * deltaTime;
		}
		auto look{ GetPlayerVector(m_target) };
		UpdateWorldMatrix(look);
		m_timer += deltaTime;
		break;
	}
	case 3:
		if (m_timer >= 1.0f)
		{
			m_order = 4;
			m_timer = 0.0f;
			m_position = targetPosition;
			m_position.y = 5000.0f;
			break;
		}

		// 클라이언트에서 안보이도록
		m_position = XMFLOAT3{ 0.0f, -999.0f, 0.0f };
		m_velocity = XMFLOAT3{};
		m_timer += deltaTime;
		break;
	case 4:
	{
		static int count{ 0 };

		m_velocity = XMFLOAT3{ 0.0f, -5000.0f, 0.0f };
		m_position.y = max(0.0f, m_position.y - 5000.0f * deltaTime);
		if (m_position.y <= 3000.0f)
		{
			if (m_position.y == 0.0f)
				m_velocity = XMFLOAT3{};
			if (m_timer >= 30.0f * 1.0f / 30.0f)
			{
				m_order = 5;
				m_timer = 0.0f;
				count = 0;
				break;
			}
			if (count < 2)
			{
				SetAnimationType(eMobAnimationType::DOWN);
				++count;
			}
			else
				SetAnimationType(eMobAnimationType::NONE);
			m_timer += deltaTime;
		}
		auto look{ GetPlayerVector(m_target) };
		UpdatePosition(deltaTime, look);
		break;
	}
	case 5:
	{
		static int count{ 0 };

		if (m_timer >= 20.0f * 1.0f / 30.0f)
		{
			m_order = 6;
			m_timer = 0.0f;
			count = 0;
			break;
		}
		if (count < 2)
		{
			SetAnimationType(eMobAnimationType::STANDUP);
			++count;
		}
		else
			SetAnimationType(eMobAnimationType::NONE);
		auto look{ GetPlayerVector(m_target) };
		UpdateWorldMatrix(look);
		m_timer += deltaTime;
		break;
	}
	case 6:
	{
		if (m_timer >= 1.0f)
		{
			m_pattern = ePattern::NONE;
			m_order = 0;
			m_timer = 0.0f;
			break;
		}
		SetAnimationType(eMobAnimationType::IDLE);
		auto look{ GetPlayerVector(m_target) };
		UpdateWorldMatrix(look);
		m_velocity = XMFLOAT3{};
		m_timer += deltaTime;
		break;
	}
	}
}