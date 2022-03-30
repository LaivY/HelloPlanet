#pragma once
#include "stdafx.h"
#include "camera.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"

enum class eUIPivot
{
	LEFTTOP, CENTERTOP, RIGHTTOP,
	LEFTCENTER, CENTER, RIGHTCENTER,
	LEFTBOT, CENTERBOT, RIGHTBOT
};

class UIObject
{
public:
	UIObject(const XMFLOAT2& size);
	virtual ~UIObject() = default;

	virtual void Update(FLOAT deltaTime);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

	XMFLOAT2 GetPosition() const { return m_position; }
	XMFLOAT2 GetSize() const { return m_size; }

	void SetPivot(eUIPivot pivot);
	void SetPosition(const XMFLOAT2& position);
	void SetTexture(const shared_ptr<Texture>& texture);

private:
	eUIPivot			m_pivot;
	XMFLOAT2			m_position;
	XMFLOAT2			m_size;
	unique_ptr<Mesh>	m_mesh;
	shared_ptr<Texture> m_texture;
};

class UI
{
public:
	UI() = default;
	~UI() = default;

	void Update(FLOAT deltaTime);
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

	void SetCamera(const shared_ptr<Camera>& camera) { m_camera = camera; }
	void SetShader(const shared_ptr<Shader>& shader) { m_shader = shader; }

private:
	vector<unique_ptr<UIObject>>	m_uiObjects;
	shared_ptr<Camera>				m_camera;
	shared_ptr<Shader>				m_shader;
};