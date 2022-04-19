#include "textObject.h"
#include "framework.h"

unordered_map<string, ComPtr<ID2D1SolidColorBrush>>	TextObject::s_brushes;
unordered_map<string, ComPtr<IDWriteTextFormat>>	TextObject::s_formats;

TextObject::TextObject() : m_isDeleted{ FALSE }, m_rect{ 0.0f, 0.0f, static_cast<float>(Setting::SCREEN_WIDTH), static_cast<float>(Setting::SCREEN_HEIGHT) }, m_position{}, m_width{}, m_height{}
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
	g_gameFramework.GetDWriteFactory()->CreateTextLayout(m_text.c_str(), static_cast<UINT32>(m_text.size()), TextObject::s_formats[m_format].Get(), Setting::SCREEN_WIDTH, Setting::SCREEN_HEIGHT, &layout);

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
}

void TextObject::SetPosition(const XMFLOAT2& position)
{
	m_position = position;
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

	// �÷��̾��� �Ѿ� ���� ǥ���Ѵ�.
	// ������ ���� �ϴ� �����̰�, ��ü �Ѿ� ���� ǥ���� �� �� ���ʿ� ���� �Ѿ� ���� �׸���.
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

	// ���� �Ѿ� �ؽ�Ʈ �ִϸ��̼�
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

	// �Ѿ� �� ���Կ� ���� �簢�� ���� ����
	m_text = to_wstring(bulletCount);
	m_format = "BULLETCOUNT";
	CalcWidthHeight();
	float width{ m_width };
	m_rect.bottom = m_height;

	// ������
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

	// �÷��̾��� �Ѿ� ���� ǥ���Ѵ�.
	// ������ ���� �ϴ� �����̰�, ��ü �Ѿ� ���� ǥ���� �� �� ���ʿ� ���� �Ѿ� ���� �׸���.
	wstring maxHpText{ to_wstring(m_player->GetMaxHp()) };
	device->SetTransform(D2D1::Matrix3x2F::Translation(m_position.x, m_position.y));
	device->DrawText(maxHpText.c_str(), static_cast<UINT32>(maxHpText.size()), s_formats["MAXBULLETCOUNT"].Get(), &m_rect, s_brushes["BLACK"].Get());

	m_text = maxHpText;
	m_format = "MAXBULLETCOUNT";
	CalcWidthHeight();
	float maxHpTextWidth{ m_width };

	wstring slash{ TEXT("/") };
	device->SetTransform(D2D1::Matrix3x2F::Translation(m_position.x - maxHpTextWidth, m_position.y));
	device->DrawText(slash.c_str(), static_cast<UINT32>(slash.size()), s_formats["MAXBULLETCOUNT"].Get(), &m_rect, s_brushes["BLUE"].Get());

	m_text = slash;
	m_format = "MAXBULLETCOUNT";
	CalcWidthHeight();
	float slashTextWidth{ m_width };

	// ���� �Ѿ� �ؽ�Ʈ �ִϸ��̼�
	m_text = to_wstring(m_hp);
	D2D1::Matrix3x2F matrix{};
	matrix.SetProduct(D2D1::Matrix3x2F::Scale(m_scale, m_scale, { m_rect.right, m_rect.bottom }), D2D1::Matrix3x2F::Translation(m_position.x - maxHpTextWidth - slashTextWidth, m_position.y));
	device->SetTransform(matrix);
	device->DrawText(m_text.c_str(), static_cast<UINT32>(m_text.size()), s_formats["BULLETCOUNT"].Get(), &m_rect, m_hp == 0 ? s_brushes["RED"].Get() : s_brushes["BLACK"].Get());
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

	// ü�� ��ġ�� ���Կ� ���� �簢�� ���� ����
	m_text = to_wstring(hp);
	m_format = "BULLETCOUNT";
	CalcWidthHeight();
	float width{ m_width };
	m_rect.bottom = m_height;

	// ������
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
