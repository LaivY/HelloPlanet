#include "stdafx.h"
#include "uiObject.h"
#include "audioEngine.h"
#include "camera.h"
#include "framework.h"
#include "gameScene.h"
#include "player.h"
#include "scene.h"
#include "shader.h"
#include "texture.h"

UIObject::UIObject(FLOAT width, FLOAT height) : m_isFitToScreen{ FALSE }, m_pivot{ ePivot::CENTER }, m_screenPivot{ ePivot::CENTER }, m_width{ width }, m_height{ height }, m_pivotPosition{}, m_scale{ 1.0f, 1.0f }
{
	SetShader(Scene::s_shaders["UI"]);
	SetMesh(Scene::s_meshes["UI"]);

	m_worldMatrix._11 = width;
	m_worldMatrix._22 = height;
}

void UIObject::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { }
void UIObject::OnMouseEvent(HWND hWnd, FLOAT deltaTime) { }

void UIObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
	if (m_isFitToScreen)
	{
		XMFLOAT4X4 worldMatrix{ m_worldMatrix };
		XMFLOAT4X4 scale{};
		XMStoreFloat4x4(&scale, XMMatrixScaling(g_width / static_cast<float>(g_maxWidth), g_height / static_cast<float>(g_maxHeight), 1.0f));
		m_worldMatrix = Matrix::Mul(m_worldMatrix, scale);

		float width{ m_width }, height{ m_height };
		m_width *= g_width / static_cast<float>(g_maxWidth);
		m_height *= g_height / static_cast<float>(g_maxHeight);
		SetPosition(m_pivotPosition);

		GameObject::Render(commandList, shader);

		m_worldMatrix = worldMatrix;
		m_width = width;
		m_height = height;
	}
	else GameObject::Render(commandList, shader);
}

void UIObject::Update(FLOAT deltaTime)
{
	if (!m_texture || !m_textureInfo) return;

	m_textureInfo->timer += deltaTime;
	if (m_textureInfo->timer > m_textureInfo->interver)
	{
		m_textureInfo->frame += static_cast<int>(m_textureInfo->timer / m_textureInfo->interver);
		m_textureInfo->timer = fmod(m_textureInfo->timer, m_textureInfo->interver);
	}
	if (m_textureInfo->frame >= m_texture->GetCount())
	{
		if (m_textureInfo->loop)
			m_textureInfo->frame = 0;
		else
		{
			m_textureInfo->frame = static_cast<int>(m_texture->GetCount() - 1);
			Delete();
		}
	}
}

void UIObject::SetFitToScreen(BOOL fitToScreen)
{
	m_isFitToScreen = fitToScreen;
}

