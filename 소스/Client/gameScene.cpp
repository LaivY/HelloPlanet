#include "gameScene.h"
#include "framework.h"

GameScene::GameScene() : m_pcbGameScene{ nullptr }
{
	ShowCursor(FALSE);
}

GameScene::~GameScene()
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

	// 그림자맵
	m_shadowMap = make_unique<ShadowMap>(device, 1 << 12, 1 << 12, Setting::SHADOWMAP_COUNT);

	// 외곽선
	m_stencilTexture = make_unique<Texture>();
	m_stencilTexture->CreateTexture(device, DXGI_FORMAT_R24G8_TYPELESS, DXGI_FORMAT_X24_TYPELESS_G8_UINT, g_width, g_height, 7);
	m_fullScreenQuad = make_unique<GameObject>();
	m_fullScreenQuad->SetMesh(s_meshes["FULLSCREEN"]);
	m_fullScreenQuad->SetShader(s_shaders["FULLSCREEN"]);

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
	float sensitive{ 2.5f };
	int dx = newMousePosition.x - oldMousePosition.x;
	int dy = newMousePosition.y - oldMousePosition.y;
	if (dx != 0 || dy != 0)
	{
#ifdef FREEVIEW
		if (m_camera) m_camera->Rotate(0.0f, dy * sensitive * deltaTime, dx * sensitive * deltaTime);
#endif
#ifdef FIRSTVIEW
		if (m_player) m_player->Rotate(0.0f, dy * sensitive * deltaTime, dx * sensitive * deltaTime);
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
		case VK_F1:
			if (m_player)
				m_player->SetHp(m_player->GetHp() - 10);
			break;
		case VK_F2:
			if (m_player)
				m_player->SetHp(m_player->GetHp() + 5);
			break;
		case VK_ESCAPE:
			CreateExitWindow();
			//PostMessage(hWnd, WM_QUIT, 0, 0);
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
	for (auto& p : m_multiPlayers)
		if (p) p->Update(deltaTime);
	for (auto& o : m_gameObjects)
		o->Update(deltaTime);
	for (auto& [_, m] : m_monsters)
		m->Update(deltaTime);
	for (auto& ui : m_uiObjects)
		ui->Update(deltaTime);
	for (auto& t : m_textObjects)
		t->Update(deltaTime);
	for (auto& w : m_windowObjects)
		w->Update(deltaTime);
}


void GameScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	memcpy(m_pcbGameScene, m_cbGameSceneData.get(), sizeof(cbGameScene));
	commandList->SetGraphicsRootConstantBufferView(3, m_cbGameScene->GetGPUVirtualAddress());
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
}

void GameScene::PreRender(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	RenderToShadowMap(commandList);
}

void GameScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	// 뷰포트, 가위사각형, 렌더타겟 설정
	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);
	commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

	// 스카이박스 렌더링
	if (m_skybox) m_skybox->Render(commandList);

	// 외곽선이 있는 게임오브젝트들 렌더링
	RenderOutlineObjects(commandList);

	// 멀티플레이어 렌더링
	for (const auto& p : m_multiPlayers)
		if (p) p->Render(commandList);

	// 몬스터 렌더링
	unique_lock<mutex> lock{ g_mutex };
	for (const auto& [_, m] : m_monsters)
		m->Render(commandList);
	lock.unlock();

	// 플레이어 렌더링
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
	if (m_player) m_player->Render(commandList);

	// UI 렌더링
	if (m_uiCamera)
	{
		unique_lock<mutex> lock{ g_mutex };
		m_uiCamera->UpdateShaderVariable(commandList);
		for (const auto& ui : m_uiObjects)
			ui->Render(commandList);
		for (const auto& w : m_windowObjects)
			w->Render(commandList);
		lock.unlock();
	}
}

void GameScene::Render2D(const ComPtr<ID2D1DeviceContext2>& device)
{
	unique_lock<mutex> lock{ g_mutex };
	for (const auto& t : m_textObjects)
		t->Render(device);
	for (const auto& w : m_windowObjects)
		w->Render2D(device);
}

void GameScene::PostProcessing(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& postRootSignature, const ComPtr<ID3D12Resource>& renderTarget)
{

}

void GameScene::ProcessClient()
{
	while (g_isConnected)
		RecvPacket();
}

void GameScene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_cbGameScene = Utile::CreateBufferResource(device, commandList, NULL, Utile::GetConstantBufferSize<cbGameScene>(), 1, D3D12_HEAP_TYPE_UPLOAD, {});
	m_cbGameScene->Map(0, NULL, reinterpret_cast<void**>(&m_pcbGameScene));
	m_cbGameSceneData = make_unique<cbGameScene>();
}

