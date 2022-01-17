#include "object.h"
#include "camera.h"

GameObject::GameObject() : m_roll{ 0.0f }, m_pitch{ 0.0f }, m_yaw{ 0.0f }, m_textureInfo{ nullptr }, m_animationInfo{ nullptr }, m_isDeleted{ FALSE }
{
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
}

void GameObject::OnAnimation(const string& animationName, FLOAT currFrame, UINT endFrame)
{
	if (currFrame >= endFrame)
	{
		PlayAnimation(animationName);
		//if (animationName == "AIMING")
		//	PlayAnimation("RELOAD");
		//else if (animationName == "RELOAD")
		//	PlayAnimation("RUN");
		//else if (animationName == "RUN")
		//	PlayAnimation("AIMING");
	}
}

void GameObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
	// 셰이더 변수 최신화
	UpdateShaderVariable(commandList);

	// PSO 설정
	if (shader) commandList->SetPipelineState(shader->GetPipelineState().Get());
	else if (m_shader) commandList->SetPipelineState(m_shader->GetPipelineState().Get());

	// 메쉬 렌더링
	if (m_mesh)
	{
		if (m_animationInfo)
			m_mesh->Render(commandList, this);
		else
			m_mesh->Render(commandList);
	}
}

void GameObject::Update(FLOAT deltaTime)
{
	if (m_texture && m_textureInfo)
	{
		// 타이머 진행
		m_textureInfo->timer += deltaTime;

		// 프레임 증가
		if (m_textureInfo->timer > m_textureInfo->interver)
		{
			m_textureInfo->frame += static_cast<int>(m_textureInfo->timer / m_textureInfo->interver);
			m_textureInfo->timer = fmod(m_textureInfo->timer, m_textureInfo->interver);
		}

		// 텍스쳐 애니메이션이 마지막일 경우
		if (m_textureInfo->frame >= m_texture->GetTextureCount())
		{
			if (m_textureInfo->doRepeat)
				m_textureInfo->frame = 0;
			else
			{
				m_textureInfo->frame = m_texture->GetTextureCount() - 1;
				m_isDeleted = true;
			}
		}
	}

	// 타이머 진행
	if (m_animationInfo) m_animationInfo->timer += deltaTime;
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

void GameObject::PlayAnimation(const string& animationName)
{
	if (!m_animationInfo) m_animationInfo = make_unique<AnimationInfo>();
	m_animationInfo->animationName = animationName;
	m_animationInfo->timer = 0.0f;
}