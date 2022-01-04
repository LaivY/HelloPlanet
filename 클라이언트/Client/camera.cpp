#include "camera.h"

Camera::Camera() : m_eye{ 0.0f, 0.0f, 0.0f }, m_at{ 0.0f, 0.0f, 1.0f }, m_up{ 0.0f, 1.0f, 0.0f },
				   m_roll{ 0.0f }, m_pitch{ 0.0f }, m_yaw{ 0.0f }, m_terrain{ nullptr }, m_pcbCamera{ nullptr }
{
	XMStoreFloat4x4(&m_viewMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_projMatrix, XMMatrixIdentity());
}

Camera::~Camera()
{
	if (m_cbCamera) m_cbCamera->Unmap(0, NULL);
}

void Camera::Update(FLOAT deltaTime)
{
	// 카메라 뷰 변환 행렬 최신화
	XMStoreFloat4x4(&m_viewMatrix, XMMatrixLookAtLH(XMLoadFloat3(&m_eye), XMLoadFloat3(&Vector3::Add(m_eye, m_at)), XMLoadFloat3(&m_up)));
}

void Camera::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	ComPtr<ID3D12Resource> dummy;
	UINT cbCameraByteSize{ ((sizeof(cbCamera) + 255) & ~255) };
	m_cbCamera = CreateBufferResource(device, commandList, NULL, cbCameraByteSize, 1, D3D12_HEAP_TYPE_UPLOAD, {}, dummy);
	m_cbCamera->Map(0, NULL, reinterpret_cast<void**>(&m_pcbCamera));
}

void Camera::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// DirectX는 행우선(row-major), HLSL는 열우선(column-major)
	// 행렬이 셰이더로 넘어갈 때 자동으로 전치 행렬로 변환된다.
	// 그래서 셰이더에 전치 행렬을 넘겨주면 DirectX의 곱셈 순서와 동일하게 계산할 수 있다.
	m_pcbCamera->viewMatrix = Matrix::Transpose(m_viewMatrix);
	m_pcbCamera->projMatrix = Matrix::Transpose(m_projMatrix);
	m_pcbCamera->eye = GetEye();
	commandList->SetGraphicsRootConstantBufferView(1, m_cbCamera->GetGPUVirtualAddress());
}

void Camera::Move(const XMFLOAT3& shift)
{
	m_eye = Vector3::Add(m_eye, shift);
}

void Camera::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	XMMATRIX rotate{ XMMatrixIdentity() };
	XMVECTOR worldXAxis{ XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f) };
	XMVECTOR worldYAxis{ XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };
	XMVECTOR worldZAxis{ XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f) };

	if (roll != 0.0f)
	{
		rotate *= XMMatrixRotationAxis(worldZAxis, XMConvertToRadians(roll));
		m_roll += roll;
	}
	if (pitch != 0.0f)
	{
		// x축(pitch)의 경우 MIN_PITCH ~ MAX_PITCH
		if (m_pitch + pitch > MAX_PITCH)
		{
			rotate *= XMMatrixRotationAxis(worldXAxis, XMConvertToRadians(MAX_PITCH - m_pitch));
			m_pitch = MAX_PITCH;
		}
		else if (m_pitch + pitch < MIN_PITCH)
		{
			rotate *= XMMatrixRotationAxis(worldXAxis, XMConvertToRadians(MIN_PITCH - m_pitch));
			m_pitch = MIN_PITCH;
		}
		else
		{
			rotate *= XMMatrixRotationAxis(worldXAxis, XMConvertToRadians(pitch));
			m_pitch += pitch;
		}
	}
	if (yaw != 0.0f)
	{
		// y축(yaw)의 경우 제한 없음
		rotate *= XMMatrixRotationAxis(worldYAxis, XMConvertToRadians(yaw));
		m_yaw += yaw;
	}
	XMStoreFloat3(&m_at, XMVector3TransformNormal(XMLoadFloat3(&m_at), rotate));
}

void Camera::SetPlayer(const shared_ptr<Player>& player)
{
	if (m_player) m_player.reset();
	m_player = player;
	SetEye(m_player->GetPosition());
}

// --------------------------------------

ThirdPersonCamera::ThirdPersonCamera() : Camera{}, m_distance{ 5.0f }, m_delay{ 0.01f }
{
	m_offset = Vector3::Normalize(XMFLOAT3{ 0.0f, 1.0f, -5.0f });
}

void ThirdPersonCamera::Update(FLOAT deltaTime)
{
	// 플레이어 위치 + 오프셋 위치로 이동
	XMFLOAT3 destination{ Vector3::Add(m_player->GetPosition(), Vector3::Mul(m_offset, m_distance)) };
	XMFLOAT3 direction{ Vector3::Sub(destination, m_eye) };
	XMFLOAT3 shift{ Vector3::Mul(direction, fmax((1.0f - m_delay) * deltaTime * 10.0f, 0.01f)) };
	SetEye(Vector3::Add(m_eye, shift));

	// 카메라가 지형 밑으로 내려가지 않도록함
	if (m_terrain)
	{
		FLOAT height{ m_terrain->GetHeight(m_eye.x, m_eye.z) };
		if (m_eye.y < height + 0.5f)
			SetEye(XMFLOAT3{ m_eye.x, height + 0.5f, m_eye.z });
	}

	// 카메라 뷰 변환 행렬 최신화
	XMStoreFloat4x4(&m_viewMatrix, XMMatrixLookAtLH(XMLoadFloat3(&m_eye), XMLoadFloat3(&Vector3::Add(m_eye, m_at)), XMLoadFloat3(&m_up)));
}

void ThirdPersonCamera::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	XMMATRIX rotate{ XMMatrixIdentity() };
	XMVECTOR worldYAxis{ XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };

	if (roll != 0.0f)
	{
		rotate *= XMMatrixRotationAxis(XMLoadFloat3(&m_player->GetFront()), XMConvertToRadians(roll));
		m_roll += roll;
	}
	if (pitch != 0.0f)
	{
		rotate *= XMMatrixRotationAxis(XMLoadFloat3(&m_player->GetLocalXAxis()), XMConvertToRadians(pitch));
		m_pitch += pitch;
	}
	if (yaw != 0.0f)
	{
		rotate *= XMMatrixRotationAxis(worldYAxis, XMConvertToRadians(yaw));
		m_yaw += yaw;
	}
	XMStoreFloat3(&m_offset, XMVector3TransformNormal(XMLoadFloat3(&m_offset), rotate));

	// 항상 플레이어를 바라보도록 설정
	XMFLOAT3 look{ Vector3::Sub(m_player->GetPosition(), m_eye) };
	if (Vector3::Length(look))
		m_at = look;
}