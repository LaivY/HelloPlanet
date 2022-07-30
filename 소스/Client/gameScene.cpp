#include "stdafx.h"
#include "gameScene.h"
#include "audioEngine.h"
#include "camera.h"
#include "filter.h"
#include "framework.h"
#include "object.h"
#include "player.h"
#include "textObject.h"
#include "texture.h"
#include "uiObject.h"
#include "windowObject.h"

vector<unique_ptr<Monster>>		GameScene::s_monsters;
vector<unique_ptr<GameObject>>	GameScene::s_screenObjects;

GameScene::GameScene() : m_pcbGameScene{ nullptr }, m_isShowing{ false }
{
	ShowCursor(FALSE);
}

GameScene::~GameScene()
{
	// 서버에게 로그아웃 패킷을 보냄
	cs_packet_logout packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_LOGOUT;
	send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), NULL);

	if (g_networkThread.joinable())
		g_networkThread.join();
	closesocket(g_socket);
	WSACleanup();

	// 정적 변수들 초기화
	s_monsters.clear();
	s_screenObjects.clear();
}

void GameScene::OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
					   const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postRootSignature,
					   const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	CreateShaderVariable(device, commandList);
	CreateGameObjects(device, commandList);
	CreateUIObjects(device, commandList);
	CreateTextObjects(d2dDeivceContext, dWriteFactory);
	CreateLights();
	LoadMapObjects(device, commandList, Utile::PATH("map.txt"));

	// 외곽선을 위한 깊이, 스텐실 버퍼 텍스쳐
	s_textures["DEPTH"] = make_unique<Texture>();
	s_textures["DEPTH"]->Create(device, DXGI_FORMAT_R24G8_TYPELESS, g_width, g_height, 7, eTextureType::DEPTH);
	s_textures["STENCIL"] = make_unique<Texture>();
	s_textures["STENCIL"]->Create(device, DXGI_FORMAT_R24G8_TYPELESS, g_width, g_height, 8, eTextureType::STENCIL);
	Texture::CreateShaderResourceView(device);

	// 블러 필터
	m_blurFilter = make_unique<BlurFilter>(device, commandList);

#ifdef NETWORK
	g_networkThread = thread{ &GameScene::ProcessClient, this };
#endif
}

void GameScene::OnDestroy()
{
	cs_packet_logout packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_LOGOUT;
	send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), NULL);

	if (g_networkThread.joinable())
		g_networkThread.join();
	closesocket(g_socket);
	WSACleanup();
}

void GameScene::OnMouseEvent(HWND hWnd, FLOAT deltaTime)
{
	static bool isCursorHide{ true };

	if (!g_gameFramework.isActive())
		return;
	if (!m_windowObjects.empty())
	{
		if (isCursorHide)
			ShowCursor(TRUE);
		isCursorHide = false;

		// 플레이어가 키보드를 누르고 있을 때 윈도우가 생기면
		// 해당 상태가 유지되므로 속도와 애니메이션을 기본 상태로 설정해줌
		if (m_player->GetHp() > 0)
		{
			m_player->SetVelocity(XMFLOAT3{ 0.0f, 0.0f, 0.0f });
			if (m_player->GetCurrAnimationName() != "IDLE" && m_player->GetAfterAnimationName() != "IDLE")
			{
				m_player->PlayAnimation("IDLE", TRUE);
				m_player->SendPlayerData();
			}
		}
		m_windowObjects.back()->OnMouseEvent(hWnd, deltaTime);
		return;
	}
	if ((GetAsyncKeyState(VK_TAB) & 0x8000))
	{
		if (isCursorHide)
			ShowCursor(TRUE);
		isCursorHide = false;
		return;
	}
	if (!isCursorHide)
	{
		ShowCursor(FALSE);
		isCursorHide = true;
	}

	// 화면 가운데 좌표 계산
	RECT rect; GetWindowRect(hWnd, &rect);
	POINT oldMousePosition{ static_cast<LONG>(rect.left + g_width / 2), static_cast<LONG>(rect.top + g_height / 2) };

	// 움직인 마우스 좌표
	POINT newMousePosition; GetCursorPos(&newMousePosition);

	// 움직인 정도에 비례해서 회전
	constexpr float sensitive{ 2.5f };
	int dx = newMousePosition.x - oldMousePosition.x;
	int dy = newMousePosition.y - oldMousePosition.y;
	if (dx != 0 || dy != 0)
	{
#ifdef FREEVIEW
		if (m_camera) m_camera->Rotate(0.0f, dy * sensitive * deltaTime, dx * sensitive * deltaTime);
#endif
#ifdef FIRSTVIEW
		if (m_player) m_player->Rotate(0.0f, dy * sensitive * deltaTime, dx * sensitive * deltaTime);

		// 관전 상태일 경우 카메라 회전
		if (m_player->GetHp() <= 0)
			m_camera->Rotate(0.0f, dy * sensitive * deltaTime, dx * sensitive * deltaTime);
#endif
#ifdef THIRDVIEW
		if (m_player) m_player->Rotate(0.0f, dy * sensitive * deltaTime, dx * sensitive * deltaTime);
#endif
		// 마우스를 화면 가운데로 이동
		SetCursorPos(oldMousePosition.x, oldMousePosition.y);
	}
	if (m_player) m_player->OnMouseEvent(hWnd, deltaTime);
}

void GameScene::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (!g_gameFramework.isActive())
		return;
	if (!m_windowObjects.empty())
	{
		m_windowObjects.back()->OnMouseEvent(hWnd, message, wParam, lParam);
		return;
	}
	if (m_camera) m_camera->OnMouseEvent(hWnd, message, wParam, lParam);
	if (m_player) m_player->OnMouseEvent(hWnd, message, wParam, lParam);
}

void GameScene::OnKeyboardEvent(FLOAT deltaTime)
{
	if (!g_gameFramework.isActive())
		return;
	if (!m_windowObjects.empty())
	{
		m_windowObjects.back()->OnKeyboardEvent(deltaTime);
		return;
	}
#ifdef FREEVIEW
	const float speed{ 100.0f * deltaTime };
	if (GetAsyncKeyState('W') & 0x8000)
	{
		m_camera->Move(Vector3::Mul(m_camera->GetAt(), speed));
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		XMFLOAT3 right{ Vector3::Cross(m_camera->GetUp(), m_camera->GetAt()) };
		m_camera->Move(Vector3::Mul(right, -1.0f * speed));
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		m_camera->Move(Vector3::Mul(m_camera->GetAt(), -1.0f * speed));
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		XMFLOAT3 right{ Vector3::Cross(m_camera->GetUp(), m_camera->GetAt()) };
		m_camera->Move(Vector3::Mul(right, speed));
	}
	if (GetAsyncKeyState(' ') & 0x8000)
	{
		m_camera->Move(Vector3::Mul(XMFLOAT3{ 0.0f, 1.0f, 0.0f }, speed));
	}
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{
		m_camera->Move(Vector3::Mul(XMFLOAT3{ 0.0f, -1.0f, 0.0f }, speed));
	}
#else
	if (m_player) m_player->OnKeyboardEvent(deltaTime);
#endif
}

void GameScene::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (!g_gameFramework.isActive())
		return;
	if (!m_windowObjects.empty())
	{
		m_windowObjects.back()->OnKeyboardEvent(hWnd, message, wParam, lParam);
		return;
	}

	if (m_player) m_player->OnKeyboardEvent(hWnd, message, wParam, lParam);
	switch (message)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_LEFT:
			if (m_multiPlayers[0])
				m_camera->SetPlayer(m_multiPlayers[0].get());
			break;
		case VK_RIGHT:
			if (m_multiPlayers[1])
				m_camera->SetPlayer(m_multiPlayers[1].get());
			break;
		case VK_ESCAPE:
			CreateExitWindow();
			break;
		}
		break;
	}
}

