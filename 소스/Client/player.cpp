#include "player.h"
#include "camera.h"

Player::Player(BOOL isMultiPlayer) : GameObject{}, m_id{ -1 }, m_isMultiPlayer{ isMultiPlayer }, m_gunType{ ePlayerGunType::NONE },
									 m_speed{ 20.0f }, m_velocity{ 0.0f, 0.0f, 0.0f }, m_maxVelocity{ 0.0f }, m_friction{ 0.96f },
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
			if (GetCurrAnimationName() == "RUNNING")
				break;
			if (GetUpperCurrAnimationName() == "RELOAD")
				break;
			PlayAnimation("FIRING", GetUpperCurrAnimationName() != "FIRING");
			SendPlayerData(); // 서버에게 총 발사했다고 알림
			break;
		}
	}
#endif
}

void Player::OnKeyboardEvent(FLOAT deltaTime)
{
#ifndef FREEVIEW
	string currPureAnimationName{ GetCurrAnimationName() };
	string afterPureAnimationName{ GetAfterAnimationName() };

	if (GetAsyncKeyState('W') && GetAsyncKeyState('A') & 0x8000)
	{
		if ((m_animationInfo->state == eAnimationState::PLAY && currPureAnimationName != "WALKLEFT") ||
			(m_animationInfo->state == eAnimationState::BLENDING && afterPureAnimationName == "IDLE"))
			PlayAnimation("WALKLEFT", TRUE);
		m_velocity.x = -m_speed / 2.0f;
		m_velocity.z = m_speed / 2.0f;
		SendPlayerData();
	}
	else if (GetAsyncKeyState('W') && GetAsyncKeyState('D') & 0x8000)
	{
		if ((m_animationInfo->state == eAnimationState::PLAY && currPureAnimationName != "WALKRIGHT") ||
			(m_animationInfo->state == eAnimationState::BLENDING && afterPureAnimationName == "IDLE"))
			PlayAnimation("WALKRIGHT", TRUE);
		m_velocity.x = m_speed / 2.0f;
		m_velocity.z = m_speed / 2.0f;
		SendPlayerData();
	}
	else if (GetAsyncKeyState('S') && GetAsyncKeyState('A') & 0x8000)
	{
		if ((m_animationInfo->state == eAnimationState::PLAY && currPureAnimationName != "WALKLEFT") ||
			(m_animationInfo->state == eAnimationState::BLENDING && afterPureAnimationName == "IDLE"))
			PlayAnimation("WALKLEFT", TRUE);
		m_velocity.x = -m_speed / 2.0f;
		m_velocity.z = -m_speed / 2.0f;
		SendPlayerData();
	}
	else if (GetAsyncKeyState('S') && GetAsyncKeyState('D') & 0x8000)
	{
		if ((m_animationInfo->state == eAnimationState::PLAY && currPureAnimationName != "WALKRIGHT") ||
			(m_animationInfo->state == eAnimationState::BLENDING && afterPureAnimationName == "IDLE"))
			PlayAnimation("WALKRIGHT", TRUE);
		m_velocity.x = m_speed / 2.0f;
		m_velocity.z = -m_speed / 2.0f;
		SendPlayerData();
	}
	else if (GetAsyncKeyState('W') & 0x8000)
	{
		if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) && GetUpperCurrAnimationName() != "RELOAD" && m_gunType != ePlayerGunType::MG)
		{
			if ((m_animationInfo->state == eAnimationState::PLAY && currPureAnimationName != "RUNNING") ||
				(m_animationInfo->state == eAnimationState::BLENDING && afterPureAnimationName == "IDLE"))
				PlayAnimation("RUNNING", TRUE);
			m_velocity.z = m_speed * 5.0f;
			SendPlayerData();
		}
		else
		{
			if ((m_animationInfo->state == eAnimationState::PLAY && currPureAnimationName != "WALKING") ||
				(m_animationInfo->state == eAnimationState::BLENDING && afterPureAnimationName == "IDLE"))
				PlayAnimation("WALKING", TRUE);
			m_velocity.z = m_speed;
			SendPlayerData();
		}
	}
	else if (GetAsyncKeyState('A') & 0x8000)
	{
		if ((m_animationInfo->state == eAnimationState::PLAY && currPureAnimationName != "WALKLEFT") ||
			(m_animationInfo->state == eAnimationState::BLENDING && afterPureAnimationName == "IDLE"))
			PlayAnimation("WALKLEFT", TRUE);
		m_velocity.x = -m_speed;
		SendPlayerData();
	}
	else if (GetAsyncKeyState('D') & 0x8000)
	{
		if ((m_animationInfo->state == eAnimationState::PLAY && currPureAnimationName != "WALKRIGHT") ||
			(m_animationInfo->state == eAnimationState::BLENDING && afterPureAnimationName == "IDLE"))
			PlayAnimation("WALKRIGHT", TRUE);
		m_velocity.x = m_speed;
		SendPlayerData();
	}
	else if (GetAsyncKeyState('S') & 0x8000)
	{
		if ((m_animationInfo->state == eAnimationState::PLAY && currPureAnimationName != "WALKBACK") ||
			(m_animationInfo->state == eAnimationState::BLENDING && afterPureAnimationName == "IDLE"))
			PlayAnimation("WALKBACK", TRUE);
		m_velocity.z = -m_speed;
		SendPlayerData();
	}
	//else SendPlayerData(eAnimationType::IDLE);
