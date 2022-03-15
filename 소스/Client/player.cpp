#include "player.h"
#include "camera.h"

Player::Player() : GameObject{}, m_velocity{ 0.0f, 0.0f, 0.0f }, m_maxVelocity{ 10.0f }, m_friction{ 0.96f }, m_weaponType{ }, m_camera{ nullptr }, m_gunMesh{ nullptr }, m_gunShader{ nullptr }
{
	
}

void Player::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_LBUTTONDOWN:
		{
			PlayAnimation("FIRING", TRUE);
			break;
		}
	}
}

void Player::OnKeyboardEvent(FLOAT deltaTime)
{
	if (GetAsyncKeyState('W') & 0x8000)
	{
		string currPureAnimationName{ GetPureAnimationName(m_animationInfo->currAnimationName) };
		string afterPureAnimationName{ GetPureAnimationName(m_animationInfo->afterAnimationName) };
		XMFLOAT3 look{ m_camera->GetAt() };
		look.y = 0.0f;
		look = Vector3::Normalize(look);

		if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) && m_weaponType != MG)
		{
			if ((m_animationInfo->state == PLAY && currPureAnimationName != "RUNNING") ||
				(m_animationInfo->state == BLENDING && afterPureAnimationName == "IDLE"))
				PlayAnimation("RUNNING", TRUE);
			Move(Vector3::Mul(look, 50.0f * deltaTime));
		}
		else
		{
			if ((m_animationInfo->state == PLAY && currPureAnimationName != "WALKING") ||
				(m_animationInfo->state == BLENDING && afterPureAnimationName == "IDLE"))
				PlayAnimation("WALKING", TRUE);
			Move(Vector3::Mul(look, 10.0f * deltaTime));
		}
	}
	else if (GetAsyncKeyState('A') & 0x8000)
	{
		string currPureAnimationName{ GetPureAnimationName(m_animationInfo->currAnimationName) };
		string afterPureAnimationName{ GetPureAnimationName(m_animationInfo->afterAnimationName) };
		XMFLOAT3 look{ m_camera->GetAt() };
		look.y = 0.0f;
		look = Vector3::Normalize(look);
		XMFLOAT3 right{ Vector3::Cross(XMFLOAT3{ 0.0f, 1.0f, 0.0f }, look) };
		XMFLOAT3 left{ Vector3::Mul(right, -1) };

		if ((m_animationInfo->state == PLAY && currPureAnimationName != "WALKLEFT") ||
			(m_animationInfo->state == BLENDING && afterPureAnimationName == "IDLE"))
			PlayAnimation("WALKLEFT", TRUE);
		Move(Vector3::Mul(left, 10.0f * deltaTime));
	}
	else if (GetAsyncKeyState('D') & 0x8000)
	{
		string currPureAnimationName{ GetPureAnimationName(m_animationInfo->currAnimationName) };
		string afterPureAnimationName{ GetPureAnimationName(m_animationInfo->afterAnimationName) };
		XMFLOAT3 look{ m_camera->GetAt() };
		look.y = 0.0f;
		look = Vector3::Normalize(look);

		XMFLOAT3 right{ Vector3::Cross(XMFLOAT3{ 0.0f, 1.0f, 0.0f }, look) };
		if ((m_animationInfo->state == PLAY && currPureAnimationName != "WALKRIGHT") ||
			(m_animationInfo->state == BLENDING && afterPureAnimationName == "IDLE"))
			PlayAnimation("WALKRIGHT", TRUE);
		Move(Vector3::Mul(right, 10.0f * deltaTime));
	}
	else if (GetAsyncKeyState('S') & 0x8000)
	{
		string currPureAnimationName{ GetPureAnimationName(m_animationInfo->currAnimationName) };
		string afterPureAnimationName{ GetPureAnimationName(m_animationInfo->afterAnimationName) };
		XMFLOAT3 look{ m_camera->GetAt() };
		look.y = 0.0f;
		look = Vector3::Normalize(look);
		XMFLOAT3 back{ Vector3::Mul(look, -1) };

		if ((m_animationInfo->state == PLAY && currPureAnimationName != "WALKBACK") ||
			(m_animationInfo->state == BLENDING && afterPureAnimationName == "IDLE"))
			PlayAnimation("WALKBACK", TRUE);
		Move(Vector3::Mul(back, 10.0f * deltaTime));
	}
}

void Player::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (wParam == 'r' || wParam == 'R')
	{
		if (message == WM_KEYDOWN && GetPureAnimationName(m_animationInfo->currAnimationName) != "RELOAD")
		{
			PlayAnimation("RELOAD", TRUE);
		}
	}
	if (wParam == 'w' || wParam == 'W' || wParam == 'a' || wParam == 'A' || wParam == 'd' || wParam == 'D' || wParam == 's' || wParam == 'S')
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

