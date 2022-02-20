#include "player.h"
#include "camera.h"

Player::Player() : GameObject{}, m_velocity{ 0.0f, 0.0f, 0.0f }, m_maxVelocity{ 10.0f }, m_friction{ 0.96f }, m_camera{ nullptr }, m_gunMesh{ nullptr }, m_gunShader{ nullptr }
{
	
}

void Player::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
	GameObject::Render(commandList, shader);
	
	if (shader) commandList->SetPipelineState(shader->GetPipelineState().Get());
	else if (m_gunShader) commandList->SetPipelineState(m_gunShader->GetPipelineState().Get());

	if (m_gunMesh) m_gunMesh->Render(commandList, this);
}

void Player::Update(FLOAT deltaTime)
{
	GameObject::Update(deltaTime);

	Move(m_velocity);

	// 마찰력
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