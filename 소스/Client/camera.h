#pragma once

class GameObject;
class Player;

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
	virtual ~Camera();

	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void Update(FLOAT deltaTime);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);

	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void UpdateShaderVariableByPlayer(const ComPtr<ID3D12GraphicsCommandList>& commandList);

	void Move(const XMFLOAT3& shift);

	void SetViewMatrix(const XMFLOAT4X4& viewMatrix) { m_viewMatrix = viewMatrix; }
	void SetProjMatrix(const XMFLOAT4X4& projMatrix) { m_projMatrix = projMatrix; }
	void SetEye(const XMFLOAT3& eye) { m_eye = eye; }
	void SetAt(const XMFLOAT3& at) { m_at = at; }
	void SetUp(const XMFLOAT3& up) { m_up = up; }
	void SetOffset(const XMFLOAT3& offset) { m_offset = offset; }
	void SetPlayer(Player* player) { m_player = player; }

	XMFLOAT4X4 GetViewMatrix() const { return m_viewMatrix; }
	XMFLOAT4X4 GetProjMatrix() const { return m_projMatrix; }
	XMFLOAT3 GetEye() const { return m_eye; }
	XMFLOAT3 GetAt() const { return m_at; }
	XMFLOAT3 GetUp() const { return m_up; }
	XMFLOAT3 GetOffset() const { return m_offset; }

protected:
	XMFLOAT4X4				m_viewMatrix;	// 뷰변환 행렬
	XMFLOAT4X4				m_projMatrix;	// 투영변환 행렬

	ComPtr<ID3D12Resource>	m_cbCamera;		// 상수 버퍼
	cbCamera*				m_pcbCamera;	// 상수 버퍼 포인터

	ComPtr<ID3D12Resource>	m_cbCamera2;	// 1인칭 플레이어를 렌더링할 때 사용할 상수 버퍼
	cbCamera*				m_pcbCamera2;	// 상수 버퍼 포인터

	XMFLOAT3				m_eye;			// 카메라 위치
	XMFLOAT3				m_at;			// 카메라가 바라보는 방향
	XMFLOAT3				m_up;			// 카메라 Up벡터

	FLOAT					m_roll;			// x축 회전각
	FLOAT					m_pitch;		// y축 회전각
	FLOAT					m_yaw;			// z축 회전각

	Player*					m_player;		// 플레이어
	XMFLOAT3				m_offset;		// 플레이어로부터 떨어져있는 위치
};

class ThirdPersonCamera : public Camera
{
public:
	ThirdPersonCamera();
	~ThirdPersonCamera() = default;

	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void Update(FLOAT deltaTime);
	virtual void Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw);

	void SetFocusOffset(const XMFLOAT3& focusOffset) { m_focusOffset = focusOffset; }
	void SetDistance(FLOAT distance) { m_distance = clamp(distance, 3.0f, 9999.0f); }
	void SetDelay(FLOAT delay) { m_delay = delay; }

	XMFLOAT3 GetFocusOffset() const { return m_focusOffset; }
	FLOAT GetDistance() const { return m_distance; }

private:
	XMFLOAT3	m_focusOffset;	// 카메라가 바라볼 위치
	FLOAT		m_distance;		// 오프셋 방향으로 떨어진 거리
	FLOAT		m_delay;		// 움직임 딜레이 (0.0 ~ 1.0)
};

class ShowCamera : public Camera
{
public:
	ShowCamera();
	~ShowCamera() = default;

	virtual void Update(FLOAT deltaTime);

	void SetTarget(GameObject* target);
	void SetTime(FLOAT time);
	void SetTimerCallback(const function<void()>& callBackFunc);

private:
	GameObject*			m_target;
	FLOAT				m_timer;
	FLOAT				m_time;
	function<void()>	m_timerCallBack;
};