#include "object.h"
#include "camera.h"
#include "scene.h"

GameObject::GameObject() : m_roll{}, m_pitch{}, m_yaw{}, m_scale{ 1.0f, 1.0f, 1.0f }, m_velocity{}, m_textureInfo{ nullptr }, m_animationInfo{ nullptr }, m_isDeleted{ FALSE }
{
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
}

void GameObject::OnAnimation(FLOAT currFrame, UINT endFrame, BOOL isUpper)
{

}

void GameObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
	//if (!m_mesh || (!m_shader && !shader)) return;

	// 셰이더 변수 최신화
	UpdateShaderVariable(commandList);

	// PSO 설정
	if (shader) commandList->SetPipelineState(shader->GetPipelineState().Get());
	else if (m_shader) commandList->SetPipelineState(m_shader->GetPipelineState().Get());

	// 메쉬 렌더링
	if (m_mesh) m_mesh->Render(commandList);

#ifdef RENDER_HITBOX
	for (const auto& bb : m_hitboxes)
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

#ifdef RENDER_HITBOX
	for (auto& hitbox : m_hitboxes)
		hitbox->Update(deltaTime);
#endif
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

void GameObject::SetScale(const XMFLOAT3& scale)
{
	XMFLOAT4X4 scaleMatrix{};
	XMStoreFloat4x4(&scaleMatrix, XMMatrixScaling(scale.x / m_scale.x, scale.y / m_scale.y, scale.z / m_scale.z));

	XMFLOAT3 position{ GetPosition() };
	SetPosition(XMFLOAT3{ 0.0f, 0.0f, 0.0f });
	m_worldMatrix = Matrix::Mul(m_worldMatrix, scaleMatrix);
	SetPosition(position);

	m_scale = scale;
}

void GameObject::SetMesh(const shared_ptr<Mesh>& mesh)
{
	m_mesh = mesh;
}

void GameObject::SetShader(const shared_ptr<Shader>& shader)
{
	m_shader = shader;
}

void GameObject::SetShadowShader(const shared_ptr<Shader>& shadowShader)
{
	m_shadowShader = shadowShader;
}

void GameObject::SetOutlineShader(const shared_ptr<Shader>& outlineShader)
{
	m_outlineShader = outlineShader;
}

void GameObject::SetTexture(const shared_ptr<Texture>& texture)
{
	m_texture = texture;
}

void GameObject::SetTextureInfo(unique_ptr<TextureInfo>& textureInfo)
{
	m_textureInfo = move(textureInfo);
}

void GameObject::AddHitbox(unique_ptr<Hitbox>& hitbox)
{
	m_hitboxes.push_back(move(hitbox));
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

void GameObject::RenderOutline(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	if (!m_outlineShader) return;

	XMFLOAT4X4 worldMatrix{ m_worldMatrix };
	XMFLOAT4X4 scale{};
	XMStoreFloat4x4(&scale, XMMatrixScaling(1.02f, 1.02f, 1.02f));
	m_worldMatrix = Matrix::Mul(scale, m_worldMatrix);
	Render(commandList, m_outlineShader);
	m_worldMatrix = worldMatrix;
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
		{
			if (m_animationInfo->currAnimationName == "DIE")
			{
				m_isDeleted = TRUE;
				return;
			}
			PlayAnimation(m_animationInfo->currAnimationName);
		}
	}
	else if (m_animationInfo->state == eAnimationState::BLENDING) // 블렌딩 진행 중
	{
		if (currFrame >= endFrame)
			PlayAnimation(m_animationInfo->afterAnimationName);
	}
}

