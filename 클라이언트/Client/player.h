#pragma once
#include "stdafx.h"
#include "object.h"

#define ROLL_MAX +20
#define ROLL_MIN -10

#define AR 1
#define SG 2
#define MG 3

class Camera;

class Player : public GameObject
{
public:
	Player();
	virtual ~Player() = default;

	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnKeyboardEvent(FLOAT deltaTime);
	virtual void OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnAnimation(const string& animationName, FLOAT currFrame, UINT endFrame);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr);
	virtual void Update(FLOAT deltaTime);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);
	virtual void PlayAnimation(const string& animationName, BOOL doBlending = FALSE);

	void AddVelocity(const XMFLOAT3& increase);
	void SetVelocity(const XMFLOAT3& velocity) { m_velocity = velocity; }
	void SetWeaponType(UINT weaponType) { m_weaponType = weaponType; }
	void SetCamera(const shared_ptr<Camera>& camera) { m_camera = camera; }
	void SetGunMesh(const shared_ptr<Mesh>& mesh) { m_gunMesh = mesh; }
	void SetGunShader(const shared_ptr<Shader>& shader) { m_gunShader = shader; }

	XMFLOAT3 GetVelocity() const { return m_velocity; }
	string GetPureAnimationName(const string& animationName) const;

private:
	XMFLOAT3			m_velocity;		// 속도
	FLOAT				m_maxVelocity;	// 최대속도
	FLOAT				m_friction;		// 마찰력
	UINT				m_weaponType;	// 총 타입

	shared_ptr<Camera>	m_camera;		// 카메라
	shared_ptr<Mesh>	m_gunMesh;		// 총 메쉬
	shared_ptr<Shader>	m_gunShader;	// 총 셰이더
};