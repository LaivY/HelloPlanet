#include "stdafx.h"
#include "textObject.h"
#include "audioEngine.h"
#include "camera.h"
#include "framework.h"
#include "player.h"
#include "scene.h"
#include "uiObject.h"

unordered_map<string, ComPtr<ID2D1SolidColorBrush>>	TextObject::s_brushes;
unordered_map<string, ComPtr<IDWriteTextFormat>>	TextObject::s_formats;

TextObject::TextObject() 
	: m_isValid{ TRUE }, m_isMouseOver{ FALSE }, m_rect{ 0.0f, 0.0f, static_cast<float>(g_width), static_cast<float>(g_height) },
	  m_position{}, m_pivotPosition{}, m_pivot{ ePivot::CENTER }, m_screenPivot{ ePivot::CENTER }, m_width{}, m_height{}
{

}

void TextObject::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { }
void TextObject::OnMouseEvent(HWND hWnd, FLOAT deltaTime) { }

void TextObject::Render(const ComPtr<ID2D1DeviceContext2>& device)
{
	device->SetTransform(D2D1::Matrix3x2F::Translation(m_position.x, m_position.y));
	device->DrawText(m_text.c_str(), static_cast<UINT32>(m_text.size()), s_formats[m_format].Get(), &m_rect, s_brushes[m_brush].Get());
}

void TextObject::Update(FLOAT deltaTime) { }

void TextObject::Delete()
{
	m_isValid = FALSE;
}

void TextObject::CalcWidthHeight()
{
	if (m_text.empty()) return;

	ComPtr<IDWriteTextLayout> layout;
	g_gameFramework.GetDWriteFactory()->CreateTextLayout(
		m_text.c_str(),
		static_cast<UINT32>(m_text.size()),
		TextObject::s_formats[m_format].Get(),
		static_cast<float>(g_width),
		static_cast<float>(g_height),
		&layout
	);

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
	if (m_brush.empty()) return;
	if (m_format.empty()) return;

	CalcWidthHeight();
	m_rect = D2D1_RECT_F{ 0.0f, 0.0f, m_width + 1.0f, m_height };
}

void TextObject::SetPivot(const ePivot& pivot)
{
	m_pivot = pivot;
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

	switch (m_pivot)
	{
	case ePivot::LEFTTOP:
		break;
	case ePivot::CENTERTOP:
		m_position.x -= m_width / 2.0f;
		break;
	case ePivot::RIGHTTOP:
		m_position.x -= m_width;
		break;
	case ePivot::LEFTCENTER:
		m_position.y -= m_height / 2.0f;
		break;
	case ePivot::CENTER:
		m_position.x -= m_width / 2.0f;
		m_position.y -= m_height / 2.0f;
		break;
	case ePivot::RIGHTCENTER:
		m_position.x -= m_width;
		m_position.y -= m_height / 2.0f;
		break;
	case ePivot::LEFTBOT:
		m_position.y -= m_height;
		break;
	case ePivot::CENTERBOT:
		m_position.x -= m_width / 2.0f;
		m_position.y -= m_height;
		break;
	case ePivot::RIGHTBOT:
		m_position.x -= m_width;
		m_position.y -= m_height;
		break;
	}
}

void TextObject::SetScreenPivot(const ePivot& pivot)
{
	m_screenPivot = pivot;
}

BOOL TextObject::isValid() const
{
	return m_isValid;
}

wstring TextObject::GetText() const
{
	return m_text;
}

ePivot TextObject::GetPivot() const
{
	return m_pivot;
}

ePivot TextObject::GetScreenPivot() const
{
	return m_screenPivot;
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
	m_player = g_gameFramework.GetScene()->GetPlayer();
}

