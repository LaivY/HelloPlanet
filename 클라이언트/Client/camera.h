#pragma once
#include "stdafx.h"
#include "player.h"

#define MAX_PITCH +60
#define MIN_PITCH -80

struct cbCamera
{
	XMFLOAT4X4	viewMatrix;
	XMFLOAT4X4	projMatrix;
	XMFLOAT3	eye;
};

class Camera
{
public:
	Camera();
	~Camera();

	virtual void Update(FLOAT deltaTime);
	void Move(const XMFLOAT3& shift);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);
	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList);

	void SetViewMatrix(const XMFLOAT4X4& viewMatrix) { m_viewMatrix = viewMatrix; }
	void SetProjMatrix(const XMFLOAT4X4& projMatrix) { m_projMatrix = projMatrix; }
	void SetEye(const XMFLOAT3& eye) { m_eye = eye; }
	void SetAt(const XMFLOAT3& at) { m_at = at; }
	void SetUp(const XMFLOAT3& up) { m_up = up; }
	void SetPlayer(const shared_ptr<Player>& player);
	void SetTerrain(HeightMapTerrain* terrain) { m_terrain = terrain; }

	XMFLOAT4X4 GetViewMatrix() const { return m_viewMatrix; }
	XMFLOAT4X4 GetProjMatrix() const { return m_projMatrix; }
	XMFLOAT3 GetEye() const { return m_eye; }
	XMFLOAT3 GetAt() const { return m_at; }
	XMFLOAT3 GetUp() const { return m_up; }

protected:
	XMFLOAT4X4				m_viewMatrix;	// 뷰변환 행렬
	XMFLOAT4X4				m_projMatrix;	// 투영변환 행렬

	XMFLOAT3				m_eye;			// 카메라 위치
	XMFLOAT3				m_at;			// 카메라가 바라보는 방향
	XMFLOAT3				m_up;			// 카메라 Up벡터

	FLOAT					m_roll;			// x축 회전각
	FLOAT					m_pitch;		// y축 회전각
	FLOAT					m_yaw;			// z축 회전각

	shared_ptr<Player>		m_player;		// 플레이어
	HeightMapTerrain*		m_terrain;		// 카메라가 위치해있는 지형

	ComPtr<ID3D12Resource>	m_cbCamera;		// 상수 버퍼
	cbCamera*				m_pcbCamera;	// 상수 버퍼 포인터
};

class ThirdPersonCamera : public Camera
{
public:
	ThirdPersonCamera();
	~ThirdPersonCamera() = default;

	virtual void Update(FLOAT deltaTime);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);

	XMFLOAT3 GetOffset() const { return m_offset; }
	FLOAT GetDistance() const { return m_distance; }

	void SetOffset(const XMFLOAT3& offset) { m_offset = offset; }
	void SetDistance(FLOAT distance) { m_distance = clamp(distance, 3.0f, 20.0f); }
	void SetDelay(FLOAT delay) { m_delay = delay; }

private:
	XMFLOAT3	m_offset;	// 플레이어로부터 떨어져있는 위치
	FLOAT		m_distance;	// 오프셋 방향으로 떨어진 거리
	FLOAT		m_delay;	// 움직임 딜레이 (0.0 ~ 1.0)
};