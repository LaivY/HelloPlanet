#include "player.h"
#include "camera.h"

Player::Player(BOOL isMultiPlayer) : GameObject{}, m_id{ -1 }, m_isMultiPlayer{ isMultiPlayer }, m_gunType{ ePlayerGunType::NONE },
									 m_velocity{ 0.0f, 0.0f, 0.0f }, m_maxVelocity{ 0.0f }, m_friction{ 0.96f },
									 m_camera{ nullptr }, m_gunMesh{ nullptr }, m_gunShader{ nullptr }
{
	
}

void Player::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifndef FREEVIEW
	switch (message)
	{
		case WM_LBUTTONDOWN:
		{
			PlayAnimation("FIRING", TRUE);
			break;
		}
	}
#endif
}

void Player::OnKeyboardEvent(FLOAT deltaTime)
{
#ifndef FREEVIEW
	if (GetAsyncKeyState('W') & 0x8000)
	{
		string currPureAnimationName{ GetCurrAnimationName() };
		string afterPureAnimationName{ GetAfterAnimationName() };
		XMFLOAT3 look{ m_camera->GetAt() };
		look.y = 0.0f;
		look = Vector3::Normalize(look);

		if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) && m_gunType != ePlayerGunType::MG)
		{
			if ((m_animationInfo->state == eAnimationState::PLAY && currPureAnimationName != "RUNNING") ||
				(m_animationInfo->state == eAnimationState::BLENDING && afterPureAnimationName == "IDLE"))
				PlayAnimation("RUNNING", TRUE);
			Move(Vector3::Mul(look, 150.0f * deltaTime));
		}
		else
		{
			if ((m_animationInfo->state == eAnimationState::PLAY && currPureAnimationName != "WALKING") ||
				(m_animationInfo->state == eAnimationState::BLENDING && afterPureAnimationName == "IDLE"))
			{
				PlayAnimation("WALKING", TRUE);
				SendAnimationState();
			}
			Move(Vector3::Mul(look, 50.0f * deltaTime));
		}
	}
	else if (GetAsyncKeyState('A') & 0x8000)
	{
		string currPureAnimationName{ GetCurrAnimationName() };
		string afterPureAnimationName{ GetAfterAnimationName() };
		XMFLOAT3 look{ m_camera->GetAt() };
		look.y = 0.0f;
		look = Vector3::Normalize(look);
		XMFLOAT3 right{ Vector3::Cross(XMFLOAT3{ 0.0f, 1.0f, 0.0f }, look) };
		XMFLOAT3 left{ Vector3::Mul(right, -1) };

		if ((m_animationInfo->state == eAnimationState::PLAY && currPureAnimationName != "WALKLEFT") ||
			(m_animationInfo->state == eAnimationState::BLENDING && afterPureAnimationName == "IDLE"))
			PlayAnimation("WALKLEFT", TRUE);
		Move(Vector3::Mul(left, 10.0f * deltaTime));
	}
	else if (GetAsyncKeyState('D') & 0x8000)
	{
		string currPureAnimationName{ GetCurrAnimationName() };
		string afterPureAnimationName{ GetAfterAnimationName() };
		XMFLOAT3 look{ m_camera->GetAt() };
		look.y = 0.0f;
		look = Vector3::Normalize(look);

		XMFLOAT3 right{ Vector3::Cross(XMFLOAT3{ 0.0f, 1.0f, 0.0f }, look) };
		if ((m_animationInfo->state == eAnimationState::PLAY && currPureAnimationName != "WALKRIGHT") ||
			(m_animationInfo->state == eAnimationState::BLENDING && afterPureAnimationName == "IDLE"))
			PlayAnimation("WALKRIGHT", TRUE);
		Move(Vector3::Mul(right, 10.0f * deltaTime));
	}
	else if (GetAsyncKeyState('S') & 0x8000)
	{
		string currPureAnimationName{ GetCurrAnimationName() };
		string afterPureAnimationName{ GetAfterAnimationName() };
		XMFLOAT3 look{ m_camera->GetAt() };
		look.y = 0.0f;
		look = Vector3::Normalize(look);
		XMFLOAT3 back{ Vector3::Mul(look, -1) };

		if ((m_animationInfo->state == eAnimationState::PLAY && currPureAnimationName != "WALKBACK") ||
			(m_animationInfo->state == eAnimationState::BLENDING && afterPureAnimationName == "IDLE"))
			PlayAnimation("WALKBACK", TRUE);
		Move(Vector3::Mul(back, 10.0f * deltaTime));
	}