void UIObject::SetPosition(const XMFLOAT2& position)
{
	m_pivotPosition = position;
	float width{ static_cast<float>(g_width) }, height{ static_cast<float>(g_height) };

	// 화면의 피봇 위치에서 (x, y)만큼 이동
	switch (m_screenPivot)
	{
	case ePivot::LEFTTOP:
		m_worldMatrix._41 = position.x - width / 2.0f;
		m_worldMatrix._42 = position.y + height / 2.0f;
		break;
	case ePivot::CENTERTOP:
		m_worldMatrix._41 = position.x;
		m_worldMatrix._42 = position.y + height / 2.0f;
		break;
	case ePivot::RIGHTTOP:
		m_worldMatrix._41 = position.x + width / 2.0f;
		m_worldMatrix._42 = position.y + height / 2.0f;
		break;
	case ePivot::LEFTCENTER:
		m_worldMatrix._41 = position.x - width / 2.0f;
		m_worldMatrix._42 = position.y;
		break;
	case ePivot::CENTER:
		m_worldMatrix._41 = position.x;
		m_worldMatrix._42 = position.y;
		break;
	case ePivot::RIGHTCENTER:
		m_worldMatrix._41 = position.x + width / 2.0f;
		m_worldMatrix._42 = position.y;
		break;
	case ePivot::LEFTBOT:
		m_worldMatrix._41 = position.x - width / 2.0f;
		m_worldMatrix._42 = position.y - height / 2.0f;
		break;
	case ePivot::CENTERBOT:
		m_worldMatrix._41 = position.x;
		m_worldMatrix._42 = position.y - height / 2.0f;
		break;
	case ePivot::RIGHTBOT:
		m_worldMatrix._41 = position.x + width / 2.0f;
		m_worldMatrix._42 = position.y - height / 2.0f;
		break;
	}

	// 오브젝트 피봇 위치에 따라 자신의 너비, 높이만큼 조정
	switch (m_pivot)
	{
	case ePivot::LEFTTOP:
		m_worldMatrix._41 += m_width / 2.0f;
		m_worldMatrix._42 -= m_height / 2.0f;
		break;
	case ePivot::CENTERTOP:
		m_worldMatrix._42 -= m_height / 2.0f;
		break;
	case ePivot::RIGHTTOP:
		m_worldMatrix._41 -= m_width / 2.0f;
		m_worldMatrix._42 -= m_height / 2.0f;
		break;
	case ePivot::LEFTCENTER:
		m_worldMatrix._41 += m_width / 2.0f;
		break;
	case ePivot::CENTER:
		break;
	case ePivot::RIGHTCENTER:
		m_worldMatrix._41 -= m_width / 2.0f;
		break;
	case ePivot::LEFTBOT:
		m_worldMatrix._41 += m_width / 2.0f;
		m_worldMatrix._42 += m_height / 2.0f;
		break;
	case ePivot::CENTERBOT:
		m_worldMatrix._42 += m_height / 2.0f;
		break;
	case ePivot::RIGHTBOT:
		m_worldMatrix._41 -= m_width / 2.0f;
		m_worldMatrix._42 += m_height / 2.0f;
		break;
	}
}

void UIObject::SetPivot(const ePivot& pivot)
{
	m_pivot = pivot;
}

void UIObject::SetScreenPivot(const ePivot& pivot)
{
	m_screenPivot = pivot;
}

void UIObject::SetScale(const XMFLOAT2& scale)
{
	m_scale = scale;
}

void UIObject::SetWidth(FLOAT width)
{
	FLOAT deltaWidth{ width - m_width };
	m_width = width;
	m_worldMatrix._11 = width;

	switch (m_pivot)
	{
	case ePivot::LEFTTOP:
	case ePivot::LEFTCENTER:
	case ePivot::LEFTBOT:
		m_worldMatrix._41 += deltaWidth / 2.0f;
		break;
	case ePivot::RIGHTTOP:
	case ePivot::RIGHTCENTER:
	case ePivot::RIGHTBOT:
		m_worldMatrix._41 -= deltaWidth / 2.0f;
		break;
	}
}

void UIObject::SetHeight(FLOAT height)
{
	FLOAT deltaHeight{ height - m_height };
	m_height = height;
	m_worldMatrix._22 = height;

	switch (m_pivot)
	{
	case ePivot::LEFTTOP:
	case ePivot::RIGHTTOP:
	case ePivot::CENTERTOP:
		m_worldMatrix._42 += deltaHeight / 2.0f;
		break;
	case ePivot::LEFTBOT:
	case ePivot::CENTERBOT:
	case ePivot::RIGHTBOT:
		m_worldMatrix._42 -= deltaHeight / 2.0f;
		break;
	}
}

ePivot UIObject::GetPivot() const
{
	return m_pivot;
}

ePivot UIObject::GetScreenPivot() const
{
	return m_screenPivot;
}

XMFLOAT2 UIObject::GetPivotPosition() const
{
	return m_pivotPosition;
}

FLOAT UIObject::GetWidth() const
{
	return m_width;
}

FLOAT UIObject::GetHeight() const
{
	return m_height;
}

