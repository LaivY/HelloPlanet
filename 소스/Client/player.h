#pragma once
#include "stdafx.h"
#include "object.h"

class Camera;

enum class eGunType
{
	NONE, AR, SG, MG
};

class Player : public GameObject
{
public:
	Player(BOOL isMultiPlayer = FALSE);
	virtual ~Player() = default;

	void OnMouseEvent(HWND hWnd, FLOAT deltaTime);
	void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnKeyboardEvent(FLOAT deltaTime);
	void OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnAnimation(FLOAT currFrame, UINT endFrame, BOOL isUpper = FALSE);
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr);
	void Update(FLOAT deltaTime);
	void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);
	void PlayAnimation(const string& animationName, BOOL doBlending = FALSE);
	void PlayUpperAnimation(const string& animationName, BOOL doBlending = FALSE);

	void RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void SendPlayerData() const;
	void ApplyServerData(const PlayerData& playerData);
	void Fire();
	void DelayRotate(FLOAT roll, FLOAT pitch, FLOAT yaw, FLOAT time);

	void SetId(INT id) { m_id = id; }
	void SetHp(INT hp) { m_hp = clamp(hp, 0, m_maxHp); }
	void SetGunType(eGunType gunType);
	void SetCamera(const shared_ptr<Camera>& camera) { m_camera = camera; }
	void SetGunMesh(const shared_ptr<Mesh>& mesh) { m_gunMesh = mesh; }
	void SetGunShader(const shared_ptr<Shader>& shader) { m_gunShader = shader; }
	void SetGunShadowShader(const shared_ptr<Shader>& shadowShader);
	void SetGunOffset(const XMFLOAT3& gunOffset) { m_gunOffset = gunOffset; };

	INT GetId() const;
	eGunType GetGunType() const;
	INT GetHp() const;
	INT GetMaxHp() const;
	INT GetBulletCount() const;
	INT GetMaxBulletCount() const;
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
	INT								m_id;				// 플레이어 고유 아이디
	BOOL							m_isMultiPlayer;	// 멀티플레이어 여부
	BOOL							m_isFired;			// 발사 여부
	eGunType						m_gunType;			// 총 타입

	FLOAT							m_delayRoll;		// 자동으로 회전할 z축 회전각
	FLOAT							m_delayPitch;		// .. x축 회전각
	FLOAT							m_delayYaw;			// .. y축 회전각
	FLOAT							m_delayTime;		// 몇 초에 걸쳐 회전할 건지
	FLOAT							m_delayTimer;		// 타이머

	INT								m_hp;				// 체력
	INT								m_maxHp;			// 최대 체력
	FLOAT							m_speed;			// 속력(실수)
	FLOAT							m_shotSpeed;		// 공격속도
	FLOAT							m_shotTimer;		// 공격속도 타이머
	INT								m_bulletCount;		// 총알 개수
	INT								m_maxBulletCount;	// 총알 최대 개수

	shared_ptr<Camera>				m_camera;			// 카메라
	shared_ptr<Mesh>				m_gunMesh;			// 총 메쉬
	shared_ptr<Shader>				m_gunShader;		// 총 셰이더
	shared_ptr<Shader>				m_gunShadowShader;	// 총 그림자 셰이더
	XMFLOAT3						m_gunOffset;		// 총 그릴 때 카메라의 위치
	FLOAT							m_gunOffsetTimer;	// 총 오프셋 변환에 쓰이는 타이머
};