void BulletTextObject::Render(const ComPtr<ID2D1DeviceContext2>& device)
{
	if (!m_player) return;

	// 플레이어의 총알 수를 표시한다.
	// 포멧은 좌측 하단 정렬이고, 전체 총알 수를 표시한 후 그 왼쪽에 현재 총알 수를 그린다.
	wstring maxBulletCount{ to_wstring(m_player->GetMaxBulletCount()) };
	device->SetTransform(D2D1::Matrix3x2F::Translation(m_position.x, m_position.y));
	device->DrawText(maxBulletCount.c_str(), static_cast<UINT32>(maxBulletCount.size()), s_formats["24R"].Get(), &m_rect, s_brushes["BLACK"].Get());

	m_text = maxBulletCount;
	m_format = "24R";
	CalcWidthHeight();
	float maxBulletTextWidth{ m_width };

	wstring slash{ TEXT("/") };
	device->SetTransform(D2D1::Matrix3x2F::Translation(m_position.x - maxBulletTextWidth, m_position.y));
	device->DrawText(slash.c_str(), static_cast<UINT32>(slash.size()), s_formats["24R"].Get(), &m_rect, s_brushes["BLUE"].Get());

	m_text = slash;
	m_format = "24R";
	CalcWidthHeight();
	float slashTextWidth{ m_width };

	// 현재 총알 텍스트 애니메이션
	m_text = to_wstring(m_bulletCount);
	D2D1::Matrix3x2F matrix{};
	matrix.SetProduct(D2D1::Matrix3x2F::Scale(m_scale, m_scale, { m_rect.right, m_rect.bottom }), D2D1::Matrix3x2F::Translation(m_position.x - maxBulletTextWidth - slashTextWidth, m_position.y));
	device->SetTransform(matrix);
	device->DrawText(m_text.c_str(), static_cast<UINT32>(m_text.size()), s_formats["36R"].Get(), &m_rect, m_bulletCount == 0 ? s_brushes["RED"].Get() : s_brushes["BLACK"].Get());
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
	m_format = "36R";
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

HPTextObject::HPTextObject() : m_hp{ -1 }, m_scale{ 1.0f }, m_timerState{ FALSE }, m_scaleTimer{}
{
	m_rect = { 0.0f, 0.0f, 100.0f, 0.0f };
	m_player = g_gameFramework.GetScene()->GetPlayer();
}

void HPTextObject::Render(const ComPtr<ID2D1DeviceContext2>& device)
{
	if (!m_player) return;

	// 플레이어의 체력을 표시한다.
	// 포멧은 우측 하단 정렬이고, 현재 체력을 표시하고 그 오른쪽에 전체 체력을 표시한다.
	wstring hpText{ to_wstring(m_hp) };
	m_text = hpText;
	m_format = "36L";
	CalcWidthHeight();
	float hpTextWidth{ m_width };

	D2D1::Matrix3x2F matrix{};
	matrix.SetProduct(D2D1::Matrix3x2F::Scale(m_scale, m_scale, { hpTextWidth, m_rect.bottom }), D2D1::Matrix3x2F::Translation(m_position.x, m_position.y));
	device->SetTransform(matrix);
	device->DrawText(hpText.c_str(), static_cast<UINT32>(hpText.size()), s_formats["36L"].Get(), &m_rect, s_brushes["BLACK"].Get());

	wstring slash{ TEXT("/") };
	device->SetTransform(D2D1::Matrix3x2F::Translation(m_position.x + hpTextWidth, m_position.y));
	device->DrawText(slash.c_str(), static_cast<UINT32>(slash.size()), s_formats["24L"].Get(), &m_rect, s_brushes["BLUE"].Get());

	m_text = slash;
	m_format = "24L";
	CalcWidthHeight();
	float slashTextWidth{ m_width };

	wstring maxHpText{ to_wstring(m_player->GetMaxHp()) };
	device->SetTransform(D2D1::Matrix3x2F::Translation(m_position.x + hpTextWidth + slashTextWidth, m_position.y));
	device->DrawText(maxHpText.c_str(), static_cast<UINT32>(maxHpText.size()), s_formats["24L"].Get(), &m_rect, s_brushes["BLACK"].Get());
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
	m_format = "36R";
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

MenuTextObject::MenuTextObject() : m_isMouseOver{ FALSE }, m_scale{ 1.0f }, m_scaleTimer{}, m_value{}
{
	m_mouseClickCallBack = []() {};
}

void MenuTextObject::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_LBUTTONDOWN && m_isMouseOver)
	{
		g_audioEngine.Play("CLICK");
		m_isMouseOver = FALSE;
		m_mouseClickCallBack();
	}
}

