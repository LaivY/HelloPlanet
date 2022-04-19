#pragma once
#include "stdafx.h"
#include "object.h"
#include "player.h"

enum class ePivot
{
	LEFTTOP, CENTERTOP, RIGHTTOP,
	LEFTCENTER, CENTER, RIGHTCENTER,
	LEFTBOT, CENTERBOT, RIGHTBOT
};

class UIObject : public GameObject
{
public:
	UIObject();
	UIObject(FLOAT width, FLOAT height);
	virtual ~UIObject() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr);
	virtual void Update(FLOAT /*deltaTime*/) { }

	void SetPosition(const XMFLOAT3& position);
	void SetPosition(FLOAT x, FLOAT y);
	void SetPivot(const ePivot& pivot) { m_pivot = pivot; }
	void SetWidth(FLOAT width);
	void SetHeight(FLOAT height);

protected:
	ePivot	m_pivot;
	FLOAT	m_width;
	FLOAT	m_height;
};

class CrosshairUIObject : public UIObject
{
public:
	CrosshairUIObject(FLOAT width, FLOAT height);
	~CrosshairUIObject() = default;

	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr);
	void Update(FLOAT deltaTime);

	void SetMesh(shared_ptr<Mesh>& mesh);
	void SetPlayer(const shared_ptr<Player>& player);

private:
	shared_ptr<Player>				m_player;
	array<unique_ptr<UIObject>, 4>	m_lines;
	FLOAT							m_radius;
	FLOAT							m_timer;
};

class HpUIObject : public UIObject
{
public:
	HpUIObject(FLOAT width, FLOAT height);
	~HpUIObject() = default;

	void Update(FLOAT deltaTime);

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