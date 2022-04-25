﻿#include "textObject.h"
#include "framework.h"

unordered_map<string, ComPtr<ID2D1SolidColorBrush>>	TextObject::s_brushes;
unordered_map<string, ComPtr<IDWriteTextFormat>>	TextObject::s_formats;

TextObject::TextObject() 
	: m_isDeleted{ FALSE }, m_isMouseOver{ FALSE }, m_rect{ 0.0f, 0.0f, static_cast<float>(g_width), static_cast<float>(g_height) },
	  m_position{}, m_pivotPosition{}, m_screenPivot{ ePivot::CENTER }, m_width{}, m_height{}
{

}

void TextObject::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

}

void TextObject::OnMouseEvent(HWND hWnd, FLOAT deltaTime)
{

}

void TextObject::Render(const ComPtr<ID2D1DeviceContext2>& device)
{
	device->SetTransform(D2D1::Matrix3x2F::Translation(m_position.x, m_position.y));
	device->DrawText(
		m_text.c_str(),
		static_cast<UINT32>(m_text.size()),
		s_formats[m_format].Get(),
		&m_rect,
		s_brushes[m_brush].Get()
	);
}

void TextObject::Update(FLOAT deltaTime)
{

}

void TextObject::CalcWidthHeight()
{
	if (m_text.empty()) return;

	ComPtr<IDWriteTextLayout> layout;
	g_gameFramework.GetDWriteFactory()->CreateTextLayout(m_text.c_str(), static_cast<UINT32>(m_text.size()), TextObject::s_formats[m_format].Get(), g_width, g_height, &layout);

	DWRITE_TEXT_METRICS metrics;
	layout->GetMetrics(&metrics);

	m_width = metrics.widthIncludingTrailingWhitespace;
	m_height = metrics.height;
}

void TextObject::SetRect(const D2D1_RECT_F& rect)
{
	m_rect = rect;
}

void TextObject::SetBrush(const string& brush)
{
	m_brush = brush;
}

void TextObject::SetFormat(const string& format)
{
	m_format = format;
}

void TextObject::SetText(const wstring& text)
{
	m_text = text;
	if (!m_brush.empty() && !m_format.empty())
	{
		CalcWidthHeight();
		m_rect = D2D1_RECT_F{ 0.0f, 0.0f, m_width + 1.0f, m_height };
	}
}

void TextObject::SetPosition(const XMFLOAT2& position)
{
	m_pivotPosition = position;
	float width{ static_cast<float>(g_width) }, height{ static_cast<float>(g_height) };

	switch (m_screenPivot)
	{
	case ePivot::LEFTTOP:
		m_position.x = position.x;
		m_position.y = position.y;
		break;
	case ePivot::CENTERTOP:
		m_position.x = position.x + width / 2.0f;
		m_position.y = position.y;
		break;
	case ePivot::RIGHTTOP:
		m_position.x = position.x + width;
		m_position.y = position.y;
		break;
	case ePivot::LEFTCENTER:
		m_position.x = position.x;
		m_position.y = position.y + height / 2.0f;
		break;
	case ePivot::CENTER:
		m_position.x = position.x + width / 2.0f;
		m_position.y = position.y + height / 2.0f;
		break;
	case ePivot::RIGHTCENTER:
		m_position.x = position.x + width;
		m_position.y = position.y + height / 2.0f;
		break;
	case ePivot::LEFTBOT:
		m_position.x = position.x;
		m_position.y = position.y + height;
		break;
	case ePivot::CENTERBOT:
		m_position.x = position.x + width / 2.0f;
		m_position.y = position.y + height;
		break;
	case ePivot::RIGHTBOT:
		m_position.x = position.x + width;
		m_position.y = position.y + height;
		break;
	}
}

void TextObject::SetScreenPivot(const ePivot& pivot)
{
	m_screenPivot = pivot;
}

BOOL TextObject::isDeleted() const
{
	return m_isDeleted;
}

wstring TextObject::GetText() const
{
	return m_text;
}

XMFLOAT2 TextObject::GetPosition() const
{
	return m_position;
}

XMFLOAT2 TextObject::GetPivotPosition() const
{
	return m_pivotPosition;
}

FLOAT TextObject::GetWidth() const
{
	return m_width;
}

FLOAT TextObject::GetHeight() const
{
	return m_height;
}