void MenuTextObject::OnMouseEvent(HWND hWnd, FLOAT deltaTime)
{
	RECT c{};
	GetWindowRect(hWnd, &c);

	POINT p{}; GetCursorPos(&p);
	p.x -= c.left;
	p.y -= c.top;

	RECT r
	{ 
		static_cast<LONG>(m_position.x), 
		static_cast<LONG>(m_position.y),
		static_cast<LONG>(m_position.x + m_rect.right),
		static_cast<LONG>(m_position.y + m_rect.bottom)
	};

	if (r.left <= p.x && p.x <= r.right &&
		r.top <= p.y && p.y <= r.bottom)
	{
		if (!m_isMouseOver)
			g_audioEngine.Play("HOVER");
		m_isMouseOver = TRUE;
	}
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

void MenuTextObject::SetValue(INT value)
{
	m_value = value;
}

void MenuTextObject::SetMouseOverBrush(const string& brush)
{
	m_mouseOverBrush = brush;
}

void MenuTextObject::SetMouseClickCallBack(const function<void()>& callBackFunc)
{
	m_mouseClickCallBack = callBackFunc;
}

INT MenuTextObject::GetValue() const
{
	return m_value;
}

DamageTextObject::DamageTextObject(const wstring& damage) : m_isOnScreen{ FALSE }, m_startPosition{}, m_scale{}, m_timer{}
{
	SetBrush("BLUE");
	SetFormat("24L");
	SetText(damage);
	m_camera = g_gameFramework.GetScene()->GetCamera();
	m_screenPivot = ePivot::LEFTTOP;
	m_direction = Utile::Random(0, 1);
}

void DamageTextObject::Render(const ComPtr<ID2D1DeviceContext2>& device)
{
	if (!m_isOnScreen) return;
	D2D1::Matrix3x2F matrix{ D2D1::Matrix3x2F::Identity() };
	matrix.SetProduct(matrix, D2D1::Matrix3x2F::Scale(m_scale, m_scale, D2D1_POINT_2F{ m_rect.right / 2.0f, m_rect.bottom / 2.0f }));
	matrix.SetProduct(matrix, D2D1::Matrix3x2F::Translation(m_position.x, m_position.y));
	device->SetTransform(matrix);
	device->DrawText(m_text.c_str(), static_cast<UINT32>(m_text.size()), s_formats[m_format].Get(), &m_rect, s_brushes[m_brush].Get());
}

void DamageTextObject::Update(FLOAT deltaTime)
{
	constexpr float PI{ 3.141592f };
	constexpr float lifeTime{ 0.3f };

	XMFLOAT3 screenPosition{ m_startPosition };
	screenPosition = Vector3::TransformCoord(screenPosition, m_camera->GetViewMatrix());
	screenPosition = Vector3::TransformCoord(screenPosition, m_camera->GetProjMatrix());

	if (screenPosition.x < -1.0f || screenPosition.x > 1.0f ||
		screenPosition.y < -1.0f || screenPosition.y > 1.0f ||
		screenPosition.z < 0.0f || screenPosition.z > 1.0f)
		m_isOnScreen = FALSE;
	else
	{
		// NDC -> 텍스트 좌표계
		screenPosition.x = g_width * (screenPosition.x + 1.0f) / 2.0f;
		screenPosition.y = g_height * (-screenPosition.y + 1.0f) / 2.0f;
		screenPosition.y -= g_height / 50.0f * m_timer;

		SetPosition(XMFLOAT2{ screenPosition.x, screenPosition.y });
		m_isOnScreen = TRUE;
	}

	m_scale = 2.5f * sinf(m_timer / lifeTime * 0.85f * PI + 0.15f * PI);

	m_timer += deltaTime;
	if (m_timer >= lifeTime)
		Delete();
}

void DamageTextObject::SetStartPosition(const XMFLOAT3& position)
{
	m_startPosition = position;
}

SkillGageTextObject::SkillGageTextObject() : m_percent{}
{
	m_player = g_gameFramework.GetScene()->GetPlayer();
	SetBrush("BLACK");
	SetFormat("36R");
}

void SkillGageTextObject::Update(FLOAT deltaTime)
{
	float value{ m_player->GetSkillGage() };
	if (value >= 100.0f)
		SetText(TEXT("Q"));
	else
		SetText(to_wstring(static_cast<int>(m_player->GetSkillGage())));
	SetPosition(GetPivotPosition());
}