#endif
}

void Player::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifndef FREEVIEW
	switch (message)
	{
	case WM_KEYUP:
	{
		switch (wParam)
		{
		case 'w': case 'W':
		case 'a': case 'A':
		case 'd': case 'D':
		case 's': case 'S':
			PlayAnimation("IDLE", TRUE);
			m_velocity = XMFLOAT3{ 0.0f, 0.0f, 0.0f };
			SendPlayerData();
			break;
		case VK_SHIFT:
			if (GetAsyncKeyState('W') & 0x8000)
			{
				if (GetUpperCurrAnimationName() != "RELOAD")
					PlayAnimation("WALKING", TRUE);
				m_velocity.z = m_speed;
			}
			else PlayAnimation("IDLE", TRUE);
			break;
		}
		break;
	}
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case 'r': case 'R':
			if (!m_upperAnimationInfo || (m_upperAnimationInfo && GetUpperCurrAnimationName() != "RELOAD"))
			{
				PlayAnimation("RELOAD", TRUE);
				SendPlayerData();
			}
			break;
		}
		break;
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
			switch (m_upperAnimationInfo->state)
			{
			case eAnimationState::PLAY:
				m_upperAnimationInfo->state = eAnimationState::SYNC;
				m_upperAnimationInfo->blendingTimer = 0.0f;
				break;
			case eAnimationState::BLENDING:
				PlayAnimation(GetUpperAfterAnimationName());
				break;
			case eAnimationState::SYNC:
				m_upperAnimationInfo.reset();
				if (!m_isMultiPlayer) SendPlayerData(); // 서버에게 상체 애니메이션 끝났다고 알림
				break;
			}
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
#endif
	}
}