HpUIObject::HpUIObject(FLOAT width, FLOAT height) : UIObject{ width, height }, m_hp{ -1 }, m_maxHp{}, m_deltaHp{}, m_originWidth{ width }, m_timerState{ FALSE }, m_timer{}
{
	// 체력이 달면 타이머를 이용해 조금씩 달게하는 애니메이션을 보여준다.
}

void HpUIObject::Update(FLOAT deltaTime)
{
	if (!m_player) return;
	int hp{ m_player->GetHp() };
	if (m_hp != hp)
	{
		m_deltaHp += m_hp - hp;
		m_hp = hp;
	}
	m_maxHp = m_player->GetMaxHp();

	constexpr float decreseSpeed{ 150.0f };
	if (m_deltaHp > 0.0f)
	{
		m_deltaHp = max(0.0f, m_deltaHp - decreseSpeed * deltaTime);
		SetWidth(m_originWidth * static_cast<float>(m_hp + m_deltaHp) / static_cast<float>(m_maxHp));
	}
	else
	{
		m_deltaHp = min(0.0f, m_deltaHp + decreseSpeed * deltaTime);
		SetWidth(m_originWidth * static_cast<float>(m_hp + m_deltaHp) / static_cast<float>(m_maxHp));
	}
}

void HpUIObject::SetPlayer(const shared_ptr<Player>& player)
{
	m_player = player;
	m_hp = player->GetHp();
	m_maxHp = player->GetMaxHp();
}

CrosshairUIObject::CrosshairUIObject(FLOAT width, FLOAT height) : UIObject{ width, height }, m_weaponType{ eWeaponType::AR }, m_bulletCount{}, m_radius{ 11.0f }, m_timer{}
{
	// width, height는 선 하나의 너비와 높이를 나타낸다.
	// 반지름은 각 선이 중심으로부터 떨어져있는 거리이다.

	// 각 선들은 메쉬, 셰이더, 텍스쳐를 가질 필요 없다.
	// 메쉬, 셰이더, 텍스쳐는 CrosshairUIObject가 갖고있고 각 선들은 월드행렬만 갖고있다.
	// 0번째는 위, 1번째는 오른쪽, 2번째는 왼쪽, 3번째는 밑에 있는 선이다.
	for (int i = 0 ; i < m_lines.size(); ++i)
	{
		m_lines[i] = make_unique<UIObject>(0.0f, 0.0f);
		XMFLOAT4X4 worldMatrix{ Matrix::Identity() };

		// 가로, 세로 설정
		if (i == 0 || i == 3)
		{
			worldMatrix._11 = width;
			worldMatrix._22 = height;
		}
		else
		{
			worldMatrix._11 = height;
			worldMatrix._22 = width;
		}
		m_lines[i]->SetWorldMatrix(worldMatrix);
	}
}

void CrosshairUIObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& /*shader*/)
{
	UIObject::UpdateShaderVariable(commandList);
	commandList->SetPipelineState(m_shader->GetPipelineState().Get());
	for (const auto& l : m_lines)
		l->Render(commandList);
}

void CrosshairUIObject::Update(FLOAT deltaTime)
{
	if (!m_player) return;

	constexpr float walkingMaxRadius{ 16.0f }, walkingRadiusSpeed{ 75.0f };
	constexpr float firingRadiusSpeed{ 51.0f };
	constexpr float minRadius{ 11.0f }, defaultRadiusSeed{ 20.0f };
	
	if (INT bulletCount{ m_player->GetBulletCount() }; m_bulletCount != bulletCount)
	{
		m_bulletCount = bulletCount;
		if (m_bulletCount != m_player->GetMaxBulletCount())
			switch (m_weaponType)
			{
			case eWeaponType::AR:
				m_radius = 14.0f;
				break;
			case eWeaponType::SG:
				m_radius = 16.0f;
				break;
			case eWeaponType::MG:
				m_radius = 15.0f;
				break;
			}
	}
	else
	{
		m_radius = max(minRadius, m_radius - defaultRadiusSeed * deltaTime);
	}

	for (int i = 0; i < m_lines.size(); ++i)
	{
		XMFLOAT4X4 worldMatrix{ m_lines[i]->GetWorldMatrix() };
		if (i == 0)
			worldMatrix._42 = m_radius;
		else if (i == 1)
			worldMatrix._41 = m_radius;
		else if (i == 2)
			worldMatrix._41 = -m_radius;
		else
			worldMatrix._42 = -m_radius;
		m_lines[i]->SetWorldMatrix(worldMatrix);
	}
}

