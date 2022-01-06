#include "object.h"
#include "camera.h"

GameObject::GameObject() : m_type{ GameObjectType::DEFAULT }, m_isDeleted{ false }, m_localXAxis{ 1.0f, 0.0f, 0.0f }, m_localYAxis{ 0.0f, 1.0f, 0.0f }, m_localZAxis{ 0.0f, 0.0f, 1.0f },
						   m_roll{ 0.0f }, m_pitch{ 0.0f }, m_yaw{ 0.0f }, m_textureInfo{ nullptr }
{
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
}

void GameObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader) const
{
	// 셰이더 변수 최신화
	UpdateShaderVariable(commandList);

	// PSO 설정
	if (shader) commandList->SetPipelineState(shader->GetPipelineState().Get());
	else if (m_shader) commandList->SetPipelineState(m_shader->GetPipelineState().Get());

	// 메쉬 렌더링
	if (m_mesh) m_mesh->Render(commandList);
}

void GameObject::Update(FLOAT deltaTime)
{
	if (!m_texture || !m_textureInfo)
		return;

	m_textureInfo->frameTimer += deltaTime;
	if (m_textureInfo->frameTimer > m_textureInfo->frameInterver)
	{
		m_textureInfo->frame += static_cast<int>(m_textureInfo->frameTimer / m_textureInfo->frameInterver);
		m_textureInfo->frameTimer = fmod(m_textureInfo->frameTimer, m_textureInfo->frameInterver);
	}

	if (m_textureInfo->frame >= m_texture->GetTextureCount())
	{
		if (m_textureInfo->doRepeat) m_textureInfo->frame = 0;
		else
		{
			m_textureInfo->frame = m_texture->GetTextureCount() - 1;
			m_isDeleted = true;
		}
	}
}

void GameObject::Move(const XMFLOAT3& shift)
{
	SetPosition(Vector3::Add(GetPosition(), shift));
}

void GameObject::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	// 회전
	XMMATRIX rotate{ XMMatrixRotationRollPitchYaw(XMConvertToRadians(pitch), XMConvertToRadians(yaw), XMConvertToRadians(roll)) };
	XMMATRIX worldMatrix{ rotate * XMLoadFloat4x4(&m_worldMatrix) };
	XMStoreFloat4x4(&m_worldMatrix, worldMatrix);

	// 로컬 x,y,z축 최신화
	XMStoreFloat3(&m_localXAxis, XMVector3TransformNormal(XMLoadFloat3(&m_localXAxis), rotate));
	XMStoreFloat3(&m_localYAxis, XMVector3TransformNormal(XMLoadFloat3(&m_localYAxis), rotate));
	XMStoreFloat3(&m_localZAxis, XMVector3TransformNormal(XMLoadFloat3(&m_localZAxis), rotate));
}

void GameObject::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	// 게임오브젝트의 월드 변환 행렬 최신화
	commandList->SetGraphicsRoot32BitConstants(0, 16, &Matrix::Transpose(m_worldMatrix), 0);

	// 텍스쳐 최신화
	if (m_texture) m_texture->UpdateShaderVariable(commandList, m_textureInfo ? m_textureInfo->frame : 0);
}

void GameObject::SetPosition(const XMFLOAT3& position)
{
	m_worldMatrix._41 = position.x;
	m_worldMatrix._42 = position.y;
	m_worldMatrix._43 = position.z;
}

void GameObject::SetMesh(const shared_ptr<Mesh>& mesh)
{
	if (m_mesh) m_mesh.reset();
	m_mesh = mesh;
}

void GameObject::SetShader(const shared_ptr<Shader>& shader)
{
	if (m_shader) m_shader.reset();
	m_shader = shader;
}

void GameObject::SetTexture(const shared_ptr<Texture>& texture)
{
	if (m_texture) m_texture.reset();
	m_texture = texture;
}

void GameObject::SetTextureInfo(unique_ptr<TextureInfo>& textureInfo)
{
	m_textureInfo = move(textureInfo);
}

XMFLOAT3 GameObject::GetPosition() const
{
	return XMFLOAT3{ m_worldMatrix._41, m_worldMatrix._42, m_worldMatrix._43 };
}

XMFLOAT3 GameObject::GetNormal() const
{
	return XMFLOAT3{ m_worldMatrix._21, m_worldMatrix._22, m_worldMatrix._23 };
}

XMFLOAT3 GameObject::GetLook() const
{
	return XMFLOAT3{ m_worldMatrix._31, m_worldMatrix._32, m_worldMatrix._33 };
}