#pragma once

class Camera;
class Mesh;
class ParticleMesh;
class Player;
class Shader;
class Texture;
class Hitbox;

enum class eAnimationState
{
	NONE, PLAY, BLENDING, SYNC
};

struct TextureInfo
{
	TextureInfo() : frame{}, timer{}, interver{ 1.0f / 60.0f }, loop{ TRUE } { }

	INT		frame;
	FLOAT	timer;
	FLOAT	interver;
	BOOL	loop;
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

class GameObject
{
public:
	GameObject();
	virtual ~GameObject() = default;

	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime);
	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnKeyboardEvent(FLOAT deltaTime);
	virtual void OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnAnimation(FLOAT currFrame, UINT endFrame);
	virtual void OnUpperAnimation(FLOAT currFrame, UINT endFrame);

	virtual void Update(FLOAT deltaTime);
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr);
	virtual void Move(const XMFLOAT3& shift);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);
	virtual void PlayAnimation(const string& animationName, BOOL doBlending = FALSE);

	void Delete();
	void SetOutline(BOOL isMakeOutline);
	void SetWorldMatrix(const XMFLOAT4X4& worldMatrix);
	void SetPosition(const XMFLOAT3& position);
	void SetScale(const XMFLOAT3& scale);
	void SetVelocity(const XMFLOAT3& velocity);
	void SetMesh(const shared_ptr<Mesh>& Mesh);
	void SetShader(const shared_ptr<Shader>& shader);
	void SetShadowShader(const shared_ptr<Shader>& shadowShader);
	void SetTexture(const shared_ptr<Texture>& texture);
	void SetTextureInfo(unique_ptr<TextureInfo>& textureInfo);
	void AddHitbox(unique_ptr<Hitbox>& hitbox);

	BOOL isValid() const;
	BOOL isMakeOutline() const;
	XMFLOAT4X4 GetWorldMatrix() const;
	XMFLOAT3 GetRollPitchYaw() const;
	XMFLOAT3 GetRight() const;
	XMFLOAT3 GetUp() const;
	XMFLOAT3 GetLook() const;
	XMFLOAT3 GetPosition() const;
	XMFLOAT3 GetScale() const;
	XMFLOAT3 GetVelocity() const;
	const vector<unique_ptr<Hitbox>>& GetHitboxes() const;
	shared_ptr<Shader> GetShadowShader() const;
	AnimationInfo* GetAnimationInfo() const;
	AnimationInfo* GetUpperAnimationInfo() const;

protected:
	BOOL							m_isValid;			    // 유효 여부
	BOOL							m_isMakeOutline;		// 외곽선 여부

	XMFLOAT4X4						m_worldMatrix;			// 월드 변환 행렬
	FLOAT							m_roll;					// z축 회전각
	FLOAT							m_pitch;				// x축 회전각
	FLOAT							m_yaw;					// y축 회전각
	XMFLOAT3						m_scale;				// 스케일
	XMFLOAT3						m_velocity;				// 속도
	vector<unique_ptr<Hitbox>>		m_hitboxes;				// 히트박스

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
	Skybox();
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
	Monster(INT id, eMobType type);
	~Monster() = default;

	virtual void OnAnimation(FLOAT currFrame, UINT endFrame);
	virtual void ApplyServerData(const MonsterData& monsterData);

	INT GetId() const;
	eMobType GetType() const;
	INT GetDamage() const;

protected:
	INT			m_id;
	eMobType	m_type;
	INT			m_damage;
	FLOAT		m_atkFrame;
	FLOAT		m_atkRange;
	BOOL		m_isAttacked;
};

class BossMonster : public Monster
{
public:
	BossMonster(INT id);
	~BossMonster() = default;

	virtual void OnAnimation(FLOAT currFrame, UINT endFrame);

private:
	BOOL m_isRoared;
};

class OutlineObject : public GameObject
{
public:
	OutlineObject(const XMFLOAT3& color, FLOAT thickness);
	~OutlineObject() = default;

	void SetColor(const XMFLOAT3& color);
	void SetThickness(FLOAT thickness);
};

class DustParticle : public GameObject
{
public:
	DustParticle();
	~DustParticle() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr);
};

class Hitbox
{
public:
	Hitbox(const XMFLOAT3& center, const XMFLOAT3& extents, const XMFLOAT3& rollPitchYaw = XMFLOAT3{ 0.0f, 0.0f, 0.0f });
	~Hitbox() = default;

	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	void Update(FLOAT /*deltaTime*/);

	void SetOwner(GameObject* gameObject);

	BoundingOrientedBox GetBoundingBox() const;

private:
	GameObject*				m_owner;
	unique_ptr<GameObject>	m_hitbox;
	XMFLOAT3				m_center;
	XMFLOAT3				m_extents;
	XMFLOAT3				m_rollPitchYaw;
};