void CrosshairUIObject::SetMesh(shared_ptr<Mesh>& mesh)
{
	for (auto& l : m_lines)
		l->SetMesh(mesh);
}

void CrosshairUIObject::SetPlayer(const shared_ptr<Player>& player)
{
	m_player = player;
	m_weaponType = player->GetWeaponType();
	m_bulletCount = player->GetBulletCount();
}

MenuUIObject::MenuUIObject(FLOAT width, FLOAT height) : UIObject{ width, height }, m_isMouseOver{ FALSE }
{
	m_mouseClickCallBack = []() {};
}

void MenuUIObject::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_LBUTTONDOWN && m_isMouseOver)
		m_mouseClickCallBack();
}

void MenuUIObject::OnMouseEvent(HWND hWnd, FLOAT deltaTime)
{
	RECT c{};
	GetWindowRect(hWnd, &c);

	POINT p{}; GetCursorPos(&p);
	ScreenToClient(hWnd, &p);

	XMFLOAT3 pos{ GetPosition() };
	float hw{ m_width / 2.0f };
	float hh{ m_height / 2.0f };

	float left{ pos.x - hw + g_width / 2.0f };
	float right{ pos.x + hw + g_width / 2.0f };
	float top{ g_height / 2.0f - (pos.y - hh)  };
	float bottom{ g_height / 2.0f - (pos.y + hh) };

	if (left <= p.x && p.x <= right &&
		bottom <= p.y && p.y <= top)
		m_isMouseOver = TRUE;
	else
		m_isMouseOver = FALSE;
}

void MenuUIObject::Update(FLOAT deltaTime)
{

}

void MenuUIObject::SetMouseClickCallBack(const function<void()>& callBackFunc)
{
	m_mouseClickCallBack = callBackFunc;
}

RewardUIObject::RewardUIObject(FLOAT width, FLOAT height) : MenuUIObject{ width, height }, m_timer{ 0.0f }
{
	
}

void RewardUIObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
	constexpr float MOUSE_OVER_SCALE{ 0.05f };

	FLOAT originWidth{ m_width }, originHeight{ m_height };

	m_worldMatrix._11 = originWidth + originWidth * MOUSE_OVER_SCALE * m_timer / 0.5f;
	m_worldMatrix._22 = originHeight + originHeight * MOUSE_OVER_SCALE * m_timer / 0.5f;
	UIObject::Render(commandList, shader);
	m_worldMatrix._11 = originWidth;
	m_worldMatrix._22 = originHeight;
}

void RewardUIObject::Update(FLOAT deltaTime)
{
	if (m_isMouseOver)
		m_timer = min(0.5f, m_timer + deltaTime * 3.0f);
	else
		m_timer = max(0.0f, m_timer - deltaTime * 3.0f);
}

HitUIObject::HitUIObject(int monsterId) : UIObject{ 50.0f, 50.0f }, m_monsterId{ monsterId }, m_angle{ 0.0f }, m_timer{ 2.0f }
{
	SetTexture(Scene::s_textures["ARROW"]);
	SetPosition(XMFLOAT2{ -9999.0f, -9999.0f });
	m_isFitToScreen = TRUE;
}