void Monster::ApplyServerData(const MonsterData& monsterData)
{
	if (m_animationInfo->currAnimationName == "DIE" && m_animationInfo->afterAnimationName != "DIE")
		return;

	switch (monsterData.aniType)
	{
	case eMobAnimationType::IDLE:
		if (m_animationInfo->currAnimationName != "IDLE" && m_animationInfo->afterAnimationName != "IDLE")
			PlayAnimation("IDLE", TRUE);
		break;
	case eMobAnimationType::RUNNING:
		if (m_animationInfo->currAnimationName != "RUNNING" && m_animationInfo->afterAnimationName != "RUNNING")
			PlayAnimation("RUNNING", TRUE);
		break;
	case eMobAnimationType::ATTACK:
		if (m_animationInfo->currAnimationName != "ATTACK" && m_animationInfo->afterAnimationName != "ATTACK")
			PlayAnimation("ATTACK", TRUE);
		break;
	case eMobAnimationType::HIT:
		if (m_animationInfo->currAnimationName != "HIT" && m_animationInfo->afterAnimationName != "HIT")
			PlayAnimation("HIT", TRUE);
		break;
	case eMobAnimationType::DIE:
		if (m_animationInfo->currAnimationName != "DIE" && m_animationInfo->afterAnimationName != "DIE")
			PlayAnimation("DIE", TRUE);
		SetVelocity(XMFLOAT3{ 0.0f, -3.0f, 0.0f });
		return;
	}

	m_id = monsterData.id;
	SetPosition(monsterData.pos);
	SetVelocity(monsterData.velocity);
	Rotate(0.0f, 0.0f, monsterData.yaw - m_yaw);
}

Hitbox::Hitbox(const XMFLOAT3& center, const XMFLOAT3& extents, const XMFLOAT3& rollPitchYaw) : m_owner{ nullptr }, m_center{ center }, m_extents{ extents }, m_rollPitchYaw{ rollPitchYaw }
{
	m_hitbox = make_unique<GameObject>();
	m_hitbox->SetMesh(Scene::s_meshes["CUBE"]);
	m_hitbox->SetShader(Scene::s_shaders["WIREFRAME"]);
}

void Hitbox::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	m_hitbox->Render(commandList);
}

void Hitbox::Update(FLOAT /*deltaTime*/)
{
	if (!m_owner) return;

	XMFLOAT4X4 worldMatrix{ Matrix::Identity() };
	XMFLOAT4X4 scaleMatrix{}, rotateMatrix{},transMatrix{};
	XMStoreFloat4x4(&scaleMatrix, XMMatrixScaling(m_extents.x * 2.0f, m_extents.y * 2.0f, m_extents.z * 2.0f));
	XMStoreFloat4x4(&rotateMatrix, XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_rollPitchYaw.y), XMConvertToRadians(m_rollPitchYaw.z), XMConvertToRadians(m_rollPitchYaw.x)));
	XMStoreFloat4x4(&transMatrix, XMMatrixTranslation(m_center.x, m_center.y, m_center.z));

	worldMatrix = Matrix::Mul(worldMatrix, scaleMatrix);
	worldMatrix = Matrix::Mul(worldMatrix, rotateMatrix);
	worldMatrix = Matrix::Mul(worldMatrix, transMatrix);
	worldMatrix = Matrix::Mul(worldMatrix, m_owner->GetWorldMatrix());
	m_hitbox->SetWorldMatrix(move(worldMatrix));
}

void Hitbox::SetOwner(GameObject* gameObject)
{
	m_owner = gameObject;
}

BoundingOrientedBox Hitbox::GetBoundingBox() const
{
	BoundingOrientedBox result{ XMFLOAT3{}, m_extents, XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f } };

	XMFLOAT4X4 worldMatrix{ Matrix::Identity() };
	XMFLOAT4X4 rotateMatrix{}, transMatrix{};
	XMStoreFloat4x4(&rotateMatrix, XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_rollPitchYaw.y), XMConvertToRadians(m_rollPitchYaw.z), XMConvertToRadians(m_rollPitchYaw.x)));
	XMStoreFloat4x4(&transMatrix, XMMatrixTranslation(m_center.x, m_center.y, m_center.z));

	worldMatrix = Matrix::Mul(worldMatrix, rotateMatrix);
	worldMatrix = Matrix::Mul(worldMatrix, transMatrix);
	worldMatrix = Matrix::Mul(worldMatrix, m_owner->GetWorldMatrix());
	result.Transform(result, XMLoadFloat4x4(&worldMatrix));
	return result;
}