void Player::OnAnimation(FLOAT currFrame, UINT endFrame, BOOL isUpper)
{
	// 상체 애니메이션 콜백 처리
	if (isUpper)
	{
		if (currFrame >= endFrame)
		{
			if (m_upperAnimationInfo->state == PLAY)
			{
				m_upperAnimationInfo->state = SYNC;
				m_upperAnimationInfo->blendingTimer = 0.0f;
			}
			else if (m_upperAnimationInfo->state == BLENDING)
				PlayAnimation(GetPureAnimationName(m_upperAnimationInfo->afterAnimationName));
			else if (m_upperAnimationInfo->state == SYNC)
				m_upperAnimationInfo.reset();
		}
		return;
	}

	if (m_animationInfo->state == PLAY) // 프레임 진행 중
	{
		if (currFrame >= endFrame)
		{
			// 이동
			string currPureAnimationName{ GetPureAnimationName(m_animationInfo->currAnimationName) };
			if (((GetAsyncKeyState('W') & 0x8000) && currPureAnimationName == "WALKING") ||
				((GetAsyncKeyState('W') & GetAsyncKeyState(VK_SHIFT) & 0x8000) && currPureAnimationName == "RUNNING") ||
				((GetAsyncKeyState('A') & 0x8000) && currPureAnimationName == "WALKLEFT") ||
				((GetAsyncKeyState('D') & 0x8000) && currPureAnimationName == "WALKRIGHT") ||
				((GetAsyncKeyState('S') & 0x8000) && currPureAnimationName == "WALKBACK"))
			{
				PlayAnimation(currPureAnimationName);
				return;
			}

			// 대기
			if (currPureAnimationName == "IDLE")
			{
				PlayAnimation("IDLE");
				return;
			}

			// 그 외에는 대기 애니메이션 재생
			PlayAnimation("IDLE", TRUE);
		}
	}
	else if (m_animationInfo->state == BLENDING) // 블렌딩 진행 중
	{
		if (currFrame >= endFrame)
		{
			PlayAnimation(GetPureAnimationName(m_animationInfo->afterAnimationName));
		}
	}
}

void Player::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
	GameObject::Render(commandList, shader);
	if (m_gunMesh)
	{
		m_gunMesh->UpdateShaderVariable(commandList, this);
		if (shader) commandList->SetPipelineState(shader->GetPipelineState().Get());
		else if (m_gunShader) commandList->SetPipelineState(m_gunShader->GetPipelineState().Get());
		m_gunMesh->Render(commandList);
	}
}

void Player::Update(FLOAT deltaTime)
{
	GameObject::Update(deltaTime);

	// 상체 애니메이션 타이머 진행
	if (m_upperAnimationInfo)
	{
		if (m_upperAnimationInfo->state == PLAY)
			m_upperAnimationInfo->currTimer += deltaTime;
		else if (m_upperAnimationInfo->state == BLENDING || m_upperAnimationInfo->state == SYNC)
			m_upperAnimationInfo->blendingTimer += deltaTime;
	}

	// 이동
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

void Player::PlayAnimation(const string& animationName, BOOL doBlending)
{
	// 무기 타입에 따라 해당 무기에 맞는 애니메이션을 재생함
	// ex) AR을 착용한 플레이어가 IDLE이라는 애니메이션을 재생한다면 AR/IDLE 애니메이션이 재생됨
	string pureAnimationName{ GetPureAnimationName(animationName) };

	// 아래의 애니메이션은 상체만 애니메이션함
	if (pureAnimationName == "RELOAD" || pureAnimationName == "FIRING")
	{
		if (m_weaponType == AR) PlayUpperAnimation("AR/" + pureAnimationName, doBlending);
		else if (m_weaponType == SG) PlayUpperAnimation("SG/" + pureAnimationName, doBlending);
		else if (m_weaponType == MG) PlayUpperAnimation("MG/" + pureAnimationName, doBlending);
		return;
	}

	// 그 외는 상하체 모두 애니메이션함
	if (m_weaponType == AR) GameObject::PlayAnimation("AR/" + pureAnimationName, doBlending);
	else if (m_weaponType == SG) GameObject::PlayAnimation("SG/" + pureAnimationName, doBlending);
	else if (m_weaponType == MG) GameObject::PlayAnimation("MG/" + pureAnimationName, doBlending);
}

void Player::PlayUpperAnimation(const string& animationName, BOOL doBlending)
{
	if (!m_upperAnimationInfo) m_upperAnimationInfo = make_unique<AnimationInfo>();
	if (doBlending)
	{
		m_upperAnimationInfo->currAnimationName = m_animationInfo->currAnimationName;
		m_upperAnimationInfo->currTimer = m_animationInfo->currTimer;
		m_upperAnimationInfo->afterAnimationName = animationName;
		m_upperAnimationInfo->afterTimer = 0.0f;
		m_upperAnimationInfo->state = BLENDING;
	}
	else
	{
		m_upperAnimationInfo->currAnimationName = animationName;
		m_upperAnimationInfo->currTimer = 0.0f;
		m_upperAnimationInfo->afterAnimationName.clear();
		m_upperAnimationInfo->afterTimer = 0.0f;
		m_upperAnimationInfo->state = PLAY;
	}
	m_upperAnimationInfo->blendingTimer = 0.0f;
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

string Player::GetPureAnimationName(const string& animationName) const
{
	if (auto p{ animationName.find_last_of('/') }; p != string::npos)
		return animationName.substr(p + 1);
	return animationName;
}