void HitUIObject::Update(FLOAT deltaTime)
{
	auto it{ ranges::find_if(GameScene::s_monsters, [&](const unique_ptr<Monster>& monster) { return monster->GetId() == m_monsterId; }) };
	if (it == GameScene::s_monsters.end())
	{
		Delete();
		return;
	}

	auto player{ g_gameFramework.GetScene()->GetPlayer() };
	auto& monster{ *it };

	XMFLOAT3 monsterPosition{ monster->GetPosition() };
	XMFLOAT3 v1{ player->GetLook() };
	v1.y = 0.0f;
	v1 = Vector3::Normalize(v1);

	XMFLOAT3 v2{ Vector3::Sub(monsterPosition, player->GetPosition()) };
	v2.y = 0.0f;
	v2 = Vector3::Normalize(v2);

	m_angle = XMVectorGetX(XMVector3AngleBetweenNormals(XMLoadFloat3(&v1), XMLoadFloat3(&v2)));
	if (Vector3::Cross(v1, v2).y < 0)
		m_angle = -m_angle;

	float radius{ g_height * 0.4f };
	SetPosition(XMFLOAT2{ radius * sinf(m_angle), radius * cosf(m_angle) });
	Rotate(-m_roll + XMConvertToDegrees(-m_angle), 0.0f, 0.0f);

	m_timer -= deltaTime;
	if (m_timer < 0.0f) Delete();
}

AutoTargetUIObject::AutoTargetUIObject() : UIObject{ 0.0f, 0.0f }
{
	SetTexture(Scene::s_textures["TARGET"]);
	m_isFitToScreen = TRUE;
	m_player = g_gameFramework.GetScene()->GetPlayer();
	m_camera = g_gameFramework.GetScene()->GetCamera();
	SetPosition(XMFLOAT2{ -999.0f, -999.0f });
}

void AutoTargetUIObject::Update(FLOAT deltaTime)
{
	if (m_player->GetWeaponType() != eWeaponType::AR)
		return;

	if (!m_player->GetIsSkillActive())
	{
		SetPosition(XMFLOAT2{ -999.0f, -999.0f });
		m_player->SetAutoTarget(-1);
		return;
	}

	bool isDetected{ false };	// 화면에 있는 몬스터를 감지했는지 여부
	float distance{ FLT_MAX };	// 화면 중심에서부터 몬스터까지의 거리(스크린 좌표계)
	XMFLOAT4X4 toScreenMatrix{ Matrix::Mul(m_camera->GetViewMatrix(), m_camera->GetProjMatrix()) };

	for (const auto& mob : GameScene::s_monsters)
	{
		if (mob->GetAnimationInfo()->currAnimationName == "DIE")
			continue;

		// 몬스터들의 y좌표가 다 0으로 되어있기 때문에
		// 조준 이미지를 적절한 위치에 렌더링하기 위해 각 몬스터들마다 정해진 값을 더해준다.
		XMFLOAT3 offset{};
		float scale{};
		switch (mob->GetType())
		{
		case eMobType::GAROO:
			offset = XMFLOAT3{ 0.0f, 11.5f, 0.0f };
			scale = 1.0f;
			break;
		case eMobType::SERPENT:
			offset = XMFLOAT3{ 0.0f, 22.0f, 0.0f };
			scale = 2.0f;
			break;
		case eMobType::HORROR:
			offset = XMFLOAT3{ 0.0f, 26.0f, 0.0f };
			scale = 1.6f;
			break;
		case eMobType::ULIFO:
			offset = XMFLOAT3{ 0.0f, 125.0f, 0.0f };
			scale = 1.8f;
			break;
		}

		// 몬스터 좌표를 스크린 좌표계로 변환
		XMFLOAT3 mobScreenPos{ Vector3::TransformCoord(Vector3::Add(mob->GetPosition(), offset), toScreenMatrix) };
		if (mobScreenPos.x < -1.1f || mobScreenPos.x > 1.1f) continue;
		if (mobScreenPos.y < -0.9f || mobScreenPos.y > 0.9f) continue;
		if (mobScreenPos.z <  0.0f || mobScreenPos.z > 1.0f) continue;

		float mobZ{ mobScreenPos.z };

		// 화면 중심과 몬스터 간의 거리
		mobScreenPos.z = 0.0f;
		float d{ Vector3::Length(mobScreenPos) };
		if (d < distance)
		{
			isDetected = true;
			distance = d;
			m_player->SetAutoTarget(mob->GetId());
			SetPosition(XMFLOAT2{ mobScreenPos.x / 2.0f * g_width, mobScreenPos.y / 2.0f * g_height });

			// z값을 이용하여 가까울수록 크게, 멀리있을수록 작게
			// 0.97 ~ 0.99 구간을 정규화해서 사용함
			constexpr float a{ 0.97f }, b{ 0.996f };
			mobZ = clamp(mobZ, a, b);

			float t{ (mobZ - a) / (b - a) };
			float value{ lerp(500.0f, 200.0f, t) * scale };
			SetWidth(value);
			SetHeight(value);
		}
	}

	// 화면에 몬스터가 없으면 렌더링되지 않도록 화면 밖으로 보냄
	if (!isDetected)
	{
		SetPosition(XMFLOAT2{ -999.0f, -999.0f });
		m_player->SetAutoTarget(-1);
	}
}

