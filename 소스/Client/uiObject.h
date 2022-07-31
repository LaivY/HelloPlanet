#pragma once
#include "object.h"

enum class ePivot
{
	LEFTTOP,	CENTERTOP,	RIGHTTOP,
	LEFTCENTER, CENTER,		RIGHTCENTER,
	LEFTBOT,	CENTERBOT,	RIGHTBOT
};

class UIObject : public GameObject
{
public:
	UIObject(FLOAT width, FLOAT height);
	virtual ~UIObject() = default;

	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr);
	virtual void Update(FLOAT deltaTime);

	void SetFitToScreen(BOOL fitToScreen);
	void SetPosition(const XMFLOAT2& position);
	void SetPivot(const ePivot& pivot);
	void SetScreenPivot(const ePivot& pivot);
	void SetScale(const XMFLOAT2& scale);
	void SetWidth(FLOAT width);
	void SetHeight(FLOAT height);

	ePivot GetPivot() const;
	ePivot GetScreenPivot() const;
	XMFLOAT2 GetPivotPosition() const;
	FLOAT GetWidth() const;
	FLOAT GetHeight() const;

protected:
	BOOL		m_isFitToScreen;
	ePivot		m_pivot;
	ePivot		m_screenPivot;
	XMFLOAT2	m_pivotPosition;
	XMFLOAT2	m_scale;
	FLOAT		m_width;
	FLOAT		m_height;
};

class CrosshairUIObject : public UIObject
{
public:
	CrosshairUIObject(FLOAT width, FLOAT height);
	~CrosshairUIObject() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr);
	virtual void Update(FLOAT deltaTime);

	void SetMesh(shared_ptr<Mesh>& mesh);
	void SetPlayer(const shared_ptr<Player>& player);

private:
	shared_ptr<Player>				m_player;
	array<unique_ptr<UIObject>, 4>	m_lines;
	eWeaponType						m_weaponType;
	INT								m_bulletCount;
	FLOAT							m_radius;
	FLOAT							m_timer;
};

class HpUIObject : public UIObject
{
public:
	HpUIObject(FLOAT width, FLOAT height);
	~HpUIObject() = default;

	virtual void Update(FLOAT deltaTime);

	void SetPlayer(const shared_ptr<Player>& player);

private:
	shared_ptr<Player>	m_player;
	INT					m_hp;
	INT					m_maxHp;
	FLOAT				m_deltaHp;
	FLOAT				m_originWidth;
	BOOL				m_timerState;
	FLOAT				m_timer;
};

class MenuUIObject : public UIObject
{
public:
	MenuUIObject(FLOAT width, FLOAT height);
	~MenuUIObject() = default;

	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime);

	virtual void Update(FLOAT deltaTime);

	void SetMouseClickCallBack(const function<void()>& callBackFunc);

protected:
	BOOL				m_isMouseOver;
	function<void()>	m_mouseClickCallBack;
};

class RewardUIObject : public MenuUIObject
{
public:
	RewardUIObject(FLOAT width, FLOAT height);
	~RewardUIObject() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr);
	virtual void Update(FLOAT deltaTime);

private:
	FLOAT m_timer;
};

class HitUIObject : public UIObject
{
public:
	HitUIObject(int monsterId);
	~HitUIObject() = default;

	virtual void Update(FLOAT deltaTime);

private:
	INT		m_monsterId;
	FLOAT	m_angle;
	FLOAT	m_timer;
};

class AutoTargetUIObject : public UIObject
{
public:
	AutoTargetUIObject();
	~AutoTargetUIObject() = default;

	virtual void Update(FLOAT deltaTime);

private:
	Player* m_player;
	Camera* m_camera;
};

class SkillGageUIObject : public UIObject
{
public:
	SkillGageUIObject();
	~SkillGageUIObject() = default;

	virtual void Update(FLOAT deltaTime);

private:
	Player* m_player;
};

class WarningUIObject : public UIObject
{
public:
	WarningUIObject();
	~WarningUIObject() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr);
	virtual void Update(FLOAT deltaTime);

private:
	Player* m_player;
	FLOAT	m_timer;
	BOOL	m_warning;
};