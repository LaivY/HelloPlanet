#include "stdafx.h"
#include "object.h"
#include "audioEngine.h"
#include "camera.h"
#include "framework.h"
#include "mesh.h"
#include "player.h"
#include "scene.h"
#include "gameScene.h"
#include "shader.h"
#include "texture.h"

GameObject::GameObject() : m_isValid{ TRUE }, m_isMakeOutline{ FALSE }, m_roll{}, m_pitch{}, m_yaw{}, m_scale{ 1.0f, 1.0f, 1.0f }, m_velocity{}, m_textureInfo{ nullptr }, m_animationInfo{ nullptr }
{
	XMStoreFloat4x4(&m_worldMatrix, XMMatrixIdentity());
}

void GameObject::OnMouseEvent(HWND hWnd, FLOAT deltaTime) { }
void GameObject::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { }
void GameObject::OnKeyboardEvent(FLOAT deltaTime) { }
void GameObject::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { }
void GameObject::OnAnimation(FLOAT currFrame, UINT endFrame) { }
void GameObject::OnUpperAnimation(FLOAT currFrame, UINT endFrame) { }

void GameObject::Update(FLOAT deltaTime)
{
	// 텍스쳐 타이머 진행
	if (m_texture && m_textureInfo)
	{
		// 프레임 증가
		if (m_textureInfo->timer > m_textureInfo->interver)
		{
			m_textureInfo->frame += static_cast<int>(m_textureInfo->timer / m_textureInfo->interver);
			m_textureInfo->timer = fmod(m_textureInfo->timer, m_textureInfo->interver);
		}

		// 텍스쳐 애니메이션이 마지막일 경우
		if (m_textureInfo->frame >= m_texture->GetCount())
		{
			if (m_textureInfo->loop)
				m_textureInfo->frame = 0;
			else
			{
				m_textureInfo->frame = static_cast<int>(m_texture->GetCount() - 1);
				m_isValid = FALSE;
			}
		}

		// 타이머 진행
		m_textureInfo->timer += deltaTime;
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

void GameObject::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 게임오브젝트 월드 변환 행렬 최신화
	commandList->SetGraphicsRoot32BitConstants(0, 16, &Matrix::Transpose(m_worldMatrix), 0);

	// 메쉬 셰이더 변수 최신화
	if (m_mesh) m_mesh->UpdateShaderVariable(commandList, this);

	// 텍스쳐 셰이더 변수 최신화
	if (m_texture) m_texture->UpdateShaderVariable(commandList, m_textureInfo ? m_textureInfo->frame : 0);
}

void GameObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
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

void GameObject::Delete()
{
	m_isValid = FALSE;
}

void GameObject::SetOutline(BOOL isMakeOutline)
{
	m_isMakeOutline = isMakeOutline;
}

void GameObject::SetWorldMatrix(const XMFLOAT4X4& worldMatrix)
{
	m_worldMatrix = worldMatrix;
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

void GameObject::SetVelocity(const XMFLOAT3& velocity)
{
	m_velocity = velocity;
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

BOOL GameObject::isValid() const
{
	return m_isValid;
}

BOOL GameObject::isMakeOutline() const
{
	return m_isMakeOutline;
}

XMFLOAT4X4 GameObject::GetWorldMatrix() const
{
	return m_worldMatrix;
}

XMFLOAT3 GameObject::GetRollPitchYaw() const
{
	return XMFLOAT3{ m_roll, m_pitch, m_yaw };
}

XMFLOAT3 GameObject::GetRight() const
{
	return XMFLOAT3{ m_worldMatrix._11, m_worldMatrix._12, m_worldMatrix._13 };
}

XMFLOAT3 GameObject::GetUp() const
{
	return XMFLOAT3{ m_worldMatrix._21, m_worldMatrix._22, m_worldMatrix._23 };
}

XMFLOAT3 GameObject::GetLook() const
{
	return XMFLOAT3{ m_worldMatrix._31, m_worldMatrix._32, m_worldMatrix._33 };
}

XMFLOAT3 GameObject::GetPosition() const
{
	return XMFLOAT3{ m_worldMatrix._41, m_worldMatrix._42, m_worldMatrix._43 };
}

XMFLOAT3 GameObject::GetScale() const
{
	return m_scale;
}

XMFLOAT3 GameObject::GetVelocity() const
{
	return m_velocity;
}

const vector<unique_ptr<Hitbox>>& GameObject::GetHitboxes() const
{
	return m_hitboxes;
}

shared_ptr<Shader> GameObject::GetShadowShader() const
{
	return m_shadowShader;
}

AnimationInfo* GameObject::GetAnimationInfo() const
{
	return m_animationInfo.get();
}

AnimationInfo* GameObject::GetUpperAnimationInfo() const
{
	return m_upperAnimationInfo.get();
}

Skybox::Skybox()
{
	m_mesh = Scene::s_meshes["SKYBOX"];
	m_shader = Scene::s_shaders["SKYBOX"];
	m_texture = Scene::s_textures["SKYBOX"];
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
		Delete();
}

Monster::Monster(INT id, eMobType type) : m_id{ id }, m_type{ type }, m_damage{}, m_atkFrame{}, m_atkRange{}, m_isAttacked{ FALSE }
{
	SetShader(Scene::s_shaders["ANIMATION"]);
	SetShadowShader(Scene::s_shaders["SHADOW_ANIMATION"]);

	switch (type)
	{
	case eMobType::GAROO:
	{
		SetMesh(Scene::s_meshes["GAROO"]);
		SetTexture(Scene::s_textures["GAROO"]);
		auto collisionBox{ make_unique<Hitbox>(XMFLOAT3{ -0.5f, 20.0f, -1.0f }, XMFLOAT3{ 5.0f, 20.0f, 7.0f }) };
		collisionBox->SetOwner(this);
		AddHitbox(collisionBox);
#ifdef RENDER_HITBOX
		auto hitbox{ make_unique<Hitbox>(XMFLOAT3{ -0.5f, 11.5f, -1.0f }, XMFLOAT3{ 5.0f, 4.0f, 7.0f }) };
		hitbox->SetOwner(this);
		AddHitbox(hitbox);
#endif
		m_damage = 20;
		m_atkFrame = 7.0f;
		m_atkRange = 27.0f;
		break;
	}
	case eMobType::SERPENT:
	{
		SetMesh(Scene::s_meshes["SERPENT"]);
		SetTexture(Scene::s_textures["SERPENT"]);
		auto collisionBox{ make_unique<Hitbox>(XMFLOAT3{ 0.0f, 22.0f, 10.0f }, XMFLOAT3{ 9.0f, 22.0f, 10.0f }) };
		collisionBox->SetOwner(this);
		AddHitbox(collisionBox);
		m_damage = 30;
		m_atkFrame = 10.0f;
		m_atkRange = 85.0f;
		break;
	}
	case eMobType::HORROR:
	{
		SetMesh(Scene::s_meshes["HORROR"]);
		SetTexture(Scene::s_textures["HORROR"]);
		auto collisionBox{ make_unique<Hitbox>(XMFLOAT3{ -3.0f, 20.0f, 5.0f }, XMFLOAT3{ 15.0f, 20.0f, 22.0f }) };
		collisionBox->SetOwner(this);
		AddHitbox(collisionBox);
#ifdef RENDER_HITBOX
		auto hitbox{ make_unique<Hitbox>(XMFLOAT3{ -3.0f, 26.0f, 5.0f }, XMFLOAT3{ 15.0f, 7.0f, 22.0f }) };
		hitbox->SetOwner(this);
		AddHitbox(hitbox);
#endif
		m_damage = 50;
		m_atkFrame = 11.0f;
		m_atkRange = 90.0f;
		break;
	}
	case eMobType::ULIFO:
	{
		SetMesh(Scene::s_meshes["ULIFO"]);
		SetTexture(Scene::s_textures["ULIFO"]);
#ifdef RENDER_HITBOX
		auto hitbox{ make_unique<Hitbox>(XMFLOAT3{ -1.0f, 125.0f, 0.0f }, XMFLOAT3{ 28.0f, 17.0f, 30.0f }) };
		hitbox->SetOwner(this);
		AddHitbox(hitbox);
#endif
		m_damage = 70;
		m_atkFrame = 10.0f;
		m_atkRange = 85.0f;
		break;
	}
	}
	PlayAnimation("IDLE");
}

void Monster::OnAnimation(FLOAT currFrame, UINT endFrame)
{
	// 몬스터 공격
	if (m_animationInfo->currAnimationName == "ATTACK" && !m_isAttacked && currFrame >= m_atkFrame)
	{
		Player* player{ g_gameFramework.GetScene()->GetPlayer() };
		float range{ Vector3::Length(Vector3::Sub(player->GetPosition(), GetPosition()))};
		if (range < m_atkRange)
		{
			auto scene{ reinterpret_cast<GameScene*>(g_gameFramework.GetScene()) };
			scene->OnPlayerHit(this);
		}
		m_isAttacked = TRUE;
	}

	// 공격 초기화
	if (m_animationInfo->currAnimationName != "ATTACK")
		m_isAttacked = FALSE;

	if (currFrame >= endFrame)
	{
		switch (m_animationInfo->state)
		{
		case eAnimationState::PLAY:
			if (m_animationInfo->currAnimationName == "DIE")
			{
				Delete();
				break;
			}
			if (m_animationInfo->currAnimationName == "ATTACK")
				m_isAttacked = FALSE;
			PlayAnimation(m_animationInfo->currAnimationName);
			break;
		case eAnimationState::BLENDING:
			PlayAnimation(m_animationInfo->afterAnimationName);
			break;
		}
	}
}

void Monster::ApplyServerData(const MonsterData& monsterData)
{
	if (m_animationInfo->currAnimationName == "DIE" && m_animationInfo->afterAnimationName != "DIE")
		return;

	switch (monsterData.aniType)
	{
	case eMobAnimationType::NONE:
		break;
	case eMobAnimationType::IDLE:
		if (m_animationInfo->currAnimationName != "IDLE" && m_animationInfo->afterAnimationName != "IDLE")
			PlayAnimation("IDLE", TRUE);
		break;
	case eMobAnimationType::WALKING:
		if (m_animationInfo->currAnimationName != "WALKING" && m_animationInfo->afterAnimationName != "WALKING")
			PlayAnimation("WALKING", TRUE);
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
	case eMobAnimationType::DOWN:
		if (m_animationInfo->currAnimationName != "DOWN" && m_animationInfo->afterAnimationName != "DOWN")
			PlayAnimation("DOWN", TRUE);
		break;
	case eMobAnimationType::STANDUP:
		if (m_animationInfo->currAnimationName != "STANDUP" && m_animationInfo->afterAnimationName != "STANDUP")
			PlayAnimation("STANDUP", TRUE);
		break;
	case eMobAnimationType::JUMPATK:
		if (m_animationInfo->currAnimationName != "JUMPATK" && m_animationInfo->afterAnimationName != "JUMPATK")
			PlayAnimation("JUMPATK", TRUE);
		break;
	case eMobAnimationType::LEGATK:
		if (m_animationInfo->currAnimationName != "LEGATK" && m_animationInfo->afterAnimationName != "LEGATK")
			PlayAnimation("LEGATK", TRUE);
		break;
	case eMobAnimationType::REST:
		if (m_animationInfo->currAnimationName != "REST" && m_animationInfo->afterAnimationName != "REST")
			PlayAnimation("REST", TRUE);
		break;
	case eMobAnimationType::ROAR:
		if (m_animationInfo->currAnimationName != "ROAR" && m_animationInfo->afterAnimationName != "ROAR")
			PlayAnimation("ROAR", TRUE);
		break;
	}

	m_id = monsterData.id;
	SetPosition(monsterData.pos);
	SetVelocity(monsterData.velocity);
	Rotate(0.0f, 0.0f, monsterData.yaw - m_yaw);
}

INT Monster::GetId() const
{
	return m_id;
}

eMobType Monster::GetType() const
{
	return m_type;
}

INT Monster::GetDamage() const
{
	return m_damage;
}

BossMonster::BossMonster(INT id) : Monster{ id, eMobType::ULIFO }, m_isRoared{ FALSE }
{

}

void BossMonster::OnAnimation(FLOAT currFrame, UINT endFrame)
{
	// 울부짖기 효과음
	if (m_animationInfo->currAnimationName == "ROAR" && !m_isRoared)
	{
		g_audioEngine.Play("ROAR");
		m_isRoared = TRUE;
	}

	// 다리 공격
	if (m_animationInfo->currAnimationName == "LEGATK" && !m_isAttacked && currFrame >= 14.0f)
	{
		Player* player{ g_gameFramework.GetScene()->GetPlayer() };
		float range{ Vector3::Length(Vector3::Sub(player->GetPosition(), GetPosition())) };
		if (113.0f <= range && range <= 122.0f)
		{
			auto scene{ reinterpret_cast<GameScene*>(g_gameFramework.GetScene()) };
			scene->OnPlayerHit(this);
		}
		m_isAttacked = TRUE;
	}

	// 내려 찍기 공격
	if (m_animationInfo->currAnimationName == "DOWN" && !m_isAttacked && currFrame >= 20.0f)
	{
		Player* player{ g_gameFramework.GetScene()->GetPlayer() };
		float range{ Vector3::Length(Vector3::Sub(player->GetPosition(), GetPosition())) };
		if (range <= 35.0f)
		{
			auto scene{ reinterpret_cast<GameScene*>(g_gameFramework.GetScene()) };
			scene->OnPlayerHit(this);
		}
		m_isAttacked = TRUE;
	}

	// 공격 초기화
	if (m_animationInfo->currAnimationName != "LEGATK" &&
		m_animationInfo->currAnimationName != "DOWN")
		m_isAttacked = FALSE;

	if (currFrame >= endFrame)
	{
		switch (m_animationInfo->state)
		{
		case eAnimationState::PLAY:
			// 효과음 초기화
			if (m_animationInfo->currAnimationName == "ROAR")
				m_isRoared = FALSE;

			// 사망
			if (m_animationInfo->currAnimationName == "DIE")
			{
				Delete();
				break;
			}

			// 공격 초기화
			if (m_animationInfo->currAnimationName == "LEGATK" ||
				m_animationInfo->currAnimationName == "DOWN")
				m_isAttacked = FALSE;

			// 착지 후 일어남
			if (m_animationInfo->currAnimationName == "DOWN")
			{
				PlayAnimation("STANDUP");
				break;
			}

			// 일어난 후 대기
			if (m_animationInfo->currAnimationName == "STANDUP")
			{
				PlayAnimation("IDLE");
				break;
			}

			PlayAnimation(m_animationInfo->currAnimationName);
			break;
		case eAnimationState::BLENDING:
			PlayAnimation(m_animationInfo->afterAnimationName);
			break;
		}
	}
}

OutlineObject::OutlineObject(const XMFLOAT3& color, FLOAT thickness)
{
	SetMesh(Scene::s_meshes["FULLSCREEN"]);
	SetShader(Scene::s_shaders["FULLSCREEN"]);

	SetColor(color);
	SetThickness(thickness);
}

void OutlineObject::SetColor(const XMFLOAT3& color)
{
	m_worldMatrix.m[3][0] = color.x;
	m_worldMatrix.m[3][1] = color.y;
	m_worldMatrix.m[3][2] = color.z;
}

void OutlineObject::SetThickness(FLOAT thickness)
{
	m_worldMatrix.m[3][3] = thickness;
}

DustParticle::DustParticle()
{
	m_mesh = Scene::s_meshes["DUST"];
	m_shader = Scene::s_shaders["DUST"];
}

void DustParticle::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
	UpdateShaderVariable(commandList);
	auto m{ reinterpret_cast<ParticleMesh*>(m_mesh.get()) };
	auto s{ reinterpret_cast<ParticleShader*>(m_shader.get()) };
	commandList->SetPipelineState(s->GetStreamPipelineState().Get());
	m->RenderStreamOutput(commandList);
	commandList->SetPipelineState(s->GetPipelineState().Get());
	m->Render(commandList);
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