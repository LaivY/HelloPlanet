#pragma once

class Camera;
class Player;
enum class ePivot;

class TextObject
{
public:
	TextObject();
	~TextObject() = default;

	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime);

	virtual void Render(const ComPtr<ID2D1DeviceContext2>& device);
	virtual void Update(FLOAT deltaTime);

	void Delete();

	void CalcWidthHeight();
	void SetRect(const D2D1_RECT_F& rect);
	void SetBrush(const string& brush);
	void SetFormat(const string& format);
	void SetText(const wstring& text);
	void SetPivot(const ePivot& pivot);
	void SetScreenPivot(const ePivot& pivot);
	void SetPosition(const XMFLOAT2& position);

	BOOL isValid() const;
	wstring GetText() const;
	ePivot GetPivot() const;
	ePivot GetScreenPivot() const;
	XMFLOAT2 GetPosition() const;
	XMFLOAT2 GetPivotPosition() const;
	FLOAT GetWidth() const;
	FLOAT GetHeight() const;

	static unordered_map<string, ComPtr<ID2D1SolidColorBrush>>	s_brushes;
	static unordered_map<string, ComPtr<IDWriteTextFormat>>		s_formats;

protected:
	BOOL		m_isValid;
	BOOL		m_isMouseOver;

	D2D1_RECT_F	m_rect;
	string		m_brush;
	string		m_format;
	wstring		m_text;

	ePivot		m_pivot;
	ePivot		m_screenPivot;
	XMFLOAT2	m_position;
	XMFLOAT2	m_pivotPosition;
	FLOAT		m_width;
	FLOAT		m_height;
};

class BulletTextObject : public TextObject
{
public:
	BulletTextObject();
	~BulletTextObject() = default;

	virtual void Render(const ComPtr<ID2D1DeviceContext2>& device);
	virtual void Update(FLOAT deltaTime);

	void SetText(const wstring&) = delete;

private:
	Player*	m_player;
	INT		m_bulletCount;
	FLOAT	m_scale;
	BOOL	m_timerState;
	FLOAT	m_scaleTimer;
};

class HPTextObject : public TextObject
{
public:
	HPTextObject();
	~HPTextObject() = default;

	virtual void Render(const ComPtr<ID2D1DeviceContext2>& device);
	virtual void Update(FLOAT deltaTime);

	void SetText(const wstring&) = delete;

private:
	Player*	m_player;
	INT		m_hp;
	FLOAT	m_scale;
	BOOL	m_timerState;
	FLOAT	m_scaleTimer;
};

class MenuTextObject : public TextObject
{
public:
	MenuTextObject();
	~MenuTextObject() = default;

	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime);

	virtual void Render(const ComPtr<ID2D1DeviceContext2>& device);
	virtual void Update(FLOAT deltaTime);

	void SetValue(INT value);
	void SetMouseOverBrush(const string& brush);
	void SetMouseClickCallBack(const function<void()>& callBackFunc);

	INT GetValue() const;

private:
	BOOL				m_isMouseOver;
	FLOAT				m_scale;
	FLOAT				m_scaleTimer;
	INT					m_value;
	string				m_mouseOverBrush;
	function<void()>	m_mouseClickCallBack;
};

class DamageTextObject : public TextObject
{
public:
	DamageTextObject(const wstring& damage);
	~DamageTextObject() = default;

	virtual void Render(const ComPtr<ID2D1DeviceContext2>& device);
	virtual void Update(FLOAT deltaTime);

	void SetStartPosition(const XMFLOAT3& position);

private:
	Camera*		m_camera;
	BOOL		m_isOnScreen;
	BOOL		m_direction;
	XMFLOAT3	m_startPosition;
	FLOAT		m_scale;
	FLOAT		m_timer;
};

class SkillGageTextObject : public TextObject
{
public:
	SkillGageTextObject();
	~SkillGageTextObject() = default;

	virtual void Update(FLOAT deltaTime);

private:
	Player*	m_player;
	INT		m_percent;
};