void GameScene::OnUpdate(FLOAT deltaTime)
{
	Update(deltaTime);
	if (m_player) m_player->Update(deltaTime);
	if (m_camera) m_camera->Update(deltaTime);
	if (m_skybox) m_skybox->Update(deltaTime);
	for (auto& ui : m_uiObjects)
		ui->Update(deltaTime);
	for (auto& o : s_screenObjects)
		o->Update(deltaTime);

	unique_lock<mutex> lock{ g_mutex };
	for (auto& p : m_multiPlayers)
		if (p) p->Update(deltaTime);
	for (auto& o : m_gameObjects)
		o->Update(deltaTime);
	for (auto& t : m_textObjects)
		t->Update(deltaTime);
	for (auto& w : m_windowObjects)
		w->Update(deltaTime);
	for (auto& m : s_monsters)
		m->Update(deltaTime);
}

void GameScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	memcpy(m_pcbGameScene, m_cbGameSceneData.get(), sizeof(cbGameScene));
	commandList->SetGraphicsRootConstantBufferView(3, m_cbGameScene->GetGPUVirtualAddress());
}

void GameScene::PreRender(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	RenderToShadowMap(commandList);
}

void GameScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	// 카메라 셰이더 변수 최신화
	m_camera->UpdateShaderVariable(commandList);

	// 뷰포트, 가위사각형, 렌더타겟 설정
	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);
	commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

	// 스카이박스 렌더링
	if (m_skybox) m_skybox->Render(commandList);

	unique_lock<mutex> lock{ g_mutex };
	RenderGameObjects(commandList, rtvHandle, dsvHandle);
	RenderMultiPlayers(commandList, rtvHandle, dsvHandle);
	RenderMonsters(commandList, rtvHandle, dsvHandle);
	if (!m_isShowing)
	{
		RenderScreenObjects(commandList, rtvHandle, dsvHandle);
		RenderPlayer(commandList, rtvHandle, dsvHandle);
		RenderUIObjects(commandList, rtvHandle, dsvHandle);
	}
}

void GameScene::Render2D(const ComPtr<ID2D1DeviceContext2>& device) const
{
	if (m_isShowing) return;

	unique_lock<mutex> lock{ g_mutex };
	for (const auto& t : m_textObjects)
		t->Render(device);
	for (const auto& w : m_windowObjects)
		w->Render2D(device);
}

void GameScene::PostProcessing(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& postRootSignature, const ComPtr<ID3D12Resource>& renderTarget) const
{
	// 조준 시 주변 블러링
	if (m_player->GetIsFocusing())
		m_blurFilter->Excute(commandList, postRootSignature, renderTarget);
}

void GameScene::ProcessClient()
{
	while (g_isConnected)
		RecvPacket();
}

void GameScene::OnPlayerHit(Monster* monster)
{
	// 이미 죽은 상태면 피격 당하지 않음
	if (m_player->GetHp() <= 0)
		return;

	// 체력 감소
	int hp{ m_player->GetHp() };
	m_player->SetHp(hp - static_cast<INT>(monster->GetDamage()));

	// 애니메이션
	if (hp > 0 && m_player->GetHp() <= 0)
		OnPlayerDie();
	else if (m_player->GetHp() > 0)
	{
		m_player->PlayAnimation("HIT");
		m_player->SendPlayerData();
	}

	// 효과음
	g_audioEngine.Play("HIT");

	// 피격 이펙트
	auto hit{ make_unique<HitUIObject>(monster->GetId()) };
	m_uiObjects.push_back(move(hit));
}

void GameScene::OnPlayerDie()
{
	// 플레이어가 죽으면 카메라를 3인칭으로 변경하고 사망 애니메이션을 재생함
	m_camera.swap(m_observeCamera);
	m_skybox->SetCamera(m_camera);
	m_player->SetSkillGage(0.0f);
	m_player->SetMesh(s_meshes["PLAYER"]);
	m_player->PlayAnimation("DIE", TRUE);
	m_player->SetVelocity(XMFLOAT3{});
	m_player->SendPlayerData();

	// 효과음
	g_audioEngine.Play("DEATH");

	// 서버에 죽었다고 알림
	cs_packet_player_state packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_PLAYER_STATE;
	packet.playerId = m_player->GetId();
	packet.playerState = ePlayerState::DIE;
	send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), NULL);
}

void GameScene::OnPlayerRevive()
{
	// 플레이어가 부활하면 카메라를 1인칭으로 변경
	m_player->SetHp(m_player->GetMaxHp());

	m_camera.swap(m_observeCamera);
	m_skybox->SetCamera(m_camera);
	m_player->SetMesh(s_meshes["ARM"]);
	m_player->PlayAnimation("IDLE");
	m_player->SendPlayerData();
}

Player* GameScene::GetPlayer() const
{
	return m_player.get();
}

Camera* GameScene::GetCamera() const
{
	return m_camera.get();
}

void GameScene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_cbGameScene = Utile::CreateBufferResource(device, commandList, NULL, Utile::GetConstantBufferSize<cbGameScene>(), 1, D3D12_HEAP_TYPE_UPLOAD, {});
	m_cbGameScene->Map(0, NULL, reinterpret_cast<void**>(&m_pcbGameScene));
	m_cbGameSceneData = make_unique<cbGameScene>();
}

void GameScene::CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 카메라 생성
	XMFLOAT4X4 projMatrix{};
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(g_width) / static_cast<float>(g_height), 1.0f, 2500.0f));
#ifdef FREEVIEW
	m_camera = make_shared<Camera>();
#endif
#ifdef FIRSTVIEW
	m_camera = make_shared<Camera>();
#endif
#ifdef THIRDVIEW
	m_camera = make_shared<ThirdPersonCamera>();
#endif
	m_camera->CreateShaderVariable(device, commandList);
	m_camera->SetProjMatrix(projMatrix);

	// 연출 카메라 생성
	m_showCamera = make_shared<ShowCamera>();
	m_showCamera->CreateShaderVariable(device, commandList);
	m_showCamera->SetProjMatrix(projMatrix);

	// 관전 카메라 생성
	m_observeCamera = make_shared<ThirdPersonCamera>();
	m_observeCamera->CreateShaderVariable(device, commandList);
	m_observeCamera->SetProjMatrix(projMatrix);

	// 스크린 카메라 생성
	m_screenCamera = make_unique<Camera>();
	m_screenCamera->CreateShaderVariable(device, commandList);
	m_screenCamera->SetProjMatrix(projMatrix);

	// 플레이어, 멀티플레이어 설정
	// 플레이어와 멀티플레이어는 이전 로비 씬에서 만들어져있다.
	m_player->SetIsMultiplayer(FALSE);
	m_player->PlayAnimation("IDLE");
	m_player->DeleteUpperAnimation();
	m_player->SetSkillGage(0);
	switch (m_player->GetId())
	{
	case 0:
		m_player->SetPosition(XMFLOAT3{ -50.0f, 0.0f, 0.0f });
		break;
	case 1:
		m_player->SetPosition(XMFLOAT3{});
		break;
	case 2:
		m_player->SetPosition(XMFLOAT3{ 50.0f, 0.0f, 0.0f });
		break;
	}
	m_player->SendPlayerData();

	for (auto& p : m_multiPlayers)
	{
		if (!p) continue;
		p->PlayAnimation("IDLE");
		p->DeleteUpperAnimation();

		switch (p->GetId())
		{
		case 0:
			p->SetPosition(XMFLOAT3{ -50.0f, 0.0f, 0.0f });
			break;
		case 1:
			p->SetPosition(XMFLOAT3{});
			break;
		case 2:
			p->SetPosition(XMFLOAT3{ 50.0f, 0.0f, 0.0f });
			break;
		}
	}

	// 카메라, 플레이어 설정
	m_player->SetCamera(m_camera.get());
	m_camera->SetPlayer(m_player.get());
	m_observeCamera->SetPlayer(m_player.get());

	// 스카이박스
	m_skybox = make_unique<Skybox>();
	m_skybox->SetCamera(m_camera);

	// 바닥
	auto floor{ make_unique<GameObject>() };
	floor->SetMesh(s_meshes["FLOOR"]);
	floor->SetShader(s_shaders["DEFAULT"]);
	m_gameObjects.push_back(move(floor));

	// 외곽선 그리는 객체
	m_redOutliner = make_unique<OutlineObject>(XMFLOAT3{ 1.0f, 0.0f, 0.0f }, 1.0f);
	m_greenOutliner = make_unique<OutlineObject>(XMFLOAT3{ 0.0f, 1.0f, 0.0f }, 1.0f);
	m_blackOutliner = make_unique<OutlineObject>(XMFLOAT3{ 0.1f, 0.1f, 0.1f }, 1.0f);

	// 파티클
	auto particle{ make_unique<DustParticle>() };
	m_gameObjects.push_back(move(particle));
}

