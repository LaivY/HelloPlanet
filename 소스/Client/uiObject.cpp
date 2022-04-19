#include "uiObject.h"

UIObject::UIObject(FLOAT width, FLOAT height) : m_pivot{ ePivot::CENTER }, m_width{ width }, m_height{ height }
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
	case ePivot::LEFTTOP:
		m_worldMatrix._41 = x + m_width / 2.0f;
		m_worldMatrix._42 = y - m_height / 2.0f;
		break;
	case ePivot::CENTERTOP:
		m_worldMatrix._42 = y - m_height / 2.0f;
		break;
	case ePivot::RIGHTTOP:
		m_worldMatrix._41 = x - m_width / 2.0f;
		m_worldMatrix._42 = y - m_height / 2.0f;
		break;
	case ePivot::LEFTCENTER:
		m_worldMatrix._41 = x + m_width / 2.0f;
		break;
	case ePivot::CENTER:
		m_worldMatrix._41 = x;
		m_worldMatrix._42 = y;
		break;
	case ePivot::RIGHTCENTER:
		m_worldMatrix._41 = x - m_width / 2.0f;
		break;
	case ePivot::LEFTBOT:
		m_worldMatrix._41 = x + m_width / 2.0f;
		m_worldMatrix._42 = y + m_height / 2.0f;
		break;
	case ePivot::CENTERBOT:
		m_worldMatrix._42 = y + m_height / 2.0f;
		break;
	case ePivot::RIGHTBOT:
		m_worldMatrix._41 = x - m_width / 2.0f;
		m_worldMatrix._42 = y + m_height / 2.0f;
		break;
	}
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
