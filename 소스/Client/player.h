﻿#pragma once
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

	void RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList, INT shadowShaderIndex);
	void SendPlayerData() const;
	void ApplyServerData(const PlayerData& playerData);
	//void SendPlayerData(eAnimationType letState) const;

	void SetId(INT id) { m_id = id; }
	void SetGunType(eGunType gunType);
	//void AddVelocity(const XMFLOAT3& increase);
	//void SetVelocity(const XMFLOAT3& velocity) { m_velocity = velocity; }
	void SetCamera(const shared_ptr<Camera>& camera) { m_camera = camera; }
	void SetGunMesh(const shared_ptr<Mesh>& mesh) { m_gunMesh = mesh; }
	void SetGunShader(const shared_ptr<Shader>& shader) { m_gunShader = shader; }
	void SetGunShadowShader(const shared_ptr<Shader>& sShader, const shared_ptr<Shader>& mShader, const shared_ptr<Shader>& lShader, const shared_ptr<Shader>& allShader);

	INT GetId() const { return m_id; }
	XMFLOAT3 GetVelocity() const { return m_velocity; }
	string GetPureAnimationName(const string& animationName) const;
	string GetCurrAnimationName() const;
	string GetAfterAnimationName() const;
	string GetUpperCurrAnimationName() const;
	string GetUpperAfterAnimationName() const;

private:
	INT								m_id;				// 플레이어 고유 아이디
	BOOL							m_isMultiPlayer;	// 멀티플레이어 여부
	eGunType						m_gunType;			// 총 타입

	FLOAT							m_speed;			// 속력(실수)
	//XMFLOAT3						m_velocity;			// 속도(벡터)
	//FLOAT							m_maxVelocity;		// 최대속도
	//FLOAT							m_friction;			// 마찰력
	FLOAT							m_shotSpeed;		// 공격속도
	FLOAT							m_shotTimer;		// 공격속도 타이머

	shared_ptr<Camera>				m_camera;			// 카메라
	shared_ptr<Mesh>				m_gunMesh;			// 총 메쉬
	shared_ptr<Shader>				m_gunShader;		// 총 셰이더
	array<shared_ptr<Shader>, 
		  Setting::SHADOWMAP_COUNT>	m_gunShadowShaders;	// 총 그림자 셰이더
};