void GameScene::CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// UI 카메라 생성
	XMFLOAT4X4 projMatrix{};
	XMStoreFloat4x4(&projMatrix, XMMatrixOrthographicLH(static_cast<float>(g_width), static_cast<float>(g_height), 0.0f, 1.0f));
	m_uiCamera = make_unique<Camera>();
	m_uiCamera->CreateShaderVariable(device, commandList);
	m_uiCamera->SetProjMatrix(projMatrix);

	// 조준점
	auto crosshair{ make_unique<CrosshairUIObject>(2.0f, 10.0f) };
	crosshair->SetMesh(s_meshes["UI"]);
	crosshair->SetShader(s_shaders["UI"]);
	crosshair->SetTexture(s_textures["WHITE"]);
	crosshair->SetPlayer(m_player);
	m_uiObjects.push_back(move(crosshair));

	// 체력바 베이스
	auto hpBarBase{ make_unique<UIObject>(200.0f, 30.0f) };
	hpBarBase->SetMesh(s_meshes["UI"]);
	hpBarBase->SetShader(s_shaders["UI"]);
	hpBarBase->SetTexture(s_textures["HPBARBASE"]);
	hpBarBase->SetPivot(ePivot::LEFTBOT);
	hpBarBase->SetScreenPivot(ePivot::LEFTBOT);
	hpBarBase->SetPosition(XMFLOAT2{ 50.0f, 50.0f });
	m_uiObjects.push_back(move(hpBarBase));

	// 체력바
	auto hpBar{ make_unique<HpUIObject>(200.0f, 30.0f) };
	hpBar->SetMesh(s_meshes["UI"]);
	hpBar->SetShader(s_shaders["UI"]);
	hpBar->SetTexture(s_textures["HPBAR"]);
	hpBar->SetPivot(ePivot::LEFTBOT);
	hpBar->SetScreenPivot(ePivot::LEFTBOT);
	hpBar->SetPosition(XMFLOAT2{ 50.0f, 50.0f });
	hpBar->SetPlayer(m_player);
	m_uiObjects.push_back(move(hpBar));

	// 자동 타겟팅
	auto autoTarget{ make_unique<AutoTargetUIObject>() };
	m_uiObjects.push_back(move(autoTarget));

	// 스킬 게이지
	auto skillGage{ make_unique<SkillGageUIObject>() };
	skillGage->SetPivot(ePivot::CENTERBOT);
	skillGage->SetScreenPivot(ePivot::CENTERBOT);
	skillGage->SetPosition(XMFLOAT2{ 0.0f, 40.0f });
	m_uiObjects.push_back(move(skillGage));

	// 체력 경고
	auto warning{ make_unique<WarningUIObject>() };
	m_uiObjects.push_back(move(warning));
}

void GameScene::CreateTextObjects(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	// 텍스트 오브젝트들의 좌표계는 좌측 상단이 (0, 0), 우측 하단이 (width, height) 이다.
	// 총알
	auto bulletText{ make_unique<BulletTextObject>() };
	bulletText->SetScreenPivot(ePivot::RIGHTBOT);
	bulletText->SetPosition(XMFLOAT2{ -160.0f, -80.0f });
	m_textObjects.push_back(move(bulletText));

	// 체력
	auto hpText{ make_unique<HPTextObject>() };
	hpText->SetScreenPivot(ePivot::LEFTBOT);
	hpText->SetPosition(XMFLOAT2{ 50.0f, -125.0f });
	m_textObjects.push_back(move(hpText));

	// 스킬게이지
	auto skillGage{ make_unique<SkillGageTextObject>() };
	skillGage->SetScreenPivot(ePivot::CENTERBOT);
	skillGage->SetPosition(XMFLOAT2{ -1.0f, -113.0f });
	m_textObjects.push_back(move(skillGage));
}

void GameScene::CreateLights() const
{
	m_cbGameSceneData->shadowLight.color = XMFLOAT3{ 0.5f, 0.5f, 0.5f };
	m_cbGameSceneData->shadowLight.direction = Vector3::Normalize(XMFLOAT3{ 0.2f, -1.0f, 0.2f });

	// 그림자를 만드는 조명의 마지막 뷰, 투영 변환 행렬의 경우 씬을 덮는 영역으로, 업데이트할 필요 없음
	XMFLOAT4X4 lightViewMatrix, lightProjMatrix;
	XMFLOAT3 shadowLightPos{ Vector3::Mul(m_cbGameSceneData->shadowLight.direction, -1500.0f) };
	XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
	XMStoreFloat4x4(&lightViewMatrix, XMMatrixLookAtLH(XMLoadFloat3(&shadowLightPos), XMLoadFloat3(&m_cbGameSceneData->shadowLight.direction), XMLoadFloat3(&up)));
	XMStoreFloat4x4(&lightProjMatrix, XMMatrixOrthographicLH(3000.0f, 3000.0f, 0.0f, 5000.0f));
	m_cbGameSceneData->shadowLight.lightViewMatrix[Setting::SHADOWMAP_COUNT - 1] = Matrix::Transpose(lightViewMatrix);
	m_cbGameSceneData->shadowLight.lightProjMatrix[Setting::SHADOWMAP_COUNT - 1] = Matrix::Transpose(lightProjMatrix);

	m_cbGameSceneData->ligths[0].color = XMFLOAT3{ 0.1f, 0.1f, 0.1f };
	m_cbGameSceneData->ligths[0].direction = Vector3::Normalize(XMFLOAT3{ 0.0f, -1.0f, 0.0f });

	m_cbGameSceneData->ligths[1].color = XMFLOAT3{ 0.1f, 0.1f, 0.1f };
	m_cbGameSceneData->ligths[1].direction = Vector3::Normalize(XMFLOAT3{ 1.0f, -1.0f, -1.0f });

	m_cbGameSceneData->ligths[2].color = XMFLOAT3{ 0.1f, 0.1f, 0.1f };
	m_cbGameSceneData->ligths[2].direction = Vector3::Normalize(XMFLOAT3{ -1.0f, -2.0f, 2.0f });
}

