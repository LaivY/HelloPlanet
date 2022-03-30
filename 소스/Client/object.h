#pragma once
#include "stdafx.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"

class Camera;

class DebugBoundingBox : public BoundingOrientedBox
{
public:
	DebugBoundingBox(const XMFLOAT3& center, const XMFLOAT3& extents, const XMFLOAT4& orientation);
	~DebugBoundingBox() = default;

	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	void SetMesh(const shared_ptr<Mesh>& mesh);
	void SetShader(const shared_ptr<Shader>& shader);

private:
	shared_ptr<Mesh>	m_mesh;
	shared_ptr<Shader>	m_shader;
};

enum class eAnimationState
{
	NONE, PLAY, BLENDING, SYNC
};

struct TextureInfo
{
	TextureInfo() : frame{}, timer{}, interver{ 1.0f / 60.0f }, doRepeat{ TRUE } { }

	INT		frame;
	FLOAT	timer;
	FLOAT	interver;
	BOOL	doRepeat;
};

struct AnimationInfo
{
	AnimationInfo() : currTimer{}, afterTimer{}, blendingTimer{}, state{ eAnimationState::NONE } { }

	eAnimationState	state;
	string			currAnimationName;
	FLOAT			currTimer;
	string			afterAnimationName;
	FLOAT			afterTimer;
	FLOAT			blendingTimer;
};

class GameObject
{
public:
	GameObject();
	virtual ~GameObject() = default;

	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { }
	virtual void OnKeyboardEvent(FLOAT deltaTime) { }
	virtual void OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { }
	virtual void OnAnimation(FLOAT currFrame, UINT endFrame, BOOL isUpper = FALSE);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr);
	virtual void Update(FLOAT deltaTime);
	virtual void Move(const XMFLOAT3& shift);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList);
	virtual void PlayAnimation(const string& animationName, BOOL doBlending = FALSE);

	void SetWorldMatrix(const XMFLOAT4X4& worldMatrix) { m_worldMatrix = worldMatrix; }
	void SetPosition(const XMFLOAT3& position);
	void SetMesh(const shared_ptr<Mesh>& Mesh);
	void SetShader(const shared_ptr<Shader>& shader);
	void SetTexture(const shared_ptr<Texture>& texture);
	void SetTextureInfo(unique_ptr<TextureInfo>& textureInfo);
	void SetBoundingBox(const shared_ptr<DebugBoundingBox>& boundingBox);

	XMFLOAT4X4 GetWorldMatrix() const { return m_worldMatrix; }
	XMFLOAT3 GetRight() const { return XMFLOAT3{ m_worldMatrix._11, m_worldMatrix._12, m_worldMatrix._13 }; }
	XMFLOAT3 GetUp() const { return XMFLOAT3{ m_worldMatrix._21, m_worldMatrix._22, m_worldMatrix._23 }; }
	XMFLOAT3 GetLook() const { return XMFLOAT3{ m_worldMatrix._31, m_worldMatrix._32, m_worldMatrix._33 }; }
	XMFLOAT3 GetPosition() const { return XMFLOAT3{ m_worldMatrix._41, m_worldMatrix._42, m_worldMatrix._43 }; }
	XMFLOAT3 GetRollPitchYaw() const { return XMFLOAT3{ m_roll, m_pitch, m_yaw }; }
	AnimationInfo* GetAnimationInfo() const { return m_animationInfo.get(); }
	AnimationInfo* GetUpperAnimationInfo() const { return m_upperAnimationInfo.get(); }
	BOOL isDeleted() const { return m_isDeleted; }
	shared_ptr<DebugBoundingBox> GetBoundingBox() const { return m_boundingBox; }

protected:
	XMFLOAT4X4						m_worldMatrix;			// 월드 변환 행렬
	FLOAT							m_roll;					// z축 회전각
	FLOAT							m_pitch;				// x축 회전각
	FLOAT							m_yaw;					// y축 회전각

	shared_ptr<Mesh>				m_mesh;					// 메쉬
	shared_ptr<Shader>				m_shader;				// 셰이더
	shared_ptr<Texture>				m_texture;				// 텍스쳐
	unique_ptr<TextureInfo>			m_textureInfo;			// 텍스쳐 정보 구조체
	unique_ptr<AnimationInfo>		m_animationInfo;		// 애니메이션 정보 구조체
	unique_ptr<AnimationInfo>		m_upperAnimationInfo;	// 상체 애니메이션 정보 구조체

	BOOL							m_isDeleted;			// 삭제 여부
	shared_ptr<DebugBoundingBox>	m_boundingBox;			// 바운딩박스
};

class Skybox : public GameObject
{
public:
	Skybox() = default;
	~Skybox() = default;

	void Update(FLOAT deltaTime);
	void SetCamera(const shared_ptr<Camera>& camera) { m_camera = camera; }

private:
	shared_ptr<Camera> m_camera;
};

class Bullet : public GameObject
{
public:
	Bullet(const XMFLOAT3& direction, FLOAT speed = 500.0f, FLOAT lifeTime = 2.0f);
	~Bullet() = default;

	void Update(FLOAT deltaTime);

private:
	XMFLOAT3	m_direction;
	FLOAT		m_speed;
	FLOAT		m_lifeTime;
	FLOAT		m_lifeTimer;
};