void GameScene::CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// UI 카메라 생성
	XMFLOAT4X4 projMatrix{};
	m_uiCamera = make_unique<Camera>();
	m_uiCamera->CreateShaderVariable(device, commandList);
	XMStoreFloat4x4(&projMatrix, XMMatrixOrthographicLH(static_cast<float>(g_width), static_cast<float>(g_height), 0.0f, 1.0f));
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
}

void GameScene::CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 카메라 생성
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
	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(g_width) / static_cast<float>(g_height), 1.0f, 2500.0f));
	m_camera->SetProjMatrix(projMatrix);

	// 플레이어, 멀티플레이어 설정
	// 플레이어와 멀티플레이어는 이전 로비 씬에서 만들어져있다.
	m_player->PlayAnimation("IDLE");
	m_player->DeleteUpperAnimation();
	for (auto& p : m_multiPlayers)
	{
		if (!p) continue;
		p->PlayAnimation("IDLE");
		p->DeleteUpperAnimation();
	}

	// 카메라, 플레이어 설정
	m_player->SetCamera(m_camera);
	m_camera->SetPlayer(m_player);

	// 스카이박스
	m_skybox = make_unique<Skybox>();
	m_skybox->SetCamera(m_camera);

	// 바닥
	auto floor{ make_unique<GameObject>() };
	floor->SetMesh(s_meshes["FLOOR"]);
	floor->SetShader(s_shaders["DEFAULT"]);
	m_gameObjects.push_back(move(floor));
}

void GameScene::CreateTextObjects(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	// 텍스트 오브젝트들의 좌표계는 좌측 상단이 (0, 0), 우측 하단이 (width, height) 이다.
	// 총알
	auto bulletText{ make_unique<BulletTextObject>() };
	bulletText->SetPlayer(m_player);
	bulletText->SetScreenPivot(ePivot::RIGHTBOT);
	bulletText->SetPosition(XMFLOAT2{ -160.0f, -80.0f });
	m_textObjects.push_back(move(bulletText));

	// 체력
	auto hpText{ make_unique<HPTextObject>() };
	hpText->SetPlayer(m_player);
	hpText->SetScreenPivot(ePivot::LEFTBOT);
	hpText->SetPosition(XMFLOAT2{ 50.0f, -125.0f });
	m_textObjects.push_back(move(hpText));
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
			auto hitbox{ make_unique<Hitbox>(trans, Vector3::Mul(scale, 50.0f), XMFLOAT3{rotat.z, rotat.x, rotat.y}) };
			hitbox->SetOwner(dumy.get());
			dumy->AddHitbox(move(hitbox));
		}
	}
	m_gameObjects.push_back(move(dumy));
}

void GameScene::CreateExitWindow()
{
	auto text{ make_unique<TextObject>() };
	text->SetBrush("BLACK");
	text->SetFormat("36_LEFT");
	text->SetText(TEXT("메인 화면으로\n돌아갈까요?"));
	text->SetPivot(ePivot::CENTER);
	text->SetScreenPivot(ePivot::CENTER);
	text->SetPosition(XMFLOAT2{});

	auto okText{ make_unique<MenuTextObject>() };
	okText->SetBrush("BLACK");
	okText->SetMouseOverBrush("BLUE");
	okText->SetFormat("48_RIGHT");
	okText->SetText(TEXT("확인"));
	okText->SetPivot(ePivot::CENTERBOT);
	okText->SetScreenPivot(ePivot::CENTERBOT);
	okText->SetPosition(XMFLOAT2{ -65.0f, -25.0f });
	okText->SetMouseClickCallBack(
		[]()
		{
			ShowCursor(TRUE);
			g_gameFramework.SetNextScene(eSceneType::MAIN);
		}
	);

	auto cancleText{ make_unique<MenuTextObject>() };
	cancleText->SetBrush("BLACK");
	cancleText->SetMouseOverBrush("BLUE");
	cancleText->SetFormat("48_RIGHT");
	cancleText->SetText(TEXT("취소"));
	cancleText->SetPivot(ePivot::CENTERBOT);
	cancleText->SetScreenPivot(ePivot::CENTERBOT);
	cancleText->SetPosition(XMFLOAT2{ 65.0f, -25.0f });
	cancleText->SetMouseClickCallBack(bind(&GameScene::CloseWindow, this));

	auto window{ make_unique<WindowObject>(400.0f, 300.0f) };
	window->SetMesh(s_meshes["UI"]);
	window->SetShader(s_shaders["UI"]);
	window->SetTexture(s_textures["WHITE"]);
	window->Add(text);
	window->Add(okText);
	window->Add(cancleText);
	m_windowObjects.push_back(move(window));
}

void GameScene::CloseWindow()
{
	if (!m_windowObjects.empty())
		m_windowObjects.pop_back();
}