#endif
}

void Player::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifndef FREEVIEW
	if (wParam == 'r' || wParam == 'R')
	{
		if (message == WM_KEYDOWN && GetCurrAnimationName() != "RELOAD")
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
				PlayAnimation("WALKING", TRUE);
			else
				PlayAnimation("IDLE", TRUE);
		}
	}
#endif
}

void Player::OnAnimation(FLOAT currFrame, UINT endFrame, BOOL isUpper)
{
	// 상체 애니메이션 콜백 처리
	if (isUpper)
	{
		if (currFrame >= endFrame)
		{
			if (m_upperAnimationInfo->state == eAnimationState::PLAY)
			{
				m_upperAnimationInfo->state = eAnimationState::SYNC;
				m_upperAnimationInfo->blendingTimer = 0.0f;
			}
			else if (m_upperAnimationInfo->state == eAnimationState::BLENDING)
				PlayAnimation(GetUpperAfterAnimationName());
			else if (m_upperAnimationInfo->state == eAnimationState::SYNC)
				m_upperAnimationInfo.reset();
		}
		return;
	}

	if (m_animationInfo->state == eAnimationState::PLAY) // 프레임 진행 중
	{
		if (currFrame >= endFrame)
		{
			// 이동
			string currPureAnimationName{ GetCurrAnimationName() };
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
	else if (m_animationInfo->state == eAnimationState::BLENDING) // 블렌딩 진행 중
	{
		if (currFrame >= endFrame)
		{
			PlayAnimation(GetAfterAnimationName());
		}
	}
}

void Player::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
	if (m_isMultiPlayer)
	{
		GameObject::Render(commandList, shader);
		if (m_gunMesh)
		{
			if (shader) commandList->SetPipelineState(shader->GetPipelineState().Get());
			else if (m_gunShader) commandList->SetPipelineState(m_gunShader->GetPipelineState().Get());
			m_gunMesh->UpdateShaderVariable(commandList, this);
			m_gunMesh->Render(commandList);
		}
	}
	else
	{
#ifdef FIRSTVIEW
		if (!m_camera) return;
		XMFLOAT3 at{ m_camera->GetAt() }, up{ m_camera->GetUp() };

		m_camera->UpdateShaderVariableByPlayer(commandList);	// 카메라 파라미터를 플레이어에 맞추고 셰이더 변수를 업데이트한다.
		GameObject::Render(commandList, shader);				// 플레이어를 렌더링한다.

		// 이후 렌더링할 객체들을 위해 카메라 파라미터를 이전 것으로 되돌린다.
		m_camera->SetAt(at);
		m_camera->SetUp(up);
		m_camera->Update(0.0f);
		m_camera->UpdateShaderVariable(commandList);
#else
		GameObject::Render(commandList, shader);
		if (m_gunMesh)
		{
			m_gunMesh->UpdateShaderVariable(commandList, this);
			if (shader) commandList->SetPipelineState(shader->GetPipelineState().Get());
			else if (m_gunShader) commandList->SetPipelineState(m_gunShader->GetPipelineState().Get());
			m_gunMesh->Render(commandList);
		}

		//string debug{};
		//debug += m_animationInfo->currAnimationName + "(" + to_string(m_animationInfo->currTimer) + ")\n";
		//if (m_animationInfo->afterAnimationName.empty())
		//	debug += "NONE\n\n";
		//else
		//	debug += m_animationInfo->afterAnimationName + "(" + to_string(m_animationInfo->afterTimer) + ")\n\n";
		//OutputDebugStringA(debug.c_str());
#endif
	}
}

void Player::Update(FLOAT deltaTime)
{
	GameObject::Update(deltaTime);

	// 상체 애니메이션 타이머 진행
	if (m_upperAnimationInfo)
	{
		if (m_upperAnimationInfo->state == eAnimationState::PLAY)
			m_upperAnimationInfo->currTimer += deltaTime;
		else if (m_upperAnimationInfo->state == eAnimationState::BLENDING || m_upperAnimationInfo->state == eAnimationState::SYNC)
			m_upperAnimationInfo->blendingTimer += deltaTime;
	}

	// 이동
	Move(m_velocity);

	// 마찰력
	m_velocity = Vector3::Mul(m_velocity, m_friction * deltaTime);
}

void Player::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	// 회전각 제한
	if (m_pitch + pitch > Setting::CAMERA_MAX_PITCH)
		pitch = Setting::CAMERA_MAX_PITCH - m_pitch;
	else if (m_pitch + pitch < Setting::CAMERA_MIN_PITCH)
		pitch = Setting::CAMERA_MIN_PITCH - m_pitch;

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

	if (m_animationInfo && m_animationInfo->state == eAnimationState::BLENDING && GetCurrAnimationName() == animationName)
	{
		m_animationInfo->state = eAnimationState::PLAY;
		m_animationInfo->afterAnimationName.clear();
		m_animationInfo->afterTimer = 0.0f;
		m_animationInfo->blendingTimer = 0.0f;
		//m_animationInfo->currAnimationName.swap(m_animationInfo->afterAnimationName);
		//swap(m_animationInfo->currTimer, m_animationInfo->afterTimer);
		return;
	}
	
	// 아래의 애니메이션은 상체만 애니메이션함
	if (pureAnimationName == "RELOAD" || pureAnimationName == "FIRING")
	{
		if (m_gunType == ePlayerGunType::AR) PlayUpperAnimation("AR/" + pureAnimationName, doBlending);
		else if (m_gunType == ePlayerGunType::SG) PlayUpperAnimation("SG/" + pureAnimationName, doBlending);
		else if (m_gunType == ePlayerGunType::MG) PlayUpperAnimation("MG/" + pureAnimationName, doBlending);
		return;
	}

	// 그 외는 상하체 모두 애니메이션함
	if (m_gunType == ePlayerGunType::AR) GameObject::PlayAnimation("AR/" + pureAnimationName, doBlending);
	else if (m_gunType == ePlayerGunType::SG) GameObject::PlayAnimation("SG/" + pureAnimationName, doBlending);
	else if (m_gunType == ePlayerGunType::MG) GameObject::PlayAnimation("MG/" + pureAnimationName, doBlending);
}