void GameScene::LoadMapObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& mapFile)
{
	// 히트박스를 가질 더미 게임오브젝트
	auto dumy{ make_unique<GameObject>() };

	ifstream map{ mapFile };
	int count{ 0 }; map >> count;
	for (int i = 0; i < count; ++i)
	{
		// 타입, 스케일, 회전, 이동
		int type{}; map >> type;
		XMFLOAT3 scale{}; map >> scale.x >> scale.y >> scale.z;
		XMFLOAT3 rotat{}; map >> rotat.x >> rotat.y >> rotat.z;
		XMFLOAT3 trans{}; map >> trans.x >> trans.y >> trans.z;
		trans = Vector3::Mul(trans, 100.0f);

		unique_ptr<GameObject> object{ make_unique<GameObject>() };
		object->SetScale(scale);
		object->Rotate(rotat.z, rotat.x, rotat.y);
		object->SetPosition(trans);
		object->SetMesh(s_meshes["OBJECT" + to_string(type)]);
		object->SetShader(s_shaders["MODEL"]);
		object->SetShadowShader(s_shaders["SHADOW_MODEL"]);
		object->SetOutline(TRUE);

		// 텍스쳐
		if (0 <= type && type <= 1)
			object->SetTexture(s_textures["OBJECT0"]);
		else if (2 <= type && type <= 8)
			object->SetTexture(s_textures["OBJECT1"]);
		else if (10 <= type && type <= 11)
			object->SetTexture(s_textures["OBJECT2"]);

		m_gameObjects.push_back(move(object));

		// 히트박스
		if (type == 12)
		{
			auto hitbox{ make_unique<Hitbox>(trans, Vector3::Mul(scale, 50.0f), XMFLOAT3{ rotat.z, rotat.x, rotat.y }) };
			hitbox->SetOwner(dumy.get());
			dumy->AddHitbox(move(hitbox));
		}
	}
	m_gameObjects.push_back(move(dumy));
}

void GameScene::RenderGameObjects(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	// 외곽선 있는 게임오브젝트들 렌더링
	UINT stencilRef{ 1 };
	for (const auto& o : m_gameObjects | views::filter([](const auto& o) { return o->isMakeOutline(); }))
	{
		commandList->OMSetStencilRef(stencilRef++);
		o->Render(commandList);
	}
	RenderOutline(commandList, m_blackOutliner);

	// 외곽선 없는 게임오브젝트들 렌더링
	for (const auto& o : m_gameObjects | views::filter([](const auto& o) { return !o->isMakeOutline(); }))
		o->Render(commandList);
}

void GameScene::RenderMultiPlayers(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	UINT stencilRef{ 1 };
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
	for (const auto& p : m_multiPlayers)
	{
		if (!p) continue;
		commandList->OMSetStencilRef(stencilRef++);
		p->Render(commandList);
	}
	RenderOutline(commandList, m_greenOutliner);
}

void GameScene::RenderMonsters(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	UINT stencilRef{ 1 };
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
	for (const auto& m : s_monsters)
	{
		commandList->OMSetStencilRef(stencilRef++);
		m->Render(commandList);
	}
	RenderOutline(commandList, m_redOutliner);
}

void GameScene::RenderScreenObjects(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	m_screenCamera->UpdateShaderVariable(commandList);
	for (const auto& o : s_screenObjects)
		o->Render(commandList);
}

void GameScene::RenderPlayer(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	m_camera->UpdateShaderVariable(commandList);
	commandList->OMSetStencilRef(1);

	D3D12_CLEAR_FLAGS flag{ D3D12_CLEAR_FLAG_STENCIL };
	if (m_player->GetHp() > 0)
		flag |= D3D12_CLEAR_FLAG_DEPTH;
	commandList->ClearDepthStencilView(dsvHandle, flag, 1.0f, 0, 0, NULL);
	m_player->Render(commandList);
	RenderOutline(commandList, m_blackOutliner);
	commandList->OMSetStencilRef(0);
}

void GameScene::RenderUIObjects(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	m_uiCamera->UpdateShaderVariable(commandList);
	for (const auto& ui : m_uiObjects)
		ui->Render(commandList);
	for (const auto& w : m_windowObjects)
		w->Render(commandList);
}

void GameScene::CreateExitWindow()
{
	auto title{ make_unique<TextObject>() };
	title->SetBrush("BLACK");
	title->SetFormat("36L");
	title->SetText(TEXT("알림"));
	title->SetPivot(ePivot::LEFTCENTER);
	title->SetScreenPivot(ePivot::LEFTTOP);
	title->SetPosition(XMFLOAT2{ 0.0f, title->GetHeight() / 2.0f + 2.0f });

	auto text{ make_unique<TextObject>() };
	text->SetBrush("BLACK");
	text->SetFormat("32L");
	text->SetText(TEXT("메인 화면으로\n돌아갈까요?"));
	text->SetPivot(ePivot::CENTER);
	text->SetScreenPivot(ePivot::CENTER);
	text->SetPosition(XMFLOAT2{});

	auto okText{ make_unique<MenuTextObject>() };
	okText->SetBrush("BLUE");
	okText->SetMouseOverBrush("BLUE");
	okText->SetFormat("36L");
	okText->SetText(TEXT("확인"));
	okText->SetPivot(ePivot::CENTERBOT);
	okText->SetScreenPivot(ePivot::CENTERBOT);
	okText->SetPosition(XMFLOAT2{ -40.0f, -okText->GetHeight() / 2.0f + 10.0f });
	okText->SetMouseClickCallBack(
		[]()
		{
			ShowCursor(TRUE);
			g_gameFramework.SetNextScene(eSceneType::MAIN);
		});

	auto cancleText{ make_unique<MenuTextObject>() };
	cancleText->SetBrush("RED");
	cancleText->SetMouseOverBrush("RED");
	cancleText->SetFormat("36L");
	cancleText->SetText(TEXT("취소"));
	cancleText->SetPivot(ePivot::CENTERBOT);
	cancleText->SetScreenPivot(ePivot::CENTERBOT);
	cancleText->SetPosition(XMFLOAT2{ 40.0f, -cancleText->GetHeight() / 2.0f + 10.0f });
	//cancleText->SetMouseClickCallBack(bind(&GameScene::CloseWindow, this));
	cancleText->SetMouseClickCallBack(
		[&]() 
		{
			m_windowObjects.back()->Delete();
		});

	auto window{ make_unique<WindowObject>(400.0f, 250.0f) };
	window->SetTexture(s_textures["WHITE"]);
	window->Add(title);
	window->Add(text);
	window->Add(okText);
	window->Add(cancleText);
	m_windowObjects.push_back(move(window));
}

void GameScene::Update(FLOAT deltaTime)
{
	unique_lock<mutex> lock{ g_mutex };
	erase_if(m_gameObjects, [](unique_ptr<GameObject>& object) { return !object->isValid(); });
	erase_if(m_uiObjects, [](unique_ptr<UIObject>& object) { return !object->isValid(); });
	erase_if(m_textObjects, [](unique_ptr<TextObject>& object) { return !object->isValid(); });
	erase_if(m_windowObjects, [](unique_ptr<WindowObject>& object) { return !object->isValid(); });
	erase_if(s_screenObjects, [](unique_ptr<GameObject>& object) { return !object->isValid(); });
	erase_if(s_monsters, [](const unique_ptr<Monster>& object) { return !object->isValid(); });
	lock.unlock();

	PlayerCollisionCheck(deltaTime);
	UpdateShadowMatrix();
}

