#include "player.h"
#include "camera.h"

Player::Player(BOOL isMultiPlayer) : GameObject{},
	m_id{ -1 }, m_isMultiPlayer{ isMultiPlayer }, m_isFired{ FALSE }, m_gunType{ eGunType::NONE },
	m_delayRoll{}, m_delayPitch{}, m_delayYaw{}, m_delayTime{}, m_delayTimer{},
	m_hp{}, m_maxHp{}, m_speed{ 20.0f },
	m_shotSpeed{ 0.0f }, m_shotTimer{ 0.0f }, m_bulletCount{}, m_maxBulletCount{},
	m_camera{ nullptr }, m_gunMesh{ nullptr }, m_gunShader{ nullptr },
	m_gunOffset{}, m_gunOffsetTimer{}
{
	SharedBoundingBox bb{ make_shared<DebugBoundingBox>(XMFLOAT3{ 0.0f, 32.5f / 2.0f, 0.0f }, XMFLOAT3{ 8.0f / 2.0f, 32.5f / 2.0f, 8.0f / 2.0f }, XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f }) };
	m_boundingBoxes.push_back(bb);
}

void Player::OnMouseEvent(HWND hWnd, FLOAT deltaTime)
{
#ifdef FIRSTVIEW
	m_shotTimer += deltaTime;
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
	{
		if (m_shotTimer < m_shotSpeed || m_bulletCount == 0 || GetCurrAnimationName() == "RUNNING" || GetUpperCurrAnimationName() == "RELOAD")
			return;
		PlayAnimation("FIRING", GetUpperCurrAnimationName() != "FIRING");
		SendPlayerData();
		m_shotTimer = 0.0f;
	}
#endif
}