void Player::SendAnimationState() const
{
	cs_packet_update_legs packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_UPDATE_LEGS;

	string currAnimationName{ GetCurrAnimationName() };
	if (currAnimationName == "IDLE")
		packet.state = legState::IDLE;
	else if (currAnimationName == "WALKING")
		packet.state = legState::WALKING;
	else if (currAnimationName == "WALKLEFT")
		packet.state = legState::WALKLEFT;
	else if (currAnimationName == "WALKRIGHT")
		packet.state = legState::WALKRIGHT;
	else if (currAnimationName == "WALKBACK")
		packet.state = legState::WALKBACK;
	else if (currAnimationName == "RUNNING")
		packet.state = legState::RUNNING;

	int send_result = send(g_c_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
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

void Player::PlayUpperAnimation(const string& animationName, BOOL doBlending)
{
	if (!m_upperAnimationInfo) m_upperAnimationInfo = make_unique<AnimationInfo>();
	if (doBlending)
	{
		m_upperAnimationInfo->state = eAnimationState::BLENDING;
		m_upperAnimationInfo->currAnimationName = m_animationInfo->currAnimationName;
		m_upperAnimationInfo->currTimer = m_animationInfo->currTimer;
		m_upperAnimationInfo->afterAnimationName = animationName;
		m_upperAnimationInfo->afterTimer = 0.0f;
	}
	else
	{
		m_upperAnimationInfo->state = eAnimationState::PLAY;
		m_upperAnimationInfo->currAnimationName = animationName;
		m_upperAnimationInfo->currTimer = 0.0f;
		m_upperAnimationInfo->afterAnimationName.clear();
		m_upperAnimationInfo->afterTimer = 0.0f;
	}
	m_upperAnimationInfo->blendingTimer = 0.0f;
}

string Player::GetPureAnimationName(const string& animationName) const
{
	if (auto p{ animationName.find_last_of('/') }; p != string::npos)
		return animationName.substr(p + 1);
	return animationName;
}

string Player::GetCurrAnimationName() const
{
	return GetPureAnimationName(m_animationInfo->currAnimationName);
}

string Player::GetAfterAnimationName() const
{
	return GetPureAnimationName(m_animationInfo->afterAnimationName);
}

string Player::GetUpperCurrAnimationName() const
{
	return GetPureAnimationName(m_upperAnimationInfo->currAnimationName);
}

string Player::GetUpperAfterAnimationName() const
{
	return GetPureAnimationName(m_upperAnimationInfo->afterAnimationName);
}