void GameScene::PlayerCollisionCheck(FLOAT deltaTime)
{
	// 플레이어가 맵 밖으로 나가지 못하도록
	XMFLOAT3 position{ m_player->GetPosition() };
	position.x = clamp(position.x, -700.0f, 700.0f);
	position.z = clamp(position.z, -625.0f, 700.0f);
	m_player->SetPosition(position);

	// 플레이어 바운딩박스
	auto playerBoundingBox{ m_player->GetHitboxes().front()->GetBoundingBox() };

	// 바운딩박스의 모서리를 담을 배열
	XMFLOAT3 temp[8]{};
	playerBoundingBox.GetCorners(temp);

	XMFLOAT3 corner[4]{};
	corner[0] = XMFLOAT3{ temp[0].x, 0.0f, temp[0].z };
	corner[1] = XMFLOAT3{ temp[1].x, 0.0f, temp[1].z };
	corner[2] = XMFLOAT3{ temp[5].x, 0.0f, temp[5].z };
	corner[3] = XMFLOAT3{ temp[4].x, 0.0f, temp[4].z };

	for (const auto& o : m_gameObjects)
		for (const auto& hb : o->GetHitboxes())
		{
			auto objectBoundingBox{ hb->GetBoundingBox() };
			if (!playerBoundingBox.Intersects(objectBoundingBox)) continue;

			objectBoundingBox.GetCorners(temp);

			XMFLOAT3 oCorner[4]{};
			oCorner[0] = XMFLOAT3{ temp[0].x, 0.0f, temp[0].z };
			oCorner[1] = XMFLOAT3{ temp[1].x, 0.0f, temp[1].z };
			oCorner[2] = XMFLOAT3{ temp[5].x, 0.0f, temp[5].z };
			oCorner[3] = XMFLOAT3{ temp[4].x, 0.0f, temp[4].z };

			for (const auto& pc : corner)
			{
				if (!objectBoundingBox.Contains(XMLoadFloat3(&pc))) continue;

				// 플레이어의 점이 오브젝트 바운딩박스의 4개의 모서리 중 어느 모서리에 가장 가까운지 계산
				float dis[4]{};
				dis[0] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&oCorner[0]), XMLoadFloat3(&oCorner[1]), XMLoadFloat3(&pc)));
				dis[1] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&oCorner[1]), XMLoadFloat3(&oCorner[2]), XMLoadFloat3(&pc)));
				dis[2] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&oCorner[2]), XMLoadFloat3(&oCorner[3]), XMLoadFloat3(&pc)));
				dis[3] = XMVectorGetX(XMVector3LinePointDistance(XMLoadFloat3(&oCorner[3]), XMLoadFloat3(&oCorner[0]), XMLoadFloat3(&pc)));

				float* minDis{ ranges::min_element(dis) };
				XMFLOAT3 v{};
				if (*minDis == dis[0])
				{
					v = Vector3::Normalize(Vector3::Sub(oCorner[1], oCorner[2]));
				}
				else if (*minDis == dis[1])
				{
					v = Vector3::Normalize(Vector3::Sub(oCorner[1], oCorner[0]));
				}
				else if (*minDis == dis[2])
				{
					v = Vector3::Normalize(Vector3::Sub(oCorner[2], oCorner[1]));
				}
				else if (*minDis == dis[3])
				{
					v = Vector3::Normalize(Vector3::Sub(oCorner[0], oCorner[1]));
				}
				m_player->Move(Vector3::Mul(v, *minDis));

				// 움직였으면 다음 판정을 위해 플레이어의 바운딩박스를 최신화 해야함
				playerBoundingBox = m_player->GetHitboxes().front()->GetBoundingBox();
				playerBoundingBox.GetCorners(temp);
				corner[0] = XMFLOAT3{ temp[0].x, 0.0f, temp[0].z };
				corner[1] = XMFLOAT3{ temp[1].x, 0.0f, temp[1].z };
				corner[2] = XMFLOAT3{ temp[5].x, 0.0f, temp[5].z };
				corner[3] = XMFLOAT3{ temp[4].x, 0.0f, temp[4].z };
			}
		}
}

void GameScene::UpdateShadowMatrix()
{
	// 케스케이드 범위를 나눔
	constexpr array<float, Setting::SHADOWMAP_COUNT> casecade{ 0.0f, 0.02f, 0.1f, 0.4f };

	// NDC좌표계에서의 한 변의 길이가 1인 정육면체의 꼭짓점 8개
	XMFLOAT3 frustum[]
	{
		// 앞쪽
		{ -1.0f,  1.0f, 0.0f },	// 왼쪽위
		{  1.0f,  1.0f, 0.0f },	// 오른쪽위
		{  1.0f, -1.0f, 0.0f },	// 오른쪽아래
		{ -1.0f, -1.0f, 0.0f },	// 왼쪽아래

		// 뒤쪽
		{ -1.0f,  1.0f, 1.0f },	// 왼쪽위
		{  1.0f,  1.0f, 1.0f },	// 오른쪽위
		{  1.0f, -1.0f, 1.0f },	// 오른쪽아래
		{ -1.0f, -1.0f, 1.0f }	// 왼쪽아래
	};

	// NDC좌표계 -> 월드좌표계 변환 행렬
	XMFLOAT4X4 toWorldMatrix{ Matrix::Inverse(Matrix::Mul(m_camera->GetViewMatrix(), m_camera->GetProjMatrix())) };

	// NDC좌표계의 큐브를 월드좌표계로 변경(시야절두체)
	for (auto& v : frustum)
		v = Vector3::TransformCoord(v, toWorldMatrix);

	// 큐브의 정점을 시야절두체 구간으로 변경
	for (int i = 0; i < casecade.size() - 1; ++i)
	{
		XMFLOAT3 wFrustum[8]{};
		for (int j = 0; j < 8; ++j)
			wFrustum[j] = frustum[j];

		for (int j = 0; j < 4; ++j)
		{
			// 앞쪽에서 뒤쪽으로 향하는 벡터
			XMFLOAT3 v{ Vector3::Sub(wFrustum[j + 4], wFrustum[j]) };

			// 구간 시작, 끝으로 만들어주는 벡터
			XMFLOAT3 n{ Vector3::Mul(v, casecade[i]) };
			XMFLOAT3 f{ Vector3::Mul(v, casecade[i + 1]) };

			// 구간 시작, 끝으로 설정
			wFrustum[j + 4] = Vector3::Add(wFrustum[j], f);
			wFrustum[j] = Vector3::Add(wFrustum[j], n);
		}

		// 해당 구간을 포함할 바운딩구의 중심을 계산
		XMFLOAT3 center{};
		for (const auto& v : wFrustum)
			center = Vector3::Add(center, v);
		center = Vector3::Mul(center, 1.0f / 8.0f);

		// 바운딩구의 반지름을 계산
		float radius{};
		for (const auto& v : wFrustum)
			radius = max(radius, Vector3::Length(Vector3::Sub(v, center)));

		// 그림자를 만들 조명의 좌표를 바운딩구의 중심에서 빛의 반대방향으로 적당히 움직이여야함
		// 이건 씬의 오브젝트를 고려해서 정말 적당한 수치만큼 움직여줘야함
		float value{ max(500.0f, radius) };
		XMFLOAT3 shadowLightPos{ Vector3::Add(center, Vector3::Mul(m_cbGameSceneData->shadowLight.direction, -value)) };

		XMFLOAT4X4 lightViewMatrix, lightProjMatrix;
		XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
		XMStoreFloat4x4(&lightViewMatrix, XMMatrixLookAtLH(XMLoadFloat3(&shadowLightPos), XMLoadFloat3(&center), XMLoadFloat3(&up)));
		XMStoreFloat4x4(&lightProjMatrix, XMMatrixOrthographicLH(radius * 2.0f, radius * 2.0f, 0.0f, 5000.0f));
		m_cbGameSceneData->shadowLight.lightViewMatrix[i] = Matrix::Transpose(lightViewMatrix);
		m_cbGameSceneData->shadowLight.lightProjMatrix[i] = Matrix::Transpose(lightProjMatrix);
	}
}