void Player::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

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
		if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) && !m_upperAnimationInfo && m_gunType != eGunType::MG && !(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
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
			if (GetAfterAnimationName() == "WALKING")
				PlayAnimation("IDLE");
			else
				PlayAnimation("IDLE", TRUE);
			m_velocity = XMFLOAT3{ 0.0f, 0.0f, 0.0f };
			SendPlayerData();
			break;
		case VK_SHIFT:
			// MG는 패스
			if (m_gunType == eGunType::MG)
				break;
			// 안뛰고 있을 때는 패스
			if (GetCurrAnimationName() != "RUNNING")
				break;
			// 달리기에서 다른 애니메이션으로 블렌딩 중일 때는 패스
			if (GetCurrAnimationName() == "RUNNING" && !m_animationInfo->afterAnimationName.empty())
				break;
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
			if (!m_upperAnimationInfo || (m_upperAnimationInfo && GetUpperCurrAnimationName() != "RELOAD" && GetUpperAfterAnimationName() != "RELOAD"))
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
		// 애니메이션에 맞춰 총 발사
		if (!m_isMultiPlayer && !m_isFired && m_upperAnimationInfo->state == eAnimationState::PLAY && GetUpperCurrAnimationName() == "FIRING")
		{
			switch (m_gunType)
			{
			case eGunType::AR:
				if (currFrame > 0.5f)
					Fire();
				break;
			case eGunType::SG:
				if (currFrame > 3.0f)
					Fire();
				break;
			case eGunType::MG:
				if (currFrame > 0.1f)
					Fire();
				break;
			}
		}

		if (currFrame >= endFrame)
		{
			switch (m_upperAnimationInfo->state)
			{
			case eAnimationState::PLAY:
				m_upperAnimationInfo->state = eAnimationState::SYNC;
				m_upperAnimationInfo->blendingTimer = 0.0f;

				// 재장전
				if (GetUpperCurrAnimationName() == "RELOAD")
					m_bulletCount = m_maxBulletCount;
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

	// 애니메이션이 끝났을 때
	if (currFrame >= endFrame)
	{
		switch (m_animationInfo->state)
		{
		case eAnimationState::PLAY:
		{
			// 이동
			string currPureAnimationName{ GetCurrAnimationName() };
			if (((GetAsyncKeyState('W') & 0x8000) && currPureAnimationName == "WALKING") ||
				((GetAsyncKeyState('W') & GetAsyncKeyState(VK_SHIFT) & 0x8000) && currPureAnimationName == "RUNNING") ||
				((GetAsyncKeyState('A') & 0x8000) && currPureAnimationName == "WALKLEFT") ||
				((GetAsyncKeyState('D') & 0x8000) && currPureAnimationName == "WALKRIGHT"))
			{
				PlayAnimation(currPureAnimationName);
				return;
			}
			if ((GetAsyncKeyState('S') & 0x8000) && currPureAnimationName == "WALKBACK")
			{
				PlayAnimation(currPureAnimationName, TRUE);
				m_animationInfo->blendingFrame = 2;
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
			break;
		}
		case eAnimationState::BLENDING:
			PlayAnimation(GetAfterAnimationName());
			m_animationInfo->blendingFrame = 5;
			break;
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
		m_camera->UpdateShaderVariableByPlayer(commandList); // 카메라 파라미터를 플레이어에 맞추고 셰이더 변수를 업데이트한다.
#endif
		if (m_gunType == eGunType::MG) // 머신건은 팔을 렌더링하지 않음
			UpdateShaderVariable(commandList);
		else
			GameObject::Render(commandList, shader);
		if (m_gunMesh)
		{
			if (shader) commandList->SetPipelineState(shader->GetPipelineState().Get());
			else if (m_gunShader) commandList->SetPipelineState(m_gunShader->GetPipelineState().Get());
			m_gunMesh->UpdateShaderVariable(commandList, this);
			m_gunMesh->Render(commandList);
		}
#ifdef FIRSTVIEW
		// 이후 렌더링할 객체들을 위해 카메라 파라미터를 이전 것으로 되돌린다.
		m_camera->SetAt(at);
		m_camera->SetUp(up);
		m_camera->Update(0.0f);
		m_camera->UpdateShaderVariable(commandList);
#endif
	}
}

void Player::RenderOutline(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	if (!m_outlineShader) return;

	XMFLOAT4X4 worldMatrix{ m_worldMatrix };
	XMFLOAT4X4 scale{};
	XMStoreFloat4x4(&scale, XMMatrixScaling(1.05f, 1.05f, 1.05f));
	m_worldMatrix = Matrix::Mul(scale, m_worldMatrix);
	Player::Render(commandList, m_outlineShader);
	m_worldMatrix = worldMatrix;
}

void Player::RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	GameObject::Render(commandList, m_shadowShader);
	if (m_gunMesh)
	{
		commandList->SetPipelineState(m_gunShadowShader->GetPipelineState().Get());
		m_gunMesh->UpdateShaderVariable(commandList, this);
		m_gunMesh->Render(commandList);
	}
}

void Player::Fire()
{
	// 총알 시작 좌표
	XMFLOAT3 start{ m_camera->GetEye() };

	// 화면 중앙
	XMFLOAT3 center{ m_camera->GetEye() };
	center = Vector3::Add(center, Vector3::Mul(m_camera->GetAt(), 1000.0f));

	switch (m_gunType)
	{
	case eGunType::NONE:
		break;
	case eGunType::AR:
	{
		// 총구에서 나오도록
		start = Vector3::Add(start, GetRight());
		start = Vector3::Add(start, Vector3::Mul(GetUp(), -0.5f));
		start = Vector3::Add(start, Vector3::Mul(m_camera->GetAt(), 1.0f));

		// 총알 발사 정보 서버로 송신
		cs_packet_bullet_fire packet{};
		packet.size = sizeof(packet);
		packet.type = CS_PACKET_BULLET_FIRE;
		packet.data = { start, Vector3::Normalize(Vector3::Sub(center, start)), static_cast<char>(GetId())};
		send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);

		// 반동
		DelayRotate(0.0f, -0.4f, Utile::Random(-0.1f, 0.1f), 0.1f);
		break;
	}
	case eGunType::SG:
	{
		// 총구에서 나오도록
		start = Vector3::Add(start, Vector3::Mul(GetRight(), 1.0f));
		start = Vector3::Add(start, Vector3::Mul(GetUp(), -0.5f));
		start = Vector3::Add(start, Vector3::Mul(m_camera->GetAt(), 5.0f));

		XMFLOAT3 up{ Vector3::Normalize(Vector3::Cross(m_camera->GetAt(), GetRight())) };

		// 각 총알의 목표 방향
		// 기본적으로 오각형 모양이고 거기서 랜덤으로 살짝 움직임
		XMFLOAT3 targets[]{ center, center, center, center, center };
		targets[0] = Vector3::Add(targets[0], Vector3::Mul(up, 20.0f));

		targets[1] = Vector3::Add(targets[1], Vector3::Mul(GetRight(), -25.0f));
		targets[1] = Vector3::Add(targets[1], Vector3::Mul(up, 10.0f));

		targets[2] = Vector3::Add(targets[2], Vector3::Mul(GetRight(), 25.0f));
		targets[2] = Vector3::Add(targets[2], Vector3::Mul(up, 10.0f));

		targets[3] = Vector3::Add(targets[3], Vector3::Mul(GetRight(), -15.0f));
		targets[3] = Vector3::Add(targets[3], Vector3::Mul(up, -10.0f));

		targets[4] = Vector3::Add(targets[4], Vector3::Mul(GetRight(), 15.0f));
		targets[4] = Vector3::Add(targets[4], Vector3::Mul(up, -10.0f));

		for (const XMFLOAT3& target : targets)
		{
			// 목표 방향을 랜덤으로 조금 움직임
			XMFLOAT3 t{ target };
			t = Vector3::Add(t, Vector3::Mul(GetRight(), Utile::Random(-5.0f, 5.0f)));
			t = Vector3::Add(t, Vector3::Mul(up, Utile::Random(-5.0f, 5.0f)));

			cs_packet_bullet_fire packet{};
			packet.size = sizeof(packet);
			packet.type = CS_PACKET_BULLET_FIRE;
			packet.data = { start, Vector3::Normalize(Vector3::Sub(t, start)), static_cast<char>(GetId()) };
			send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
		}

		// 반동
		DelayRotate(0.0f, -2.0f, 0.0f, 0.1f);
		break;
	}
	case eGunType::MG:
		// 총구에서 나오도록
		start = Vector3::Add(start, GetRight());
		start = Vector3::Add(start, Vector3::Mul(GetUp(), -0.5f));
		start = Vector3::Add(start, Vector3::Mul(m_camera->GetAt(), 1.0f));

		// 총알 방향을 살짝 흔듬
		XMFLOAT3 up{ Vector3::Normalize(Vector3::Cross(m_camera->GetAt(), GetRight())) };
		XMFLOAT3 right{ GetRight() };
		XMFLOAT3 target{ center };
		target = Vector3::Add(target, Vector3::Mul(right, Utile::Random(-15.0f, 15.0f)));
		target = Vector3::Add(target, Vector3::Mul(up, Utile::Random(-15.0f, 15.0f)));

		// 총알 발사 정보 서버로 송신
		cs_packet_bullet_fire packet{};
		packet.size = sizeof(packet);
		packet.type = CS_PACKET_BULLET_FIRE;
		packet.data = { start, Vector3::Normalize(Vector3::Sub(target, start)), static_cast<char>(GetId()) };
		send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);

		// 반동
		DelayRotate(0.0f, -0.4f, Utile::Random(-0.1f, 0.1f), 0.1f);
		break;
	}

	m_isFired = TRUE;
	--m_bulletCount;
}

void Player::DelayRotate(FLOAT roll, FLOAT pitch, FLOAT yaw, FLOAT time)
{
	m_delayRoll = roll;
	m_delayPitch = pitch;
	m_delayYaw = yaw;
	m_delayTime = time;
	m_delayTimer = time;
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

	// 회전해야되면 회전(반동)
	if (m_delayTimer > 0.0f)
	{
		Rotate(m_delayRoll * deltaTime / m_delayTime,
			   m_delayPitch * deltaTime / m_delayTime,
			   m_delayYaw * deltaTime / m_delayTime);
		m_delayTimer = max(0.0f, m_delayTimer - deltaTime);
	}

	// 달리고 있을 때 총 오프셋 변경
	if (GetCurrAnimationName() == "RUNNING" && GetAfterAnimationName().empty() && !m_upperAnimationInfo)
		m_gunOffsetTimer = min(0.5f, m_gunOffsetTimer + deltaTime);
	else
		m_gunOffsetTimer = max(0.0f, m_gunOffsetTimer - 10.0f * deltaTime);
}

void Player::Rotate(FLOAT roll, FLOAT pitch, FLOAT yaw)
{
	// 회전각 제한
	if (m_pitch + pitch > Setting::CAMERA_MAX_PITCH)
		pitch = Setting::CAMERA_MAX_PITCH - m_pitch;
	else if (m_pitch + pitch < Setting::CAMERA_MIN_PITCH)
		pitch = Setting::CAMERA_MIN_PITCH - m_pitch;

	// 카메라는 x,y축으로 회전할 수 있다.
	// GameObject::Rotate에서 플레이어의 로컬 x,y,z축을 변경하므로 먼저 호출해야한다.
	if (m_camera) m_camera->Rotate(0.0f, pitch, yaw);

	// 플레이어는 y축으로만 회전할 수 있다.
	GameObject::Rotate(0.0f, 0.0f, yaw);

	// 회전각 합산. yaw의 경우 GameObject::Rotate에서 합산됬다.
	m_roll += roll; m_pitch += pitch;

	if (!m_isMultiPlayer) SendPlayerData(); // 서버에게 회전했다고 알림
}

void Player::PlayAnimation(const string& animationName, BOOL doBlending)
{
	// 무기 타입에 따라 해당 무기에 맞는 애니메이션을 재생함
	// ex) AR을 착용한 플레이어가 IDLE이라는 애니메이션을 재생한다면 AR/IDLE 애니메이션이 재생됨
	
	// 아래의 애니메이션은 상체만 애니메이션함
	string pureAnimationName{ GetPureAnimationName(animationName) };
	if (pureAnimationName == "RELOAD" || pureAnimationName == "FIRING")
	{
		switch (m_gunType)
		{
		case eGunType::AR:
			PlayUpperAnimation("AR/" + pureAnimationName, doBlending);
			if (pureAnimationName == "FIRING")
			{
				m_upperAnimationInfo->blendingFrame = 3;
				m_upperAnimationInfo->fps = 1.0f / 24.0f;
				m_isFired = FALSE;
			}
			break;
		case eGunType::SG:
			PlayUpperAnimation("SG/" + pureAnimationName, doBlending);
			if (pureAnimationName == "FIRING")
			{
				m_upperAnimationInfo->blendingFrame = 3;
				m_upperAnimationInfo->fps = 1.0f / 30.0f;
				m_isFired = FALSE;
			}
			break;
		case eGunType::MG:
			PlayUpperAnimation("MG/" + pureAnimationName, doBlending);
			if (pureAnimationName == "FIRING")
			{
				m_upperAnimationInfo->blendingFrame = 3;
				m_upperAnimationInfo->fps = 1.0f / 30.0f;
				m_isFired = FALSE;
			}
			break;
		}
		return;
	}

	// 그 외는 상하체 모두 애니메이션함
	if (m_gunType == eGunType::AR) GameObject::PlayAnimation("AR/" + pureAnimationName, doBlending);
	else if (m_gunType == eGunType::SG) GameObject::PlayAnimation("SG/" + pureAnimationName, doBlending);
	else if (m_gunType == eGunType::MG) GameObject::PlayAnimation("MG/" + pureAnimationName, doBlending);
}

void Player::SetGunType(eGunType gunType)
{
	switch (gunType)
	{
	case eGunType::AR:
		m_hp = m_maxHp = 150;
		m_shotSpeed = 0.16f;
		m_bulletCount = m_maxBulletCount = 30;
		m_gunOffset = XMFLOAT3{ 0.0f, 30.0f, -1.0f };
		break;
	case eGunType::SG:
		m_hp = m_maxHp = 175;
		m_shotSpeed = 0.8f;
		m_bulletCount = m_maxBulletCount = 8;
		m_gunOffset = XMFLOAT3{ 0.0f, 30.0f, -1.0f };
		break;
	case eGunType::MG:
		m_hp = m_maxHp = 200;
		m_shotSpeed = 0.1f;
		m_bulletCount = m_maxBulletCount = 100;
		m_gunOffset = XMFLOAT3{ 0.0f, 19.0f, 0.0f };
		break;
	}
	m_shotTimer = 0.0f;
	m_gunType = gunType;
	g_playerGunType = gunType;
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
	cs_packet_update_player packet;
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_UPDATE_PLAYER;
	packet.aniType = GetAnimationType();
	packet.upperAniType = GetUpperAnimationType();
	packet.pos = GetPosition();
	packet.velocity = GetVelocity();
	packet.yaw = m_yaw;
	send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
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

void Player::SetGunShadowShader(const shared_ptr<Shader>& shadowShader)
{
	m_gunShadowShader = shadowShader;
}

INT Player::GetId() const
{
	return m_id;
}

eGunType Player::GetGunType() const
{
	return m_gunType;
}

INT Player::GetHp() const
{
	return m_hp;
}

INT Player::GetMaxHp() const
{
	return m_maxHp;
}

INT Player::GetBulletCount() const
{
	return m_bulletCount;
}

INT Player::GetMaxBulletCount() const
{
	return m_maxBulletCount;
}

string Player::GetPureAnimationName(const string& animationName) const
{
	if (auto p{ animationName.find_last_of('/') }; p != string::npos)
		return animationName.substr(p + 1);
	return animationName;
}

string Player::GetCurrAnimationName() const
{
	if (!m_animationInfo) return "";
	return GetPureAnimationName(m_animationInfo->currAnimationName);
}

string Player::GetAfterAnimationName() const
{
	if (!m_animationInfo) return "";
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

eAnimationType Player::GetAnimationType() const
{
	static auto pred = [](const string& aniName) {
		if (aniName == "IDLE")
			return eAnimationType::IDLE;
		else if (aniName == "RUNNING")
			return eAnimationType::RUNNING;
		else if (aniName == "WALKING")
			return eAnimationType::WALKING;
		else if (aniName == "WALKLEFT")
			return eAnimationType::WALKLEFT;
		else if (aniName == "WALKRIGHT")
			return eAnimationType::WALKRIGHT;
		else if (aniName == "WALKBACK")
			return eAnimationType::WALKBACK;
		return eAnimationType::NONE;
	};
	if (!m_animationInfo) return eAnimationType::NONE;
	if (m_animationInfo->afterAnimationName.empty())
		return pred(GetCurrAnimationName());
	else
		return pred(GetAfterAnimationName());
}

eUpperAnimationType Player::GetUpperAnimationType() const
{
	static auto pred = [](const string& aniName) {
		if (aniName == "RELOAD")
			return eUpperAnimationType::RELOAD;
		else if (aniName == "FIRING")
			return eUpperAnimationType::FIRING;
		return eUpperAnimationType::NONE;
	};
	if (!m_upperAnimationInfo) return eUpperAnimationType::NONE;
	if (m_upperAnimationInfo->afterAnimationName.empty())
		return pred(GetUpperCurrAnimationName());
	else
		return pred(GetUpperAfterAnimationName());
}

XMFLOAT3 Player::GetGunOffset() const
{
	return m_gunOffset;
}

FLOAT Player::GetGunOffsetTimer() const
{
	return m_gunOffsetTimer;
}