BulletTextObject::BulletTextObject() : m_bulletCount{ -1 }, m_scale{ 1.0f }, m_timerState{ FALSE }, m_scaleTimer{}
{
	m_rect = { 0.0f, 0.0f, 100.0f, 0.0f };
}

void BulletTextObject::Render(const ComPtr<ID2D1DeviceContext2>& device)
{
	if (!m_player) return;

	// 플레이어의 총알 수를 표시한다.
	// 포멧은 좌측 하단 정렬이고, 전체 총알 수를 표시한 후 그 왼쪽에 현재 총알 수를 그린다.
	wstring maxBulletCount{ to_wstring(m_player->GetMaxBulletCount()) };
	device->SetTransform(D2D1::Matrix3x2F::Translation(m_position.x, m_position.y));
	device->DrawText(maxBulletCount.c_str(), static_cast<UINT32>(maxBulletCount.size()), s_formats["MAXBULLETCOUNT"].Get(), &m_rect, s_brushes["BLACK"].Get());

	m_text = maxBulletCount;
	m_format = "MAXBULLETCOUNT";
	CalcWidthHeight();
	float maxBulletTextWidth{ m_width };

	wstring slash{ TEXT("/") };
	device->SetTransform(D2D1::Matrix3x2F::Translation(m_position.x - maxBulletTextWidth, m_position.y));
	device->DrawText(slash.c_str(), static_cast<UINT32>(slash.size()), s_formats["MAXBULLETCOUNT"].Get(), &m_rect, s_brushes["BLUE"].Get());

	m_text = slash;
	m_format = "MAXBULLETCOUNT";
	CalcWidthHeight();
	float slashTextWidth{ m_width };

	// 현재 총알 텍스트 애니메이션
	m_text = to_wstring(m_bulletCount);
	D2D1::Matrix3x2F matrix{};
	matrix.SetProduct(D2D1::Matrix3x2F::Scale(m_scale, m_scale, { m_rect.right, m_rect.bottom }), D2D1::Matrix3x2F::Translation(m_position.x - maxBulletTextWidth - slashTextWidth, m_position.y));
	device->SetTransform(matrix);
	device->DrawText(m_text.c_str(), static_cast<UINT32>(m_text.size()), s_formats["BULLETCOUNT"].Get(), &m_rect, m_bulletCount == 0 ? s_brushes["RED"].Get() : s_brushes["BLACK"].Get());
}

void BulletTextObject::Update(FLOAT deltaTime)
{
	if (!m_player) return;
	int bulletCount{ m_player->GetBulletCount() };
	if (m_bulletCount != bulletCount)
	{
		m_bulletCount = bulletCount;
		m_timerState = TRUE;
		m_scaleTimer = 0.0f;
	}

	// 총알 수 변함에 따라 사각형 범위 재계산
	m_text = to_wstring(bulletCount);
	m_format = "BULLETCOUNT";
	CalcWidthHeight();
	float width{ m_width };
	m_rect.bottom = m_height;

	// 스케일
	constexpr float duration{ 0.1f };
	float angle{ 30.0f + 150.0f * (m_scaleTimer / duration) };
	m_scale = 1.0f + sinf(XMConvertToRadians(angle)) * 0.2f;

	if (m_timerState) m_scaleTimer += deltaTime;
	if (m_scaleTimer > duration)
		m_timerState = FALSE;
}

void BulletTextObject::SetPlayer(const shared_ptr<Player>& player)
{
	m_player = player;
}

HPTextObject::HPTextObject() : m_hp{ -1 }, m_scale{ 1.0f }, m_timerState{ FALSE }, m_scaleTimer{}
{
	m_rect = { 0.0f, 0.0f, 100.0f, 0.0f };
}