void GameScene::RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	auto shadowTexture{ reinterpret_cast<ShadowTexture*>(s_textures["SHADOW"].get()) };

	// 뷰포트, 가위사각형 설정
	commandList->RSSetViewports(1, &shadowTexture->GetViewport());
	commandList->RSSetScissorRects(1, &shadowTexture->GetScissorRect());

	// 셰이더에 묶기
	shadowTexture->UpdateShaderVariable(commandList);

	// 리소스배리어 설정(깊이버퍼쓰기)
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadowTexture->GetBuffer().Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// 깊이스텐실 버퍼 초기화
	commandList->ClearDepthStencilView(shadowTexture->GetDsvCpuHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	// 렌더타겟 설정
	commandList->OMSetRenderTargets(0, NULL, FALSE, &shadowTexture->GetDsvCpuHandle());

	// 렌더링
	unique_lock<mutex> lock{ g_mutex };
	for (const auto& object : m_gameObjects)
	{
		if (auto shadowShader{ object->GetShadowShader() })
			object->Render(commandList, shadowShader);
	}
	for (const auto& monster : s_monsters)
	{
		auto shadowShader{ monster->GetShadowShader() };
		if (shadowShader)
			monster->Render(commandList, shadowShader);
	}
	lock.unlock();
	for (const auto& player : m_multiPlayers)
	{
		if (!player) continue;
		player->RenderToShadowMap(commandList);
	}
	if (m_player)
	{
		// 죽었을 경우 이미 메쉬가 PLAYER로 되어있음
		if (m_player->GetHp() <= 0)
			m_player->RenderToShadowMap(commandList);
		else
		{
			m_player->SetMesh(s_meshes["PLAYER"]);
			m_player->RenderToShadowMap(commandList);
			m_player->SetMesh(s_meshes["ARM"]);
		}
	}

	// 리소스배리어 설정(셰이더에서 읽기)
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadowTexture->GetBuffer().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void GameScene::RenderOutline(const ComPtr<ID3D12GraphicsCommandList>& commandList, const unique_ptr<OutlineObject>& outliner) const
{
	s_textures["DEPTH"]->Copy(commandList, g_gameFramework.GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
	s_textures["DEPTH"]->UpdateShaderVariable(commandList);
	s_textures["STENCIL"]->Copy(commandList, g_gameFramework.GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
	s_textures["STENCIL"]->UpdateShaderVariable(commandList);
	outliner->Render(commandList);
}

void GameScene::RecvPacket()
{
	// size, type
	char buf[2];
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD recvByte{ 0 }, recvFlag{ 0 };
	int error_code = WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);
	if (error_code == SOCKET_ERROR) error_display("RecvSizeType");

	UCHAR size{ static_cast<UCHAR>(buf[0]) };
	UCHAR type{ static_cast<UCHAR>(buf[1]) };
	switch (type)
	{
	case SC_PACKET_UPDATE_CLIENT:
		RecvUpdateClient();
		break;
	case SC_PACKET_UPDATE_MONSTER:
		RecvUpdateMonster();
		break;
	case SC_PACKET_BULLET_FIRE:
		RecvBulletFire();
		break;
	case SC_PACKET_BULLET_HIT:
		RecvBulletHit();
		break;
	case SC_PACKET_MONSTER_ATTACK:
		RecvMosterAttack();
		break;
	case SC_PACKET_ROUND_RESULT:
		RecvRoundResult();
		break;
	case SC_PACKET_LOGOUT_OK:
		RecvLogoutOkPacket();
		break;
	default:
	{
		string debug{};
		debug += "GAMESCENE::RECV_ERR | size : " + to_string(static_cast<int>(size)) + ", type : " + to_string(static_cast<int>(type)) + "\n";
		OutputDebugStringA(debug.c_str());
		break;
	}
	}
}

void GameScene::RecvUpdateClient()
{
	char subBuf[sizeof(PlayerData) * MAX_USER]{};
	WSABUF wsabuf{ sizeof(subBuf), subBuf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	// 모든 플레이어의 데이터
	array<PlayerData, MAX_USER> data;
	memcpy(&data, subBuf, sizeof(PlayerData) * MAX_USER);

	// 멀티플레이어 업데이트
	unique_lock<mutex> lock{ g_mutex };
	for (auto& p : m_multiPlayers)
	{
		if (!p) continue;
		for (auto& d : data)
		{
			if (!d.isActive) continue;
			if (p->GetId() != d.id) continue;
			p->ApplyServerData(d);
		}
	}
}

void GameScene::RecvUpdateMonster()
{
	// 모든 몬스터의 정보를 수신함
	char subBuf[sizeof(MonsterData) * MAX_MONSTER]{};
	WSABUF wsabuf{ sizeof(subBuf), subBuf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	array<MonsterData, MAX_MONSTER> monsters{};
	memcpy(monsters.data(), subBuf, sizeof(MonsterData) * MAX_MONSTER);

	for (MonsterData& m : monsters)
	{
		// id가 0보다 작으면 유효하지 않음
		if (m.id < 0) continue;

		// 해당 id의 몬스터가 없는 경우엔 생성
		auto it{ ranges::find_if(s_monsters, [&](const unique_ptr<Monster>& monster) { return monster->GetId() == static_cast<int>(m.id); }) };
		if (it == s_monsters.end())
		{
			unique_ptr<Monster> mob{};
			if (m.type == eMobType::ULIFO)
			{
				mob = make_unique<BossMonster>(m.id);
				auto camera{ reinterpret_cast<ShowCamera*>(m_showCamera.get()) };
				camera->SetTarget(mob.get());
				camera->SetTime(4.0f);
				camera->SetTimerCallback(
					[&]()
					{
						m_camera.swap(m_showCamera);
						m_skybox->SetCamera(m_camera);
						m_isShowing = FALSE;
					});
				m_camera.swap(m_showCamera);
				m_skybox->SetCamera(m_camera);
				m_isShowing = TRUE;
			}
			else
			{
				mob = make_unique<Monster>(m.id, m.type);
			}
			mob->ApplyServerData(m);

			unique_lock<mutex> lock{ g_mutex };
			s_monsters.push_back(move(mob));
			continue;
		}

		// 있으면 데이터 최신화
		(*it)->ApplyServerData(m);
	}
}

void GameScene::RecvBulletFire()
{
	char subBuf[sizeof(BulletData)]{};
	WSABUF wsabuf{ sizeof(subBuf), subBuf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	BulletData bulletData{};
	memcpy(&bulletData, &subBuf, sizeof(BulletData));

	auto bullet{ make_unique<Bullet>(bulletData.dir) };
	if (bulletData.playerId == m_player->GetId())
		bullet->SetMesh(s_meshes["BULLET"]);
	else
		bullet->SetMesh(s_meshes["BULLET2"]);
	bullet->SetShader(s_shaders["DEFAULT"]);
	bullet->SetPosition(bulletData.pos);

	unique_lock<mutex> lock{ g_mutex };
	m_gameObjects.push_back(move(bullet));
}

void GameScene::RecvBulletHit()
{
	char buf[sizeof(BulletHitData)]{};
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	BulletHitData data{};
	memcpy(&data, buf, sizeof(data));

	auto textureInfo{ make_unique<TextureInfo>() };
	textureInfo->loop = FALSE;
	textureInfo->interver = 1.0f / 30.0f;

	auto hitEffect{ make_unique<UIObject>(50.0f, 50.0f) };
	hitEffect->SetTexture(s_textures["HIT"]);
	hitEffect->SetTextureInfo(move(textureInfo));

	unique_lock<mutex> lock{ g_mutex };
	m_uiObjects.push_back(move(hitEffect));
	lock.unlock();

	auto it{ ranges::find_if(s_monsters, [&](const unique_ptr<Monster>& monster) { return monster->GetId() == static_cast<int>(data.mobId); }) };
	if (it != s_monsters.end())
	{
		wstring dmg{ to_wstring(m_player->GetDamage()) };
		auto dmgText{ make_unique<DamageTextObject>(dmg.c_str()) };

		XMFLOAT3 offset{};
		switch ((*it)->GetType())
		{
		case eMobType::GAROO:
			offset = XMFLOAT3{ 0.0f, 20.0f, 0.0f };
			break;
		case eMobType::SERPENT:
			offset = XMFLOAT3{ 0.0f, 45.0f, 0.0f };
			break;
		case eMobType::HORROR:
			offset = XMFLOAT3{ 0.0f, 38.0f, 0.0f };
			break;
		case eMobType::ULIFO:
			offset = XMFLOAT3{ 0.0f, 145.0f, 0.0f };
			break;
		}
		// 랜덤하게 좀 더 움직임
		offset = Vector3::Add(offset, XMFLOAT3{ Utile::Random(-4.0f, 4.0f), Utile::Random(-2.0f, 2.0f), 0.0f });
		dmgText->SetStartPosition(Vector3::Add((*it)->GetPosition(), offset));
		lock.lock();
		m_textObjects.push_back(move(dmgText));
	}

	// 총알 맞춘게 나라면 스킬 게이지 증가
	if (data.bullet.playerId == m_player->GetId() && !m_player->GetIsSkillActive())
	{
		switch (m_player->GetWeaponType())
		{
		case eWeaponType::AR:
			m_player->SetSkillGage(m_player->GetSkillGage() + 2.0f);
			break;
		case eWeaponType::SG:
			m_player->SetSkillGage(m_player->GetSkillGage() + 1.0f);
			break;
		case eWeaponType::MG:
			m_player->SetSkillGage(m_player->GetSkillGage() + 0.2f);
			break;
		}
	}
}

void GameScene::RecvMosterAttack()
{
	//char buf[sizeof(MonsterAttackData)]{};
	//WSABUF wsabuf{ sizeof(buf), buf };
	//DWORD recvByte{}, recvFlag{};
	//WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	//MonsterAttackData data{};
	//memcpy(&data, buf, sizeof(data));
	//if (m_player->GetHp() == 0) return;
	//if (m_player->GetId() != data.id) return;

	//// 체력 감소
	//int hp{ m_player->GetHp() };
	//m_player->SetHp(hp - static_cast<int>(data.damage));

	//// 애니메이션
	//if (hp > 0 && m_player->GetHp() <= 0)
	//	OnPlayerDie();
	//else
	//	m_player->PlayAnimation("HIT");

	//// 피격 이펙트
	//auto hit{ make_unique<HitUIObject>(data.mobId) };
	//unique_lock<mutex> lock{ g_mutex };
	//m_uiObjects.push_back(move(hit));
}

void GameScene::RecvRoundResult()
{
	char buf{};
	WSABUF wsabuf{ sizeof(buf), &buf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	eRoundResult roundResult{ static_cast<eRoundResult>(buf) };
	switch (roundResult)
	{
	case eRoundResult::CLEAR:
		RecvRoundClear();
		break;
	case eRoundResult::OVER:
	{
		auto window{ make_unique<WindowObject>(400.0f, 300.0f) };
		window->SetTexture(s_textures["WHITE"]);

		auto title{ make_unique<TextObject>() };
		title->SetBrush("BLACK");
		title->SetFormat("36L");
		title->SetText(TEXT("알림"));
		title->SetPivot(ePivot::LEFTCENTER);
		title->SetScreenPivot(ePivot::LEFTTOP);
		title->SetPosition(XMFLOAT2{ 0.0f, title->GetHeight() / 2.0f + 2.0f });
		window->Add(title);

		auto descText{ make_unique<TextObject>() };
		descText->SetBrush("BLACK");
		descText->SetFormat("32L");
		descText->SetPivot(ePivot::CENTER);
		descText->SetScreenPivot(ePivot::CENTER);
		descText->SetText(TEXT("클리어에 실패했습니다..."));
		descText->SetPosition(XMFLOAT2{});
		window->Add(descText);

		auto goToMainText{ make_unique<MenuTextObject>() };
		goToMainText->SetBrush("BLACK");
		goToMainText->SetMouseOverBrush("BLUE");
		goToMainText->SetFormat("32L");
		goToMainText->SetText(TEXT("메인으로"));
		goToMainText->SetPivot(ePivot::CENTERBOT);
		goToMainText->SetScreenPivot(ePivot::CENTERBOT);
		goToMainText->SetPosition(XMFLOAT2{});
		goToMainText->SetMouseClickCallBack(
			[]()
			{
				g_gameFramework.SetNextScene(eSceneType::MAIN);
				ShowCursor(TRUE);
			});
		window->Add(goToMainText);

		unique_lock<mutex> lock{ g_mutex };
		m_windowObjects.push_back(move(window));
		break;
	}
	case eRoundResult::ENDING:
	{
		auto window{ make_unique<WindowObject>(400.0f, 300.0f) };
		window->SetTexture(s_textures["WHITE"]);

		auto title{ make_unique<TextObject>() };
		title->SetBrush("BLACK");
		title->SetFormat("36L");
		title->SetText(TEXT("알림"));
		title->SetPivot(ePivot::LEFTCENTER);
		title->SetScreenPivot(ePivot::LEFTTOP);
		title->SetPosition(XMFLOAT2{ 0.0f, title->GetHeight() / 2.0f + 2.0f });
		window->Add(title);

		auto descText{ make_unique<TextObject>() };
		descText->SetBrush("BLACK");
		descText->SetFormat("32L");
		descText->SetPivot(ePivot::CENTER);
		descText->SetScreenPivot(ePivot::CENTER);
		descText->SetText(TEXT("모든 라운드를\r\n클리어했습니다!"));
		descText->SetPosition(XMFLOAT2{});
		window->Add(descText);

		auto goToMainText{ make_unique<MenuTextObject>() };
		goToMainText->SetBrush("BLACK");
		goToMainText->SetMouseOverBrush("BLUE");
		goToMainText->SetFormat("32L");
		goToMainText->SetText(TEXT("메인으로"));
		goToMainText->SetPivot(ePivot::CENTERBOT);
		goToMainText->SetScreenPivot(ePivot::CENTERBOT);
		goToMainText->SetPosition(XMFLOAT2{});
		goToMainText->SetMouseClickCallBack(
			[]()
			{
				g_gameFramework.SetNextScene(eSceneType::MAIN);
				ShowCursor(TRUE);
			});
		window->Add(goToMainText);

		unique_lock<mutex> lock{ g_mutex };
		m_windowObjects.push_back(move(window));
		break;
	}
	}
}

void GameScene::RecvLogoutOkPacket()
{
	char buf{};
	WSABUF wsabuf{ sizeof(buf), &buf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	int id{ static_cast<int>(buf) };
	if (m_player->GetId() == id)
	{
		g_isConnected = FALSE;
		return;
	}
	for (auto& p : m_multiPlayers)
	{
		if (!p) continue;
		if (p->GetId() == id)
		{
			p.reset();
			return;
		}
	}
}

void GameScene::RecvRoundClear()
{
	// 죽어있다면 부활
	if (m_player->GetHp() <= 0)
		OnPlayerRevive();

	// 멀티플레이어도 죽어있다면 IDLE 애니메이션 재생
	for (auto& player : m_multiPlayers)
	{
		if (!player) continue;
		player->PlayAnimation("IDLE");
	}

	// 중복없이 보상 3개 선택
	const auto r = { eReward::DAMAGE, eReward::SPEED, eReward::MAXHP, eReward::MAXBULLET, eReward::SPECIAL };
	vector<eReward> rewards;
	ranges::sample(r, back_inserter(rewards), 3, g_randomEngine);

	// 보상 칸 하나 당 가로 길이 비율
	constexpr float REWARE_WIDTH_RATIO{ 0.18f };

	unique_ptr<RewardUIObject> rewardUIObjects[3]{};
	unique_ptr<UIObject> rewardImageObjects[3]{};
	unique_ptr<TextObject> rewardTextObjects[3]{};
	for (int i = 0; i < 3; ++i)
	{
		// 보상 UI
		rewardUIObjects[i] = make_unique<RewardUIObject>(g_width * REWARE_WIDTH_RATIO, g_height * 0.5f);
		rewardUIObjects[i]->SetTexture(s_textures["OUTLINE"]);
		rewardUIObjects[i]->SetMouseClickCallBack(bind(
			[&](const shared_ptr<Player>& player, eReward reward)
			{
				eWeaponType weaponType{ player->GetWeaponType() };
				switch (reward)
				{
				case eReward::DAMAGE:
					switch (weaponType)
					{
					case eWeaponType::AR:
					case eWeaponType::SG:
						player->AddBonusDamage(5);
						break;
					case eWeaponType::MG:
						player->AddBonusDamage(3);
						break;
					}
					break;
				case eReward::SPEED:
					player->AddBonusSpeed(10);
					break;
				case eReward::MAXHP:
					player->AddMaxHp(25);
					break;
				case eReward::MAXBULLET:
					switch (weaponType)
					{
					case eWeaponType::AR:
						player->AddMaxBulletCount(10);
						break;
					case eWeaponType::SG:
						player->AddMaxBulletCount(3);
						break;
					case eWeaponType::MG:
						player->AddMaxBulletCount(20);
						break;
					}
					break;
				case eReward::SPECIAL:
					switch (weaponType)
					{
					case eWeaponType::AR:
						player->AddBonusReloadSpeed(20);
						break;
					case eWeaponType::SG:
						player->AddBonusBulletFire(2);
						break;
					case eWeaponType::MG:
						player->AddBonusAttackSpeed(20);
						break;
					}
					break;
				}
#ifdef NETWORK
				cs_packet_select_reward packet{};
				packet.type = CS_PACKET_SELECT_REWARD;
				packet.size = sizeof(packet);
				packet.playerId = m_player->GetId();
				send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), NULL);
#endif
				m_windowObjects.back()->Delete();
			}, m_player, rewards[i]));

		// 보상 아이콘
		rewardImageObjects[i] = make_unique<UIObject>(g_width * (REWARE_WIDTH_RATIO - 0.08f), g_width * (REWARE_WIDTH_RATIO - 0.08f));
		rewardImageObjects[i]->SetPivot(ePivot::CENTERTOP);
		rewardImageObjects[i]->SetScreenPivot(ePivot::CENTERTOP);

		// 보상 설명 텍스트
		rewardTextObjects[i] = make_unique<TextObject>();
		rewardTextObjects[i]->SetFormat("28L");
		rewardTextObjects[i]->SetBrush("WHITE");

		// 보상 별 설정
		switch (rewards[i])
		{
		case eReward::DAMAGE:
			rewardImageObjects[i]->SetTexture(s_textures["REWARD_DAMAGE"]);
			switch (m_player->GetWeaponType())
			{
			case eWeaponType::AR:
			case eWeaponType::SG:
				rewardTextObjects[i]->SetText(TEXT("공격력 +5"));
				break;
			case eWeaponType::MG:
				rewardTextObjects[i]->SetText(TEXT("공격력 +3"));
				break;
			}
			break;
		case eReward::SPEED:
			rewardImageObjects[i]->SetTexture(s_textures["REWARD_SPEED"]);
			rewardTextObjects[i]->SetText(TEXT("이동 속도 +10%"));
			break;
		case eReward::MAXHP:
			rewardImageObjects[i]->SetTexture(s_textures["REWARD_HP"]);
			rewardTextObjects[i]->SetText(TEXT("최대 체력 +25"));
			break;
		case eReward::MAXBULLET:
			rewardImageObjects[i]->SetTexture(s_textures["REWARD_BULLET"]);
			switch (m_player->GetWeaponType())
			{
			case eWeaponType::AR:
				rewardTextObjects[i]->SetText(TEXT("탄창 +10"));
				break;
			case eWeaponType::SG:
				rewardTextObjects[i]->SetText(TEXT("탄창 +3"));
				break;
			case eWeaponType::MG:
				rewardTextObjects[i]->SetText(TEXT("탄창 +20"));
				break;
			}
			break;
		case eReward::SPECIAL:
			switch (m_player->GetWeaponType())
			{
			case eWeaponType::AR:
				rewardImageObjects[i]->SetTexture(s_textures["REWARD_AR"]);
				rewardTextObjects[i]->SetText(TEXT("재장전 속도 +20%"));
				break;
			case eWeaponType::SG:
				rewardImageObjects[i]->SetTexture(s_textures["REWARD_SG"]);
				rewardTextObjects[i]->SetText(TEXT("발사 탄환 수 +2"));
				break;
			case eWeaponType::MG:
				rewardImageObjects[i]->SetTexture(s_textures["REWARD_MG"]);
				rewardTextObjects[i]->SetText(TEXT("공격 속도 +20%"));
				break;
			}
			break;
		}

		// 순서 별 설정
		switch (i)
		{
		case 0:
			rewardUIObjects[i]->SetPosition(XMFLOAT2{ g_width * -(REWARE_WIDTH_RATIO + 0.02f), 0.0f });
			rewardImageObjects[i]->SetPosition(XMFLOAT2{ g_width * -(REWARE_WIDTH_RATIO + 0.02f), g_width * -0.04f });
			rewardTextObjects[i]->SetPosition(XMFLOAT2{ g_width * -(REWARE_WIDTH_RATIO + 0.02f), g_width * -0.04f });
			break;
		case 1:
			rewardImageObjects[i]->SetPosition(XMFLOAT2{ 0.0f, g_width * -0.04f });
			rewardTextObjects[i]->SetPosition(XMFLOAT2{ 0.0f, g_width * -0.04f });
			break;
		case 2:
			rewardUIObjects[i]->SetPosition(XMFLOAT2{ g_width * (REWARE_WIDTH_RATIO + 0.02f), 0.0f });
			rewardImageObjects[i]->SetPosition(XMFLOAT2{ g_width * (REWARE_WIDTH_RATIO + 0.02f), g_width * -0.04f });
			rewardTextObjects[i]->SetPosition(XMFLOAT2{ g_width * (REWARE_WIDTH_RATIO + 0.02f), g_width * -0.04f });
			break;
		}
	}

	auto title{ make_unique<TextObject>() };
	title->SetBrush("BLACK");
	title->SetFormat("36L");
	title->SetText(TEXT("보상"));
	title->SetPivot(ePivot::LEFTBOT);
	title->SetScreenPivot(ePivot::LEFTTOP);
	title->SetPosition(XMFLOAT2{ 0.0f, 2.0f });

	auto window{ make_unique<WindowObject>(g_width * 0.6f, g_height * 0.54f) };
	window->SetTexture(s_textures["OUTLINE"]);
	window->Add(title);
	for (int i = 0; i < 3; ++i)
	{
		window->Add(rewardUIObjects[i]);
		window->Add(rewardImageObjects[i]);
		window->Add(rewardTextObjects[i]);
	}
	m_windowObjects.push_back(move(window));
}

void GameScene::SetPlayer(unique_ptr<Player>& player)
{
	m_player = move(player);
}

void GameScene::SetMultiPlayers(array<shared_ptr<Player>, Setting::MAX_PLAYERS>& multiPlayers)
{
	m_multiPlayers = move(multiPlayers);
}