void GameScene::Update(FLOAT deltaTime)
{
	unique_lock<mutex> lock{ g_mutex };
	erase_if(m_gameObjects, [](unique_ptr<GameObject>& object) { return object->isDeleted(); });
	erase_if(m_uiObjects, [](unique_ptr<UIObject>& object) { return object->isDeleted(); });
	erase_if(m_textObjects, [](unique_ptr<TextObject>& object) { return object->isDeleted(); });
	erase_if(m_monsters, [](const auto& item) { return item.second->isDeleted(); });
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

				float* minDis{ min_element(begin(dis), end(dis)) };
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
	XMFLOAT3 frustum[]{
		// 앞쪽
		{ -1.0f, 1.0f, 0.0f },	// 왼쪽위
		{ 1.0f, 1.0f, 0.0f },	// 오른쪽위
		{ 1.0f, -1.0f, 0.0f },	// 오른쪽아래
		{ -1.0f, -1.0f, 0.0f },	// 왼쪽아래

		// 뒤쪽
		{ -1.0f, 1.0f, 1.0f },	// 왼쪽위
		{ 1.0f, 1.0f, 1.0f },	// 오른쪽위
		{ 1.0f, -1.0f, 1.0f },	// 오른쪽아래
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
	if (!m_shadowMap) return;

	// 뷰포트, 가위사각형 설정
	commandList->RSSetViewports(1, &m_shadowMap->GetViewport());
	commandList->RSSetScissorRects(1, &m_shadowMap->GetScissorRect());

	// 셰이더에 묶기
	ID3D12DescriptorHeap* ppHeaps[]{ m_shadowMap->GetSrvHeap().Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandList->SetGraphicsRootDescriptorTable(6, m_shadowMap->GetGpuSrvHandle());

	// 리소스배리어 설정(깊이버퍼쓰기)
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMap->GetShadowMap().Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// 깊이스텐실 버퍼 초기화
	commandList->ClearDepthStencilView(m_shadowMap->GetCpuDsvHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	// 렌더타겟 설정
	commandList->OMSetRenderTargets(0, NULL, FALSE, &m_shadowMap->GetCpuDsvHandle());

	// 렌더링
	unique_lock<mutex> lock{ g_mutex };
	for (const auto& object : m_gameObjects)
	{
		if (auto shadowShader{ object->GetShadowShader() })
			object->Render(commandList, shadowShader);
	}
	for (const auto& [_, monster] : m_monsters)
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
		m_player->SetMesh(s_meshes["PLAYER"]);
		m_player->RenderToShadowMap(commandList);
		m_player->SetMesh(s_meshes["ARM"]);
	}

	// 리소스배리어 설정(셰이더에서 읽기)
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMap->GetShadowMap().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void GameScene::RenderOutlineObjects(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	// 외곽선을 그릴 게임오브젝트 렌더링
	UINT stencilRef{ 1 };
	unique_lock<mutex> lock{ g_mutex };
	for (const auto& o : m_gameObjects)
	{
		//if (!o->isMakeOutline()) continue;
		commandList->OMSetStencilRef(stencilRef++);
		o->Render(commandList);
	}
	lock.unlock();
	commandList->OMSetStencilRef(0);

	// 외곽선 렌더링
	m_stencilTexture->Copy(commandList, g_gameFramework.GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
	m_stencilTexture->UpdateShaderVariable(commandList);
	m_fullScreenQuad->Render(commandList);
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
		debug += "RECV_ERR | size : " + to_string(static_cast<int>(size)) + ", type : " + to_string(static_cast<int>(type)) + "\n";
		OutputDebugStringA(debug.c_str());
		break;
	}
	}
}

void GameScene::RecvLoginOk()
{
	char subBuf[sizeof(PlayerData)]{};
	WSABUF wsabuf{ sizeof(subBuf), subBuf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	char nameBuf[10]{};
	wsabuf = WSABUF{ sizeof(nameBuf), nameBuf };
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	PlayerData data{};
	memcpy(&data, subBuf, sizeof(data));

	// 로그인 패킷을 처음 수신했을 경우 자신의 id 설정
	if (m_player->GetId() == -1)
	{
		m_player->SetId(static_cast<int>(data.id));
		return;
	}

	// 다른 플레이어들 정보일 경우
	if (m_player->GetId() != data.id)
		for (auto& p : m_multiPlayers)
		{
			if (p) continue;
			p = make_unique<Player>(TRUE);
			p->SetId(static_cast<int>(data.id));
			p->SetWeaponType(eWeaponType::SG);
			p->PlayAnimation("IDLE");
			p->ApplyServerData(data);
			break;
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

	for (const MonsterData& m : monsters)
	{
		// id가 0보다 작으면 유효하지 않음
		if (m.id < 0) continue;

		// 해당 id의 몬스터가 없는 경우엔 생성
		if (m_monsters.find(m.id) == m_monsters.end())
		{
			unique_lock<mutex> lock{ g_mutex };
			m_monsters[m.id] = make_unique<Monster>();
			lock.unlock();
			m_monsters[m.id]->SetMesh(s_meshes["GAROO"]);
			m_monsters[m.id]->SetShader(s_shaders["ANIMATION"]);
			m_monsters[m.id]->SetShadowShader(s_shaders["SHADOW_ANIMATION"]);
			m_monsters[m.id]->SetTexture(s_textures["GAROO"]);
			m_monsters[m.id]->PlayAnimation("IDLE");
		}
		m_monsters[m.id]->ApplyServerData(m);
	}
}

void GameScene::RecvBulletFire()
{
	// pos, dir, playerId
	char subBuf[12 + 12 + 1]{};
	WSABUF wsabuf{ sizeof(subBuf), subBuf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	XMFLOAT3 pos{}, dir{}; char playerId{};
	memcpy(&pos, &subBuf[0], sizeof(pos));
	memcpy(&dir, &subBuf[12], sizeof(dir));
	memcpy(&playerId, &subBuf[24], sizeof(playerId));

	auto bullet{ make_unique<Bullet>(dir) };
	bullet->SetMesh(s_meshes["BULLET"]);
	bullet->SetShader(s_shaders["DEFAULT"]);
	bullet->SetPosition(pos);

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
	textureInfo->doRepeat = FALSE;
	textureInfo->interver = 1.0f / 30.0f;

	auto hitEffect{ make_unique<UIObject>(50.0f, 50.0f) };
	hitEffect->SetMesh(s_meshes["UI"]);
	hitEffect->SetShader(s_shaders["UI"]);
	hitEffect->SetTexture(s_textures["HIT"]);
	hitEffect->SetTextureInfo(move(textureInfo));

	unique_lock<mutex> lock{ g_mutex };
	m_uiObjects.push_back(move(hitEffect));

	if (m_monsters.find(static_cast<int>(data.mobId)) != m_monsters.end())
	{
		auto dmgText{ make_unique<DamageTextObject>(TEXT("40")) };
		dmgText->SetCamera(m_camera);
		dmgText->SetStartPosition(m_monsters[static_cast<int>(data.mobId)]->GetPosition());
		m_textObjects.push_back(move(dmgText));
	}
}

void GameScene::RecvMosterAttack()
{
	char buf[sizeof(MonsterAttackData)]{};
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	MonsterAttackData data{};
	memcpy(&data, buf, sizeof(data));

	if (m_player->GetId() == data.id)
	{
		m_player->SetHp(m_player->GetHp() - static_cast<INT>(data.damage));
	}
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
	{
		auto clearWindow{ make_unique<WindowObject>(400.0f, 300.0f) };
		clearWindow->SetMesh(s_meshes["UI"]);
		clearWindow->SetShader(s_shaders["UI"]);
		clearWindow->SetTexture(s_textures["WHITE"]);

		auto goToMainText{ make_unique<MenuTextObject>() };
		goToMainText->SetBrush("BLACK");
		goToMainText->SetMouseOverBrush("BLUE");
		goToMainText->SetFormat("48_RIGHT");
		goToMainText->SetText(TEXT("메인으로"));
		goToMainText->SetPivot(ePivot::CENTERBOT);
		goToMainText->SetScreenPivot(ePivot::CENTERBOT);
		goToMainText->SetPosition(XMFLOAT2{ 0.0f, 0.0f });
		goToMainText->SetMouseClickCallBack(
			[]()
			{
				g_gameFramework.SetNextScene(eSceneType::MAIN);
				ShowCursor(TRUE);
			});
		clearWindow->Add(move(goToMainText));

		auto descText{ make_unique<TextObject>() };
		descText->SetBrush("BLACK");
		descText->SetFormat("24_RIGHT");
		descText->SetPivot(ePivot::CENTER);
		descText->SetScreenPivot(ePivot::CENTER);
		descText->SetText(TEXT("모든 라운드를 클리어했습니다!"));
		descText->SetPosition(XMFLOAT2{ 0.0f, 0.0f });
		clearWindow->Add(move(descText));

		unique_lock<mutex> lock{ g_mutex };
		m_windowObjects.push_back(move(clearWindow));
		break;
	}
	case eRoundResult::OVER:
		break;
	case eRoundResult::ENDING:
		break;
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

void GameScene::SetPlayer(unique_ptr<Player>& player)
{
	m_player = move(player);
}

void GameScene::SetMultiPlayers(array<unique_ptr<Player>, Setting::MAX_PLAYERS>& multiPlayers)
{
	m_multiPlayers = move(multiPlayers);
}
