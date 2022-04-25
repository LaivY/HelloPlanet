#include "uiObject.h"
#include "framework.h"

UIObject::UIObject() : m_isFitToScreen{ FALSE }, m_pivot { ePivot::CENTER }, m_screenPivot{ ePivot::CENTER }, m_width{}, m_height{}, m_pivotPosition{}, m_scale{ 1.0f, 1.0f }
{

}

UIObject::UIObject(FLOAT width, FLOAT height) : m_isFitToScreen{ FALSE }, m_pivot{ ePivot::CENTER }, m_screenPivot{ ePivot::CENTER }, m_width{ width }, m_height{ height }, m_pivotPosition{}, m_scale{ 1.0f, 1.0f }
{
	m_worldMatrix._11 = width;
	m_worldMatrix._22 = height;
}

void UIObject::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

}

void UIObject::OnMouseEvent(HWND hWnd, FLOAT deltaTime)
{

}

void UIObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
	if (m_isFitToScreen)
	{
		float width{ m_width }, height{ m_height };
		m_width *= g_width / static_cast<float>(g_maxWidth);
		m_height *= g_height / static_cast<float>(g_maxHeight);
		m_worldMatrix._11 = m_width;
		m_worldMatrix._22 = m_height;
		SetPosition(m_pivotPosition);

		GameObject::Render(commandList, shader);

		m_width = width;
		m_height = height;
		m_worldMatrix._11 = width;
		m_worldMatrix._22 = height;
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

void UIObject::SetFitToScreen(BOOL fitToScreen)
{
	m_isFitToScreen = fitToScreen;
}

void UIObject::SetPosition(const XMFLOAT3& position)
{
	SetPosition(XMFLOAT2{ position.x, position.y });
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

	constexpr float decreseSpeed{ 50.0f };
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

CrosshairUIObject::CrosshairUIObject(FLOAT width, FLOAT height) : UIObject{ width, height }, m_radius{ 11.0f }, m_timer{}
{
	// width, height는 선 하나의 너비와 높이를 나타낸다.
	// 반지름은 각 선이 중심으로부터 떨어져있는 거리이다.

	// 각 선들은 메쉬, 셰이더, 텍스쳐를 가질 필요 없다.
	// 메쉬, 셰이더, 텍스쳐는 CrosshairUIObject가 갖고있고 각 선들은 월드행렬만 갖고있다.
	// 0번째는 위, 1번째는 오른쪽, 2번째는 왼쪽, 3번째는 밑에 있는 선이다.
	for (int i = 0 ; i < m_lines.size(); ++i)
	{
		m_lines[i] = make_unique<UIObject>();
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
	constexpr float minRadius{ 11.0f }, defaultRadiusSeed{ 50.0f };

	float r{ 0.0f };
	if (m_player->GetCurrAnimationName() != "IDLE")
	{
		// 움직이면서 쏘면 더 많이 벌어짐
		if (m_player->GetUpperCurrAnimationName() == "FIRING")
			r = Utile::Random(0.0f, 7.0f);
		m_radius = min(walkingMaxRadius + r, m_radius + walkingRadiusSpeed * deltaTime);
	}
	else
	{
		// 제자리에서 쏘면 조금 벌어짐
		if (m_player->GetUpperCurrAnimationName() == "FIRING")
			r = Utile::Random(0.0f, 3.0f);
		m_radius = max(minRadius + r, m_radius - defaultRadiusSeed * deltaTime);
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
}

MenuUIObject::MenuUIObject(FLOAT width, FLOAT height) : UIObject{ width, height }, m_isMouseOver{ FALSE }
{

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

void MenuUIObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
	UIObject::Render(commandList, shader);
}

void MenuUIObject::Update(FLOAT deltaTime)
{

}

void MenuUIObject::SetMouseClickCallBack(const function<void()>& callBackFunc)
{
	m_mouseClickCallBack = callBackFunc;
}