SkillGageUIObject::SkillGageUIObject() : UIObject{ 150.0f, 150.0f }
{
	SetShader(Scene::s_shaders["UI_SKILL_GAGE"]);
	m_player = g_gameFramework.GetScene()->GetPlayer();
}

void SkillGageUIObject::Update(FLOAT deltaTime)
{
	// 월드행렬의 (4, 4)에 각도를 넣어줌
	// 정점 셰이더에서는 (4, 4)는 1로 바꿔서 렌더링함
	// 픽셀 셰이더에서는 (4, 4)값을 읽어 각도로 사용함
	m_worldMatrix._44 = lerp(0.0f, 360.0f, m_player->GetSkillGage() / 100.0f);
}

WarningUIObject::WarningUIObject() : UIObject{ static_cast<float>(g_width), static_cast<float>(g_height) }, m_timer{}, m_warning{ FALSE }
{
	SetShader(Scene::s_shaders["UI_WARNING"]);
	SetTexture(Scene::s_textures["WARNING"]);
	m_player = g_gameFramework.GetScene()->GetPlayer();
}

void WarningUIObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
	float ratio{ static_cast<float>(m_player->GetHp()) / static_cast<float>(m_player->GetMaxHp()) };
	if (m_player->GetHp() > 0 && ratio <= 0.3f)
		UIObject::Render(commandList, shader);
}

void WarningUIObject::Update(FLOAT deltaTime)
{
	float ratio{ static_cast<float>(m_player->GetHp()) / static_cast<float>(m_player->GetMaxHp()) };
	if (m_player->GetHp() > 0 && ratio <= 0.3f)
	{
		// 한번 깜빡임에 걸리는 시간
		constexpr float frequence{ 3.0f };
		if (m_timer <= frequence / 2.0f)
		{
			m_worldMatrix._44 = lerp(0.0f, 1.0f, m_timer / (frequence / 2.0f));
		}
		else
		{
			m_worldMatrix._44 = lerp(1.0f, 0.0f, (m_timer - frequence / 2.0f) / (frequence / 2.0f));
		}
		m_timer = min(frequence, m_timer + deltaTime);
		if (m_timer >= frequence)
			m_timer = 0.0f;

		// 효과음
		if (!m_warning)
		{
			m_warning = TRUE;
			g_audioEngine.ChangeMusic("HEARTBEAT");
		}
	}
	else
	{
		m_timer = 0.0f;
		m_worldMatrix._44 = 0.0f;

		if (m_warning)
		{
			m_warning = FALSE;
			g_audioEngine.ChangeMusic("INGAME");
		}
	}
}
