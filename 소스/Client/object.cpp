#include "object.h"
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

GameObject::GameObject() : m_roll{ 0.0f }, m_pitch{ 0.0f }, m_yaw{ 0.0f }, m_velocity{ 0.0f, 0.0f, 0.0f }, m_textureInfo{ nullptr }, m_animationInfo{ nullptr }, m_isDeleted{ FALSE }
{
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
}

void GameObject::OnAnimation(FLOAT currFrame, UINT endFrame, BOOL isUpper)
{
	//if (currFrame >= endFrame)
	//	PlayAnimation(m_animationInfo->currAnimationName);
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
	for (const auto& bb : m_boundingBoxes)
		bb->Render(commandList);
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
				m_textureInfo->frame = static_cast<int>(m_texture->GetTextureCount() - 1);
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

	// 이동
	Move(Vector3::Mul(GetRight(), m_velocity.x * deltaTime));
	Move(Vector3::Mul(GetUp(), m_velocity.y * deltaTime));
	Move(Vector3::Mul(GetLook(), m_velocity.z * deltaTime));
}

void GameObject::Move(const XMFLOAT3& shift)
{
	SetPosition(Vector3::Add(GetPosition(), shift));
}

void GameObject::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	// 회전각 합산
	m_roll += roll; m_pitch += pitch; m_yaw += yaw;

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

void GameObject::SetShadowShader(const shared_ptr<Shader>& sShader, const shared_ptr<Shader>& mShader, const shared_ptr<Shader>& lShader, const shared_ptr<Shader>& allShader)
{
	m_shadowShaders[0] = sShader;
	m_shadowShaders[1] = mShader;
	m_shadowShaders[2] = lShader;
	m_shadowShaders[3] = allShader;
}

void GameObject::SetTexture(const shared_ptr<Texture>& texture)
{
	m_texture = texture;
}

void GameObject::SetTextureInfo(unique_ptr<TextureInfo>& textureInfo)
{
	m_textureInfo = move(textureInfo);
}

void GameObject::AddBoundingBox(const SharedBoundingBox& boundingBox)
{
	m_boundingBoxes.push_back(boundingBox);
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
		m_animationInfo->currAnimationName = animationName;
		m_animationInfo->currTimer = 0.0f;
		m_animationInfo->afterAnimationName.clear();
		m_animationInfo->afterTimer = 0.0f;
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

void Monster::OnAnimation(FLOAT currFrame, UINT endFrame, BOOL isUpper)
{
	// 오류
	if (isUpper) return;
	if (m_animationInfo->state == eAnimationState::PLAY)
	{
		if (currFrame >= endFrame)
			PlayAnimation(m_animationInfo->currAnimationName);
	}
	else if (m_animationInfo->state == eAnimationState::BLENDING) // 블렌딩 진행 중
	{
		if (currFrame >= endFrame)
			PlayAnimation(m_animationInfo->afterAnimationName);
	}
}

void Monster::ApplyServerData(const MonsterData& monsterData)
{
	switch (monsterData.aniType)
	{
	case eMobAnimationType::IDLE:
		if (m_animationInfo->currAnimationName != "IDLE" && m_animationInfo->afterAnimationName != "IDLE")
			PlayAnimation("IDLE", TRUE);
		break;
	case eMobAnimationType::ACK:
		if (m_animationInfo->currAnimationName != "ATTACK" && m_animationInfo->afterAnimationName != "ATTACK")
			PlayAnimation("ATTACK", TRUE);
		break;
	case eMobAnimationType::RUNNING:
		if (m_animationInfo->currAnimationName != "RUNNING" && m_animationInfo->afterAnimationName != "RUNNING")
			PlayAnimation("RUNNING", TRUE);
		break;
	case eMobAnimationType::DIE:
		if (m_animationInfo->currAnimationName != "DIE" && m_animationInfo->afterAnimationName != "DIE")
			PlayAnimation("DIE", TRUE);
		break;
	}

	m_id = monsterData.id;
	SetPosition(monsterData.pos);
	SetVelocity(monsterData.velocity);
	Rotate(0.0f, 0.0f, monsterData.yaw - m_yaw);
}

UIObject::UIObject(FLOAT width, FLOAT height) : m_pivot{ eUIPivot::CENTER }, m_width{ width }, m_height{ height }
{
	m_worldMatrix._11 = width;
	m_worldMatrix._22 = height;
}

void UIObject::SetPosition(const XMFLOAT3& position)
{
	SetPosition(position.x, position.y);
}

void UIObject::SetPosition(FLOAT x, FLOAT y)
{
	switch (m_pivot)
	{
	case eUIPivot::LEFTTOP:
		m_worldMatrix._41 = x + m_width / 2.0f;
		m_worldMatrix._42 = y - m_height / 2.0f;
		break;
	case eUIPivot::CENTERTOP:
		m_worldMatrix._42 = y - m_height / 2.0f;
		break;
	case eUIPivot::RIGHTTOP:
		m_worldMatrix._41 = x - m_width / 2.0f;
		m_worldMatrix._42 = y - m_height / 2.0f;
		break;
	case eUIPivot::LEFTCENTER:
		m_worldMatrix._41 = x + m_width / 2.0f;
		break;
	case eUIPivot::CENTER:
		m_worldMatrix._41 = x;
		m_worldMatrix._42 = y;
		break;
	case eUIPivot::RIGHTCENTER:
		m_worldMatrix._41 = x - m_width / 2.0f;
		break;
	case eUIPivot::LEFTBOT:
		m_worldMatrix._41 = x + m_width / 2.0f;
		m_worldMatrix._42 = y + m_height / 2.0f;
		break;
	case eUIPivot::CENTERBOT:
		m_worldMatrix._42 = y + m_height / 2.0f;
		break;
	case eUIPivot::RIGHTBOT:
		m_worldMatrix._41 = x - m_width / 2.0f;
		m_worldMatrix._42 = y + m_height / 2.0f;
		break;
	}
}

void UIObject::SetWidth(FLOAT width)
{
	m_width = width;
	m_worldMatrix._11 = width;

	FLOAT deltaWidth{ width - m_width };
	switch (m_pivot)
	{
	case eUIPivot::LEFTTOP:
	case eUIPivot::LEFTCENTER:
	case eUIPivot::LEFTBOT:
		m_worldMatrix._41 += deltaWidth / 2.0f;
		break;
	case eUIPivot::RIGHTTOP:
	case eUIPivot::RIGHTCENTER:
	case eUIPivot::RIGHTBOT:
		m_worldMatrix._41 -= deltaWidth / 2.0f;
		break;
	}
}

void UIObject::SetHeight(FLOAT height)
{
	m_height = height;
	m_worldMatrix._22 = height;

	FLOAT deltaHeight{ height - m_height };
	switch (m_pivot)
	{
	case eUIPivot::LEFTTOP:
	case eUIPivot::RIGHTTOP:
	case eUIPivot::CENTERTOP:
		m_worldMatrix._42 += deltaHeight / 2.0f;
		break;
	case eUIPivot::LEFTBOT:
	case eUIPivot::CENTERBOT:
	case eUIPivot::RIGHTBOT:
		m_worldMatrix._42 -= deltaHeight / 2.0f;
		break;
	}
}