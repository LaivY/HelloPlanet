#pragma once
#include "stdafx.h"
#include "object.h"
#include "terrain.h"

#define ROLL_MAX +20
#define ROLL_MIN -10

class Camera;

class Player : public GameObject
{
public:
	Player();
	~Player() = default;

	virtual void Update(FLOAT deltaTime);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);

	void SetPlayerInArea();
	void SetPlayerOnTerrain();
	void SetPlayerNormalAndLook();

	void AddVelocity(const XMFLOAT3& increase);
	void SetVelocity(const XMFLOAT3& velocity) { m_velocity = velocity; }
	void SetCamera(const shared_ptr<Camera>& camera) { m_camera = camera; }

	XMFLOAT3 GetVelocity() const { return m_velocity; }

private:
	XMFLOAT3			m_velocity;		// 속도
	FLOAT				m_maxVelocity;	// 최대속도
	FLOAT				m_friction;		// 마찰력
	shared_ptr<Camera>	m_camera;		// 카메라
};