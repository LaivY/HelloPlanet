#pragma once
#include "stdafx.h"
#include "player.h"
#include "uiObject.h"

class TextObject
{
public:
	TextObject();
	~TextObject() = default;

	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime);

	virtual void Render(const ComPtr<ID2D1DeviceContext2>& device);
	virtual void Update(FLOAT deltaTime);

	void CalcWidthHeight();
	void SetRect(const D2D1_RECT_F& rect);
	void SetBrush(const string& brush);
	void SetFormat(const string& format);
	void SetText(const wstring& text);
	void SetScreenPivot(const ePivot& pivot);
	void SetPosition(const XMFLOAT2& position);

	BOOL isDeleted() const;
	wstring GetText() const;
	XMFLOAT2 GetPosition() const;
	XMFLOAT2 GetPivotPosition() const;
	FLOAT GetWidth() const;
	FLOAT GetHeight() const;

	static unordered_map<string, ComPtr<ID2D1SolidColorBrush>>	s_brushes;
	static unordered_map<string, ComPtr<IDWriteTextFormat>>		s_formats;

protected:
	BOOL		m_isDeleted;
	BOOL		m_isMouseOver;

	D2D1_RECT_F	m_rect;
	string		m_brush;
	string		m_format;
	wstring		m_text;

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

	void Render(const ComPtr<ID2D1DeviceContext2>& device);
	void Update(FLOAT deltaTime);

	void SetText(const wstring&) = delete;
	void SetPlayer(const shared_ptr<Player>& player);

private:
	shared_ptr<Player>	m_player;
	INT					m_bulletCount;
	FLOAT				m_scale;
	BOOL				m_timerState;
	FLOAT				m_scaleTimer;
};

class HPTextObject : public TextObject
{
public:
	HPTextObject();
	~HPTextObject() = default;

	void Render(const ComPtr<ID2D1DeviceContext2>& device);
	void Update(FLOAT deltaTime);

	void SetText(const wstring&) = delete;
	void SetPlayer(const shared_ptr<Player>& player);

private:
	shared_ptr<Player>	m_player;
	INT					m_hp;
	FLOAT				m_scale;
	BOOL				m_timerState;
	FLOAT				m_scaleTimer;
};

class MenuTextObject : public TextObject
{
public:
	MenuTextObject();
	~MenuTextObject() = default;

	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime);

	void Render(const ComPtr<ID2D1DeviceContext2>& device);
	void Update(FLOAT deltaTime);

	void SetMouseOverBrush(const string& brush);

private:
	BOOL	m_isMouseOver;
	FLOAT	m_scale;
	FLOAT	m_scaleTimer;
	string	m_mouseOverBrush;
};