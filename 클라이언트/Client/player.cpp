#include "player.h"
#include "camera.h"

Player::Player() : GameObject{}, m_velocity{ 0.0f, 0.0f, 0.0f }, m_maxVelocity{ 10.0f }, m_friction{ 0.96f }
{
	m_type = GameObjectType::PLAYER;
}

void Player::Update(FLOAT deltaTime)
{
	// m_velocity의 길이가 0.171을 넘으면 블러

	Move(m_velocity);

	// 실내, 지형 밖으로 못나가게 설정
	SetPlayerInArea();

	// 플레이어가 지형 위에 있고 실내에 있는게 아니라면
	if (m_terrain && GetPosition().y < 300.0f)
	{
		// 플레이어가 지형 위를 움직이도록 설정
		SetPlayerOnTerrain();

		// 플레이어의 노말과 룩 벡터 설정
		SetPlayerNormalAndLook();
	}
	m_velocity = Vector3::Mul(m_velocity, m_friction);
}

void Player::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	// 회전각 제한
	if (m_pitch + pitch > MAX_PITCH)
		pitch = MAX_PITCH - m_pitch;
	else if (m_pitch + pitch < MIN_PITCH)
		pitch = MIN_PITCH - m_pitch;

	// 회전각 합산
	m_roll += roll; m_pitch += pitch; m_yaw += yaw;

	// 카메라는 x,y축으로 회전할 수 있다.
	// GameObject::Rotate에서 플레이어의 로컬 x,y,z축을 변경하므로 먼저 호출해야한다.
	m_camera->Rotate(0.0f, pitch, yaw);

	// 플레이어는 y축으로만 회전할 수 있다.
	GameObject::Rotate(0.0f, 0.0f, yaw);
}

void Player::SetPlayerInArea()
{
	XMFLOAT3 pos{ GetPosition() };

	// 실내 밖으로 못나가게
	if (GetPosition().y > 300.0f)
	{
		pos.x = clamp(pos.x, -14.0f, 14.0f);
		pos.z = clamp(pos.z, -14.0f, 14.0f);
		SetPosition(pos);
	}

	// 지형 밖으로 못나가게
	else if (m_terrain)
	{
		XMFLOAT3 tPos{ m_terrain->GetPosition() };
		XMFLOAT3 scale{ m_terrain->GetScale() };
		pos.x = clamp(pos.x, tPos.x, tPos.x + m_terrain->GetWidth() * scale.x);
		pos.z = clamp(pos.z, tPos.z, tPos.z + m_terrain->GetLength() * scale.z);
		SetPosition(pos);
	}
}

void Player::SetPlayerOnTerrain()
{
	/*
	1. 플레이어가 위치한 블록의 좌측하단 좌표를 구한다.
	2. 해당 좌표를 이용하여 블록 메쉬의 정점 25개를 구한다.
	3. 정점 25개를 이용하여 4차 베지어 곡면을 구한다.
	4. 플레이어의 위치 t를 구해서 베지어 곡면 상의 높이를 구한다.
	*/

	XMFLOAT3 pos{ GetPosition() };
	XMFLOAT3 LB{ m_terrain->GetBlockPosition(pos.x, pos.z) };
	int blockWidth{ m_terrain->GetBlockWidth() };
	int blockLength{ m_terrain->GetBlockLength() };
	XMFLOAT3 scale{ m_terrain->GetScale() };

	auto CubicBezierSum = [](const array<XMFLOAT3, 25>& patch, XMFLOAT2 t) {

		// 4차 베지어 곡선 계수
		array<float, 5> uB, vB;
		float txInv{ 1.0f - t.x };
		uB[0] = txInv * txInv * txInv * txInv;
		uB[1] = 4.0f * t.x * txInv * txInv * txInv;
		uB[2] = 6.0f * t.x * t.x * txInv * txInv;
		uB[3] = 4.0f * t.x * t.x * t.x * txInv;
		uB[4] = t.x * t.x * t.x * t.x;

		float tyInv{ 1.0f - t.y };
		vB[0] = tyInv * tyInv * tyInv * tyInv;
		vB[1] = 4.0f * t.y * tyInv * tyInv * tyInv;
		vB[2] = 6.0f * t.y * t.y * tyInv * tyInv;
		vB[3] = 4.0f * t.y * t.y * t.y * tyInv;
		vB[4] = t.y * t.y * t.y * t.y;

		// 4차 베지에 곡면 계산
		XMFLOAT3 sum{ 0.0f, 0.0f, 0.0f };
		for (int i = 0; i < 5; ++i)
		{
			XMFLOAT3 subSum{ 0.0f, 0.0f, 0.0f };
			for (int j = 0; j < 5; ++j)
			{
				XMFLOAT3 temp{ Vector3::Mul(patch[(i * 5) + j], uB[j]) };
				subSum = Vector3::Add(subSum, temp);
			}
			subSum = Vector3::Mul(subSum, vB[i]);
			sum = Vector3::Add(sum, subSum);
		}
		return sum;
	};

	// 베지에 평면 제어점 25개
	array<XMFLOAT3, 25> vertices;
	for (int i = 0, z = 4; z >= 0; --z)
		for (int x = 0; x < 5; ++x)
		{
			vertices[i].x = LB.x + (x * blockWidth / 4 * scale.x);
			vertices[i].z = LB.z + (z * blockLength / 4 * scale.z);
			vertices[i].y = m_terrain->GetHeight(vertices[i].x, vertices[i].z);
			++i;
		}

	// 플레이어의 위치 t
	XMFLOAT2 uv{ (pos.x - LB.x) / (blockWidth * scale.x), 1.0f - ((pos.z - LB.z) / (blockLength * scale.z)) };
	XMFLOAT3 posOnBazier{ CubicBezierSum(vertices, uv) };
	SetPosition(XMFLOAT3{ pos.x, posOnBazier.y, pos.z });
}

void Player::SetPlayerNormalAndLook()
{
	XMFLOAT3 pos{ GetPosition() };
	XMFLOAT3 up{ m_terrain->GetNormal(pos.x, pos.z) };

	// 지형 노멀을 y축으로 x, z축 계산
	if (float theta = acosf(Vector3::Dot(XMFLOAT3{ 0.0f, 1.0f, 0.0f }, up)))
	{
		XMFLOAT3 right{ Vector3::Normalize(Vector3::Cross(XMFLOAT3{ 0.0f, 1.0f, 0.0f }, up)) };
		XMFLOAT4X4 rotate; XMStoreFloat4x4(&rotate, XMMatrixRotationNormal(XMLoadFloat3(&right), theta));
		XMFLOAT3 look{ Vector3::TransformNormal(GetFront(), rotate) };
		right = Vector3::Normalize(Vector3::Cross(up, look));

		m_worldMatrix._11 = right.x;	m_worldMatrix._12 = right.y;	m_worldMatrix._13 = right.z;
		m_worldMatrix._21 = up.x;		m_worldMatrix._22 = up.y;		m_worldMatrix._23 = up.z;
		m_worldMatrix._31 = look.x;		m_worldMatrix._32 = look.y;		m_worldMatrix._33 = look.z;
	}
}

void Player::AddVelocity(const XMFLOAT3& increase)
{
	m_velocity = Vector3::Add(m_velocity, increase);

	// 최대 속도에 걸린다면 해당 비율로 축소시킴
	FLOAT length{ Vector3::Length(m_velocity) };
	if (length > m_maxVelocity)
	{
		FLOAT ratio{ m_maxVelocity / length };
		m_velocity = Vector3::Mul(m_velocity, ratio);
	}
}