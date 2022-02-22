#include "player.h"
#include "camera.h"

Player::Player() : GameObject{}, m_velocity{ 0.0f, 0.0f, 0.0f }, m_maxVelocity{ 10.0f }, m_friction{ 0.96f }, m_camera{ nullptr }, m_gunMesh{ nullptr }, m_gunShader{ nullptr }
{
	
}

void Player::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_LBUTTONDOWN:
		{
			PlayAnimation("FIRINGAR");
			break;
		}
	}
}

void Player::OnKeyboardEvent(FLOAT deltaTime)
{
	if (GetAsyncKeyState('W') & 0x8000)
	{
		XMFLOAT3 look{ m_camera->GetAt() };
		look.y = 0.0f;
		look = Vector3::Normalize(look);
		if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
		{
			if (m_animationInfo->animationName != "RUNNING")
				PlayAnimation("RUNNING", TRUE);
			Move(Vector3::Mul(look, 50.0f * deltaTime));
		}
		else
		{
			if (m_animationInfo->animationName != "WALKING")
				PlayAnimation("WALKING", TRUE);
			Move(Vector3::Mul(look, 10.0f * deltaTime));
		}
	}
	else if (GetAsyncKeyState('A') & 0x8000)
	{
		XMFLOAT3 look{ m_camera->GetAt() };
		look.y = 0.0f;
		look = Vector3::Normalize(look);

		XMFLOAT3 right{ Vector3::Cross(XMFLOAT3{ 0.0f, 1.0f, 0.0f }, look) };
		XMFLOAT3 left{ Vector3::Mul(right, -1) };
		if (m_animationInfo->animationName != "WALKLEFT")
			PlayAnimation("WALKLEFT", TRUE);
		Move(Vector3::Mul(left, 10.0f * deltaTime));
	}
	else if (GetAsyncKeyState('D') & 0x8000)
	{
		XMFLOAT3 look{ m_camera->GetAt() };
		look.y = 0.0f;
		look = Vector3::Normalize(look);

		XMFLOAT3 right{ Vector3::Cross(XMFLOAT3{ 0.0f, 1.0f, 0.0f }, look) };
		if (m_animationInfo->animationName != "WALKRIGHT")
			PlayAnimation("WALKRIGHT", TRUE);
		Move(Vector3::Mul(right, 10.0f * deltaTime));
	}
}

void Player::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (wParam == 'r' || wParam == 'R')
	{
		if (message == WM_KEYDOWN && m_animationInfo->animationName != "RELOAD")
		{
			PlayAnimation("RELOAD");
		}
	}
	if (wParam == 'w' || wParam == 'W' || wParam == 'a' || wParam == 'A' || wParam == 'd' || wParam == 'D')
	{
		if (message == WM_KEYUP)
			PlayAnimation("IDLE", TRUE);
	}
	if (wParam == VK_SHIFT)
	{
		if (message == WM_KEYUP)
		{
			if (GetAsyncKeyState('W') & 0x8000)
			{
				PlayAnimation("WALKING", TRUE);
			}
		}
	}
}

void Player::OnAnimation(const string& animationName, FLOAT currFrame, UINT endFrame)
{
	if (m_animationInfo->beforeAnimationName.empty()) // 애니메이션 프레임 진행 중
	{
		if (currFrame >= endFrame)
		{
			// w키가 눌려진 상태라면 걷기 애니메이션 재생
			// w키와 쉬프트키가 눌려진 상태라면 뛰기 애니메이션 재생
			if (((GetAsyncKeyState('W') & 0x8000) && animationName == "WALKING") ||
				((GetAsyncKeyState('W') & GetAsyncKeyState(VK_SHIFT) & 0x8000) && animationName == "RUNNING"))
				PlayAnimation(animationName);

			// 그 외에는 대기 애니메이션 재생
			else PlayAnimation("IDLE", TRUE);
		}
	}
	else // 애니메이션 블렌딩 진행 중
	{
		if (currFrame >= endFrame)
		{
			PlayAnimation(m_animationInfo->animationName);
		}
	}
}

void Player::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
	GameObject::Render(commandList, shader);
	
	if (shader) commandList->SetPipelineState(shader->GetPipelineState().Get());
	else if (m_gunShader) commandList->SetPipelineState(m_gunShader->GetPipelineState().Get());

	if (m_gunMesh) m_gunMesh->Render(commandList, this);
}

void Player::Update(FLOAT deltaTime)
{
	GameObject::Update(deltaTime);

	Move(m_velocity);

	// 마찰력
	m_velocity = Vector3::Mul(m_velocity, m_friction);
}

void Player::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	// 회전각 제한
	if (m_pitch + pitch > MAX_PITCH)
		pitch = MAX_PITCH - m_pitch;
	else if (m_pitch + pitch < MIN_PITCH)
		pitch = MIN_PITCH - m_pitch;

	// 회전각 합산
	m_roll += roll; m_pitch += pitch; m_yaw += yaw;

	// 카메라는 x,y축으로 회전할 수 있다.
	// GameObject::Rotate에서 플레이어의 로컬 x,y,z축을 변경하므로 먼저 호출해야한다.
	m_camera->Rotate(0.0f, pitch, yaw);

	// 플레이어는 y축으로만 회전할 수 있다.
	GameObject::Rotate(0.0f, 0.0f, yaw);
}

void Player::AddVelocity(const XMFLOAT3& increase)
{
	m_velocity = Vector3::Add(m_velocity, increase);

	// 최대 속도에 걸린다면 해당 비율로 축소시킴
	FLOAT length{ Vector3::Length(m_velocity) };
	if (length > m_maxVelocity)
	{
		FLOAT ratio{ m_maxVelocity / length };
		m_velocity = Vector3::Mul(m_velocity, ratio);
	}
}