void Player::Update(FLOAT deltaTime)
{
	GameObject::Update(deltaTime);

	// 상체 애니메이션 타이머 진행
	if (m_upperAnimationInfo)
		switch (m_upperAnimationInfo->state)
		{
		case eAnimationState::PLAY:
			m_upperAnimationInfo->currTimer += deltaTime;
			break;
		case eAnimationState::BLENDING:
		case eAnimationState::SYNC:
			m_upperAnimationInfo->blendingTimer += deltaTime;
			break;
		}

	// 이동
	Move(Vector3::Mul(GetRight(), m_velocity.x * deltaTime));
	Move(Vector3::Mul(GetLook(), m_velocity.z * deltaTime));

	// 마찰력
	//m_velocity = Vector3::Mul(m_velocity, m_friction * deltaTime);
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
	if (m_camera) m_camera->Rotate(0.0f, pitch, yaw);

	// 플레이어는 y축으로만 회전할 수 있다.
	GameObject::Rotate(0.0f, 0.0f, yaw);

	if (!m_isMultiPlayer) SendPlayerData(); // 서버에게 회전했다고 알림
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

void Player::SendPlayerData() const
{
#ifdef NETWORK
	cs_packet_update_legs packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_UPDATE_LEGS;
	
	// 애니메이션 타입
	if (GetAsyncKeyState('W') && GetAsyncKeyState('A') & 0x8000)
		packet.aniType = eAnimationType::WALKLEFT;
	else if (GetAsyncKeyState('W') && GetAsyncKeyState('D') & 0x8000)
		packet.aniType = eAnimationType::WALKRIGHT;
	else if (GetAsyncKeyState('S') && GetAsyncKeyState('A') & 0x8000)
		packet.aniType = eAnimationType::WALKLEFT;
	else if (GetAsyncKeyState('S') && GetAsyncKeyState('D') & 0x8000)
		packet.aniType = eAnimationType::WALKRIGHT;
	else if (GetAsyncKeyState('W') & 0x8000)
		if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
			packet.aniType = eAnimationType::RUNNING;
		else
			packet.aniType = eAnimationType::WALKING;
	else if (GetAsyncKeyState('A') & 0x8000)
		packet.aniType = eAnimationType::WALKLEFT;
	else if (GetAsyncKeyState('S') & 0x8000)
		packet.aniType = eAnimationType::WALKBACK;
	else if (GetAsyncKeyState('D') & 0x8000)
		packet.aniType = eAnimationType::WALKRIGHT;
	else
		packet.aniType = eAnimationType::IDLE;

	// 상체 애니메이션 타입
	packet.upperAniType = eUpperAnimationType::NONE;
	if (GetAsyncKeyState('R') & 0x8000)
		packet.upperAniType = eUpperAnimationType::RELOAD;
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
		packet.upperAniType = eUpperAnimationType::FIRING;

	packet.pos = GetPosition();
	packet.velocity = GetVelocity();
	packet.yaw = m_yaw;
	send(g_c_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
#endif
}

void Player::ApplyServerData(const PlayerData& playerData)
{
	// 애니메이션
	switch (playerData.aniType)
	{
	case eAnimationType::IDLE:
		if (GetCurrAnimationName() != "IDLE" && GetAfterAnimationName() != "IDLE")
			PlayAnimation("IDLE", TRUE);
		break;
	case eAnimationType::WALKING:
		if (GetCurrAnimationName() != "WALKING" && GetAfterAnimationName() != "WALKING")
			PlayAnimation("WALKING", TRUE);
		break;
	case eAnimationType::WALKLEFT:
		if (GetCurrAnimationName() != "WALKLEFT" && GetAfterAnimationName() != "WALKLEFT")
			PlayAnimation("WALKLEFT", TRUE);
		break;
	case eAnimationType::WALKRIGHT:
		if (GetCurrAnimationName() != "WALKRIGHT" && GetAfterAnimationName() != "WALKRIGHT")
			PlayAnimation("WALKRIGHT", TRUE);
		break;
	case eAnimationType::WALKBACK:
		if (GetCurrAnimationName() != "WALKBACK" && GetAfterAnimationName() != "WALKBACK")
			PlayAnimation("WALKBACK", TRUE);
		break;
	case eAnimationType::RUNNING:
		if (GetCurrAnimationName() != "RUNNING" && GetAfterAnimationName() != "RUNNING")
			PlayAnimation("RUNNING", TRUE);
		break;
	}

	// 상체 애니메이션
	switch (playerData.upperAniType)
	{
	case eUpperAnimationType::NONE:
		break;
	case eUpperAnimationType::RELOAD:
		if (GetUpperCurrAnimationName() != "RELOAD" && GetUpperAfterAnimationName() != "RELOAD")
			PlayAnimation("RELOAD", TRUE);
		break;
	case eUpperAnimationType::FIRING:
		if (GetUpperCurrAnimationName() != "FIRING" && GetUpperAfterAnimationName() != "FIRING")
			PlayAnimation("FIRING", TRUE);
		break;
	}

	SetPosition(playerData.pos);
	SetVelocity(playerData.velocity);
	Rotate(0.0f, 0.0f, playerData.yaw - m_yaw);
}

//void Player::SendPlayerData(eAnimationType legState) const
//{
//	cs_packet_update_legs packet;
//	packet.size = sizeof(packet);
//	packet.type = CS_PACKET_UPDATE_LEGS;
//	packet.state = legState;
//	packet.pos = GetPosition();
//	packet.velocity = GetVelocity();
//	packet.yaw = m_yaw;
//	send(g_c_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
//}

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
	if (!m_upperAnimationInfo) return "";
	return GetPureAnimationName(m_upperAnimationInfo->currAnimationName);
}

string Player::GetUpperAfterAnimationName() const
{
	if (!m_upperAnimationInfo) return "";
	return GetPureAnimationName(m_upperAnimationInfo->afterAnimationName);
}