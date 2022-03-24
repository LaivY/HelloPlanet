#pragma once
#include "stdafx.h"
#include "object.h"

class Camera;

enum class ePlayerGunType
{
	NONE, AR, SG, MG
};

class Player : public GameObject
{
public:
	Player(BOOL isMultiPlayer = FALSE);
	virtual ~Player() = default;

	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnKeyboardEvent(FLOAT deltaTime);
	virtual void OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnAnimation(FLOAT currFrame, UINT endFrame, BOOL isUpper = FALSE);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr);
	virtual void Update(FLOAT deltaTime);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);
	virtual void PlayAnimation(const string& animationName, BOOL doBlending = FALSE);

	void SendAnimationState() const;

	void AddVelocity(const XMFLOAT3& increase);
	void SetVelocity(const XMFLOAT3& velocity) { m_velocity = velocity; }
	void SetGunType(ePlayerGunType weaponType) { m_gunType = weaponType; }
	void SetCamera(const shared_ptr<Camera>& camera) { m_camera = camera; }
	void SetGunMesh(const shared_ptr<Mesh>& mesh) { m_gunMesh = mesh; }
	void SetGunShader(const shared_ptr<Shader>& shader) { m_gunShader = shader; }

	XMFLOAT3 GetVelocity() const { return m_velocity; }

private:
	void PlayUpperAnimation(const string& animationName, BOOL doBlending = FALSE);
	string GetPureAnimationName(const string& animationName) const;
	string GetCurrAnimationName() const;
	string GetAfterAnimationName() const;
	string GetUpperCurrAnimationName() const;
	string GetUpperAfterAnimationName() const;

private:
	INT					m_id;				// 플레이어 고유 아이디
	BOOL				m_isMultiPlayer;	// 멀티플레이어 여부
	ePlayerGunType		m_gunType;			// 총 타입

	XMFLOAT3			m_velocity;			// 속도
	FLOAT				m_maxVelocity;		// 최대속도
	FLOAT				m_friction;			// 마찰력

	shared_ptr<Camera>	m_camera;			// 카메라
	shared_ptr<Mesh>	m_gunMesh;			// 총 메쉬
	shared_ptr<Shader>	m_gunShader;		// 총 셰이더
};