void HPTextObject::Render(const ComPtr<ID2D1DeviceContext2>& device)
{
	if (!m_player) return;

	// 플레이어의 체력을 표시한다.
	// 포멧은 우측 하단 정렬이고, 현재 체력을 표시하고 그 오른쪽에 전체 체력을 표시한다.
	wstring hpText{ to_wstring(m_hp) };
	m_text = hpText;
	m_format = "HP";
	CalcWidthHeight();
	float hpTextWidth{ m_width };

	D2D1::Matrix3x2F matrix{};
	matrix.SetProduct(D2D1::Matrix3x2F::Scale(m_scale, m_scale, { hpTextWidth, m_rect.bottom }), D2D1::Matrix3x2F::Translation(m_position.x, m_position.y));
	device->SetTransform(matrix);
	device->DrawText(hpText.c_str(), static_cast<UINT32>(hpText.size()), s_formats["HP"].Get(), &m_rect, s_brushes["BLACK"].Get());

	wstring slash{ TEXT("/") };
	device->SetTransform(D2D1::Matrix3x2F::Translation(m_position.x + hpTextWidth, m_position.y));
	device->DrawText(slash.c_str(), static_cast<UINT32>(slash.size()), s_formats["MAXHP"].Get(), &m_rect, s_brushes["BLUE"].Get());

	m_text = slash;
	m_format = "MAXHP";
	CalcWidthHeight();
	float slashTextWidth{ m_width };

	wstring maxHpText{ to_wstring(m_player->GetMaxHp()) };
	device->SetTransform(D2D1::Matrix3x2F::Translation(m_position.x + hpTextWidth + slashTextWidth, m_position.y));
	device->DrawText(maxHpText.c_str(), static_cast<UINT32>(maxHpText.size()), s_formats["MAXHP"].Get(), &m_rect, s_brushes["BLACK"].Get());
}

void HPTextObject::Update(FLOAT deltaTime)
{
	if (!m_player) return;
	int hp{ m_player->GetHp() };
	if (m_hp != hp)
	{
		m_hp = hp;
		m_timerState = TRUE;
		m_scaleTimer = 0.0f;
	}

	// 체력 수치가 변함에 따라 사각형 범위 재계산
	m_text = to_wstring(hp);
	m_format = "BULLETCOUNT";
	CalcWidthHeight();
	float width{ m_width };
	m_rect.bottom = m_height;

	// 스케일
	constexpr float duration{ 0.1f };
	float angle{ 30.0f + 150.0f * (m_scaleTimer / duration) };
	m_scale = 1.0f + sinf(XMConvertToRadians(angle)) * 0.2f;

	if (m_timerState) m_scaleTimer += deltaTime;
	if (m_scaleTimer > duration)
		m_timerState = FALSE;
}

void HPTextObject::SetPlayer(const shared_ptr<Player>& player)
{
	m_player = player;
}

MenuTextObject::MenuTextObject() : m_isMouseOver{ FALSE }, m_scale { 1.0f }, m_scaleTimer{}
{
	m_mouseClickCallBack = []() {};
}

void MenuTextObject::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_LBUTTONDOWN && m_isMouseOver)
		m_mouseClickCallBack();
}

void MenuTextObject::OnMouseEvent(HWND hWnd, FLOAT deltaTime)
{
	RECT c{};
	GetWindowRect(hWnd, &c);

	POINT p{}; GetCursorPos(&p);
	p.x -= c.left;
	p.y -= c.top;

	RECT r{ 
		static_cast<LONG>(m_position.x), 
		static_cast<LONG>(m_position.y),
		static_cast<LONG>(m_position.x + m_rect.right),
		static_cast<LONG>(m_position.y + m_rect.bottom)
	};

	if (r.left <= p.x && p.x <= r.right &&
		r.top <= p.y && p.y <= r.bottom)
		m_isMouseOver = TRUE;
	else
		m_isMouseOver = FALSE;
}

void MenuTextObject::Render(const ComPtr<ID2D1DeviceContext2>& device)
{
	D2D1::Matrix3x2F matrix{ D2D1::Matrix3x2F::Identity() };
	matrix.SetProduct(matrix, D2D1::Matrix3x2F::Scale(m_scale, m_scale, { m_rect.right / 2.0f, m_rect.bottom / 2.0f }));
	matrix.SetProduct(matrix, D2D1::Matrix3x2F::Translation(m_position.x, m_position.y));
	device->SetTransform(matrix);
	device->DrawText(m_text.c_str(), static_cast<UINT32>(m_text.size()), s_formats[m_format].Get(), &m_rect, s_brushes[m_isMouseOver ? m_mouseOverBrush : m_brush].Get());
}

void MenuTextObject::Update(FLOAT deltaTime)
{
	constexpr float maxScale{ 1.2f }, minScale{ 1.0f }, speed{ 1.0f };
	if (m_isMouseOver)
		m_scale = min(maxScale, m_scale + speed * deltaTime);
	else
		m_scale = max(minScale, m_scale - speed * deltaTime);
}

void MenuTextObject::SetMouseOverBrush(const string& brush)
{
	m_mouseOverBrush = brush;
}

void MenuTextObject::SetMouseClickCallBack(const function<void()>& callBackFunc)
{
	m_mouseClickCallBack = callBackFunc;
}
