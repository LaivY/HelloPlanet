#pragma once
#include "object.h"

class Camera;

enum class eReward
{
	DAMAGE, SPEED, MAXHP, MAXBULLET, SPECIAL
};

class Player : public GameObject
{
public:
	Player(BOOL isMultiPlayer = FALSE);
	virtual ~Player() = default;

	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime);
	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnKeyboardEvent(FLOAT deltaTime);
	virtual void OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnAnimation(FLOAT currFrame, UINT endFrame);
	virtual void OnUpperAnimation(FLOAT currFrame, UINT endFrame);

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr);
	virtual void Update(FLOAT deltaTime);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);
	virtual void PlayAnimation(const string& animationName, BOOL doBlending = FALSE);

	// 애니메이션
	void PlayUpperAnimation(const string& animationName, BOOL doBlending = FALSE);
	void DeleteUpperAnimation();

	void RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void DelayRotate(FLOAT roll, FLOAT pitch, FLOAT yaw, FLOAT time);
	void Fire();

	// 통신
	void SendPlayerData() const;
	void ApplyServerData(const PlayerData& playerData);

	void SetId(INT id) { m_id = id; }
	void SetIsMultiplayer(BOOL isMultiPlayer);
	void SetHp(INT hp);
	void SetDamage(INT damage) { m_damage = damage; }
	void SetWeaponType(eWeaponType gunType);
	void SetCamera(Camera* camera) { m_camera = camera; }
	void SetGunMesh(const shared_ptr<Mesh>& mesh) { m_gunMesh = mesh; }
	void SetGunShader(const shared_ptr<Shader>& shader) { m_gunShader = shader; }
	void SetGunShadowShader(const shared_ptr<Shader>& shadowShader);
	void SetGunOffset(const XMFLOAT3& gunOffset) { m_gunOffset = gunOffset; };
	void SetSkillGage(FLOAT value);

	void AddMaxHp(INT hp);
	void AddBonusSpeed(INT speed);
	void AddBonusDamage(INT damage);
	void AddBonusAttackSpeed(INT speed);
	void AddMaxBulletCount(INT count);
	void AddBonusReloadSpeed(INT speed);
	void AddBonusBulletFire(INT count);
	void SetAutoTarget(INT targetId);

	INT GetId() const;
	BOOL GetIsFocusing() const;
	BOOL GetIsSkillActive() const;
	eWeaponType GetWeaponType() const;
	INT GetHp() const;
	INT GetMaxHp() const;
	INT GetDamage() const;
	INT GetBulletCount() const;
	INT GetMaxBulletCount() const;
	FLOAT GetSkillGage() const;
	string GetPureAnimationName(const string& animationName) const;
	string GetCurrAnimationName() const;
	string GetAfterAnimationName() const;
	string GetUpperCurrAnimationName() const;
	string GetUpperAfterAnimationName() const;
	eAnimationType GetAnimationType() const;
	eUpperAnimationType GetUpperAnimationType() const;
	XMFLOAT3 GetGunOffset() const;
	FLOAT GetGunOffsetTimer() const;

private:
	void OnSkillActive();
	void OnSkillInactive();

	void UpdateZoomInOut(FLOAT deltaTime);
	void UpdateSkill(FLOAT deltaTime);

private:
	INT					m_id;				// 플레이어 고유 아이디
	BOOL				m_isMultiPlayer;	// 멀티플레이어 여부
	BOOL				m_isFired;			// 발사 여부(총소리)
	BOOL				m_isMoved;			// 이동 여부(발소리)
	bool				m_isInvincible;		// 무적 여부

	bool				m_isFocusing;		// 확대 조준 중인지
	bool				m_isZooming;		// 줌인, 줌아웃 중인지
	bool				m_isZoomIn;			// 줌인 중인지
	FLOAT				m_zoomTimer;		// 줌 타이머

	eWeaponType			m_weaponType;		// 총 타입
	INT					m_hp;				// 현재 체력
	INT					m_maxHp;			// 최대 체력
	FLOAT				m_speed;			// 이동 속력
	INT					m_damage;			// 공격력
	FLOAT				m_attackSpeed;		// 공격속도
	FLOAT				m_attackTimer;		// 공격속도 타이머
	INT					m_bulletCount;		// 총알 개수
	INT					m_maxBulletCount;	// 총알 최대 개수

	INT					m_bonusSpeed;		// 추가 이동속도(+n%)
	INT					m_bonusDamage;		// 추가 공격력(+n)
	INT					m_bonusAttackSpeed;	// 추가 공격속도(+n%)
	INT					m_bonusReloadSpeed;	// 추가 재장전 속도(+n%)
	INT					m_bonusBulletFire;	// 추가 발사 수(+n)

	BOOL				m_isSkillActive;	// 스킬 활성화 여부
	FLOAT				m_skillActiveTime;	// 스킬 지속 시간
	FLOAT				m_skillGage;		// 스킬 게이지 양
	FLOAT				m_skillGageTimer;	// 자동으로 스킬 게이지를 채우기 위한 타이머
	INT					m_autoTargetMobId;	// AR 오토 타겟 대상 몬스터 id

	FLOAT				m_delayRoll;		// 자동으로 회전할 z축 회전각
	FLOAT				m_delayPitch;		// .. x축 회전각
	FLOAT				m_delayYaw;			// .. y축 회전각
	FLOAT				m_delayTime;		// 몇 초에 걸쳐 회전할 건지
	FLOAT				m_delayTimer;		// 타이머

	shared_ptr<Mesh>	m_gunMesh;			// 총 메쉬
	shared_ptr<Shader>	m_gunShader;		// 총 셰이더
	shared_ptr<Shader>	m_gunShadowShader;	// 총 그림자 셰이더
	XMFLOAT3			m_gunOffset;		// 총 그릴 때 카메라의 위치
	FLOAT				m_gunOffsetTimer;	// 총 오프셋 변환에 쓰이는 타이머

	Camera*				m_camera;			// 카메라
};