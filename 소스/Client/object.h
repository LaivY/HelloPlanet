#pragma once
#include "stdafx.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"

class Camera;
class Player;

enum class eMapObjectType
{
	MOUNTAIN, PLANT, TREE, ROCK1, ROCK2, ROCK3, SMALLROCK, ROCKGROUP1, ROCKGROUP2, DROPSHIP,
	MUSHROOMS, SKULL, RIBS, ROCK4, ROCK5
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
	AnimationInfo() : state{ eAnimationState::NONE }, currTimer{}, afterTimer{}, blendingTimer{}, blendingFrame{ 5 }, fps{ 1.0f / 30.0f } { }

	eAnimationState	state;
	string			currAnimationName;
	FLOAT			currTimer;
	string			afterAnimationName;
	FLOAT			afterTimer;
	FLOAT			blendingTimer;
	INT				blendingFrame;
	FLOAT			fps;
};

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

typedef shared_ptr<DebugBoundingBox> SharedBoundingBox;

class GameObject
{
public:
	GameObject();
	virtual ~GameObject() = default;

	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime) { }
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

	void Delete() { m_isDeleted = TRUE; }
	void SetWorldMatrix(const XMFLOAT4X4& worldMatrix) { m_worldMatrix = worldMatrix; }
	virtual void SetPosition(const XMFLOAT3& position);
	void SetVelocity(const XMFLOAT3& velocity) { m_velocity = velocity; }
	void SetMesh(const shared_ptr<Mesh>& Mesh);
	void SetShader(const shared_ptr<Shader>& shader);
	void SetShadowShader(const shared_ptr<Shader>& shadowShader);
	void SetTexture(const shared_ptr<Texture>& texture);
	void SetTextureInfo(unique_ptr<TextureInfo>& textureInfo);
	void AddBoundingBox(const SharedBoundingBox& boundingBox);

	BOOL isDeleted() const { return m_isDeleted; }
	XMFLOAT4X4 GetWorldMatrix() const { return m_worldMatrix; }
	XMFLOAT3 GetRight() const { return XMFLOAT3{ m_worldMatrix._11, m_worldMatrix._12, m_worldMatrix._13 }; }
	XMFLOAT3 GetUp() const { return XMFLOAT3{ m_worldMatrix._21, m_worldMatrix._22, m_worldMatrix._23 }; }
	XMFLOAT3 GetLook() const { return XMFLOAT3{ m_worldMatrix._31, m_worldMatrix._32, m_worldMatrix._33 }; }
	XMFLOAT3 GetPosition() const { return XMFLOAT3{ m_worldMatrix._41, m_worldMatrix._42, m_worldMatrix._43 }; }
	XMFLOAT3 GetRollPitchYaw() const { return XMFLOAT3{ m_roll, m_pitch, m_yaw }; }
	XMFLOAT3 GetVelocity() const { return m_velocity; }
	const vector<SharedBoundingBox>& GetBoundingBox() const { return m_boundingBoxes; }
	shared_ptr<Shader> GetShadowShader() const { return m_shadowShader; }
	AnimationInfo* GetAnimationInfo() const { return m_animationInfo.get(); }
	AnimationInfo* GetUpperAnimationInfo() const { return m_upperAnimationInfo.get(); }

protected:
	BOOL							m_isDeleted;			// 삭제 여부

	XMFLOAT4X4						m_worldMatrix;			// 월드 변환 행렬
	FLOAT							m_roll;					// z축 회전각
	FLOAT							m_pitch;				// x축 회전각
	FLOAT							m_yaw;					// y축 회전각
	XMFLOAT3						m_velocity;				// 속도
	vector<SharedBoundingBox>		m_boundingBoxes;		// 바운딩박스

	shared_ptr<Mesh>				m_mesh;					// 메쉬
	shared_ptr<Shader>				m_shader;				// 셰이더
	shared_ptr<Shader>				m_shadowShader;			// 그림자 셰이더
	shared_ptr<Texture>				m_texture;				// 텍스쳐
	unique_ptr<TextureInfo>			m_textureInfo;			// 텍스쳐 정보 구조체
	unique_ptr<AnimationInfo>		m_animationInfo;		// 애니메이션 정보 구조체
	unique_ptr<AnimationInfo>		m_upperAnimationInfo;	// 상체 애니메이션 정보 구조체
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
	Bullet(const XMFLOAT3& direction, FLOAT speed = 2000.0f, FLOAT lifeTime = 0.2f);
	~Bullet() = default;

	void Update(FLOAT deltaTime);

private:
	XMFLOAT3	m_direction;
	FLOAT		m_speed;
	FLOAT		m_lifeTime;
	FLOAT		m_lifeTimer;
};

class Monster : public GameObject
{
public:
	Monster() : m_id{ -1 } { }
	~Monster() = default;

	void OnAnimation(FLOAT currFrame, UINT endFrame, BOOL isUpper = FALSE);
	void ApplyServerData(const MonsterData& monsterData);

private:
	INT	m_id;
};