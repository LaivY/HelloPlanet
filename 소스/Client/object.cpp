﻿#include "object.h"
#include "camera.h"

DebugBoundingBox::DebugBoundingBox(const XMFLOAT3& center, const XMFLOAT3& extents, const XMFLOAT4& orientation) : BoundingOrientedBox{ center, extents, orientation }, m_mesh{ nullptr }, m_shader{ nullptr }
{

}

void DebugBoundingBox::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	if (!m_mesh || !m_shader) return;
	commandList->SetPipelineState(m_shader->GetPipelineState().Get());
	m_mesh->UpdateShaderVariable(commandList, nullptr);
	m_mesh->Render(commandList);
}

void DebugBoundingBox::SetMesh(const shared_ptr<Mesh>& mesh)
{
	m_mesh = mesh;
}

void DebugBoundingBox::SetShader(const shared_ptr<Shader>& shader)
{
	m_shader = shader;
}

GameObject::GameObject() : m_roll{ 0.0f }, m_pitch{ 0.0f }, m_yaw{ 0.0f }, m_textureInfo{ nullptr }, m_animationInfo{ nullptr }, m_isDeleted{ FALSE }
{
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
}

void GameObject::OnAnimation(FLOAT currFrame, UINT endFrame, BOOL isUpper)
{
	if (currFrame >= endFrame)
		PlayAnimation(m_animationInfo->currAnimationName);
}

void GameObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
	if (!m_mesh || (!m_shader && !shader)) return;

	// 셰이더 변수 최신화
	UpdateShaderVariable(commandList);

	// PSO 설정
	if (shader) commandList->SetPipelineState(shader->GetPipelineState().Get());
	else if (m_shader) commandList->SetPipelineState(m_shader->GetPipelineState().Get());

	// 메쉬 렌더링
	m_mesh->Render(commandList);

#ifdef BOUNDINGBOX
	if (m_boundingBox) m_boundingBox->Render(commandList);
#endif
}

void GameObject::Update(FLOAT deltaTime)
{
	// 텍스쳐 타이머 진행
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

	// 애니메이션 타이머 진행
	if (m_animationInfo)
	{
		if (m_animationInfo->state == eAnimationState::PLAY)
			m_animationInfo->currTimer += deltaTime;
		else if (m_animationInfo->state == eAnimationState::BLENDING)
			m_animationInfo->blendingTimer += deltaTime;
	}

	// 바운딩 박스 정렬
	//if (m_boundingBox) m_boundingBox->Center = GetPosition();
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

void GameObject::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 게임오브젝트 월드 변환 행렬 최신화
	commandList->SetGraphicsRoot32BitConstants(0, 16, &Matrix::Transpose(m_worldMatrix), 0);

	// 메쉬 셰이더 변수 최신화
	if (m_mesh) m_mesh->UpdateShaderVariable(commandList, this);

	// 텍스쳐 셰이더 변수 최신화
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
	m_mesh = mesh;
}

void GameObject::SetShader(const shared_ptr<Shader>& shader)
{
	m_shader = shader;
}

void GameObject::SetTexture(const shared_ptr<Texture>& texture)
{
	m_texture = texture;
}

void GameObject::SetTextureInfo(unique_ptr<TextureInfo>& textureInfo)
{
	m_textureInfo = move(textureInfo);
}

void GameObject::SetBoundingBox(const shared_ptr<DebugBoundingBox>& boundingBox)
{
	m_boundingBox = boundingBox;
}

void GameObject::PlayAnimation(const string& animationName, BOOL doBlending)
{
	if (!m_animationInfo) m_animationInfo = make_unique<AnimationInfo>();
	if (doBlending)
	{
		m_animationInfo->afterAnimationName = animationName;
		m_animationInfo->afterTimer = 0.0f;
		m_animationInfo->state = eAnimationState::BLENDING;
	}
	else
	{
		m_animationInfo->afterAnimationName.clear();
		m_animationInfo->afterTimer = 0.0f;
		m_animationInfo->currAnimationName = animationName;
		m_animationInfo->currTimer = 0.0f;
		m_animationInfo->state = eAnimationState::PLAY;
	}
	m_animationInfo->blendingTimer = 0.0f;
}

void Skybox::Update(FLOAT deltaTime)
{
	if (!m_camera) return;
	SetPosition(m_camera->GetEye());
}

Bullet::Bullet(const XMFLOAT3& direction, FLOAT speed, FLOAT lifeTime) : m_direction{ Vector3::Normalize(direction) }, m_speed{ speed }, m_lifeTime{ lifeTime }, m_lifeTimer{}
{
	XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
	XMFLOAT3 right{ Vector3::Normalize(Vector3::Cross(up, direction)) };
	up = Vector3::Normalize(Vector3::Cross(direction, right));
	
	m_worldMatrix._11 = right.x;		m_worldMatrix._12 = right.y;		m_worldMatrix._13 = right.z;
	m_worldMatrix._21 = up.x;			m_worldMatrix._22 = up.y;			m_worldMatrix._23 = up.z;
	m_worldMatrix._31 = direction.x;	m_worldMatrix._32 = direction.y;	m_worldMatrix._33 = direction.z;
}

void Bullet::Update(FLOAT deltaTime)
{
	Move(Vector3::Mul(m_direction, m_speed * deltaTime));
	
	m_lifeTimer += deltaTime;
	if (m_lifeTimer > m_lifeTime)
		m_isDeleted = TRUE;
}

UIObject::UIObject() : m_pivot{ eUIPivot::CENTER }
{

}

void UIObject::SetPosition(const XMFLOAT3& position)
{
	switch (m_pivot)
	{
	case eUIPivot::CENTER:
		GameObject::SetPosition(position);
		break;
	}
}

void UIObject::SetPivot(eUIPivot pivot)
{
	m_pivot = pivot;
}