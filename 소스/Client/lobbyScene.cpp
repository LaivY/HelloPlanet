#include "lobbyScene.h"
#include "framework.h"

LobbyScene::LobbyScene() : m_isReadyToPlay{ FALSE }, m_loginCount{ 0 }, m_pcbGameScene{ nullptr }
{

}

LobbyScene::~LobbyScene()
{
	if (m_cbGameScene) m_cbGameScene->Unmap(0, NULL);
#ifdef NETWORK
	// 클라이언트 강제 종료 시
	//if (!m_isReadyToPlay)
	//{
	//	m_isReadyToPlay = TRUE;
	//	closesocket(g_socket);
	//	WSACleanup();
	//}

	// 다음 씬으로 넘어갈 경우
	//if (g_networkThread.joinable())
	//	g_networkThread.join();
#endif
}

void LobbyScene::OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature, const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	CreateShaderVariable(device, commandList);
	CreateGameObjects(device, commandList);
	CreateUIObjects(device, commandList);
	CreateTextObjects(d2dDeivceContext, dWriteFactory);
	CreateLights();
	m_shadowMap = make_unique<ShadowMap>(device, 1 << 12, 1 << 12, Setting::SHADOWMAP_COUNT);

#ifdef NETWORK
	//g_networkThread = thread{ &LobbyScene::RecvPacket, this };
#endif
}

void LobbyScene::OnDestroy()
{
	m_isReadyToPlay = TRUE;
	closesocket(g_socket);
	WSACleanup();
	if (g_networkThread.joinable())
		g_networkThread.join();
}

void LobbyScene::OnMouseEvent(HWND hWnd, FLOAT deltaTime)
{
	if (!m_windowObjects.empty())
	{
		m_windowObjects.back()->OnMouseEvent(hWnd, deltaTime);
		return;
	}
	for (auto& t : m_textObjects)
		t->OnMouseEvent(hWnd, deltaTime);
}

void LobbyScene::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (!m_windowObjects.empty())
	{
		m_windowObjects.back()->OnMouseEvent(hWnd, message, wParam, lParam);
		return;
	}
	for (auto& t : m_textObjects)
		t->OnMouseEvent(hWnd, message, wParam, lParam);
}

void LobbyScene::OnKeyboardEvent(FLOAT deltaTime)
{

}

void LobbyScene::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

}

void LobbyScene::OnUpdate(FLOAT deltaTime)
{
	Update(deltaTime);
	if (m_player) m_player->Update(deltaTime);
	if (m_camera) m_camera->Update(deltaTime);
	if (m_skybox) m_skybox->Update(deltaTime);
	for (auto& p : m_multiPlayers)
		if (p) p->Update(deltaTime);
	for (auto& o : m_gameObjects)
		o->Update(deltaTime);
	for (auto& ui : m_uiObjects)
		ui->Update(deltaTime);
	for (auto& t : m_textObjects)
		t->Update(deltaTime);
	for (auto& w : m_windowObjects)
		w->Update(deltaTime);
}

void LobbyScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	memcpy(m_pcbGameScene, m_cbGameSceneData.get(), sizeof(cbGameScene));
	commandList->SetGraphicsRootConstantBufferView(3, m_cbGameScene->GetGPUVirtualAddress());
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
}

void LobbyScene::PreRender(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	RenderToShadowMap(commandList);
}

void LobbyScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);
	commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

	if (m_skybox) m_skybox->Render(commandList);
	if (m_player) m_player->Render(commandList);
	for (const auto& p : m_multiPlayers)
		if (p) p->Render(commandList);
	for (const auto& o : m_gameObjects)
		o->Render(commandList);
	if (m_uiCamera)
	{
		m_uiCamera->UpdateShaderVariable(commandList);
		for (const auto& ui : m_uiObjects)
			ui->Render(commandList);
		for (const auto& w : m_windowObjects)
			w->Render(commandList);
	}
}

void LobbyScene::Render2D(const ComPtr<ID2D1DeviceContext2>& device)
{
	for (const auto& t : m_textObjects)
		t->Render(device);
	for (const auto& w : m_windowObjects)
		w->Render2D(device);
}

void LobbyScene::RecvPacket()
{
	constexpr char name[10] = "Hello\0";

	cs_packet_login packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_LOGIN;
	memcpy(packet.name, name, sizeof(char) * 10);
	send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), NULL);

	while (!m_isReadyToPlay)
		ProcessPacket();
}

void LobbyScene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_cbGameScene = Utile::CreateBufferResource(device, commandList, NULL, Utile::GetConstantBufferSize<cbGameScene>(), 1, D3D12_HEAP_TYPE_UPLOAD, {});
	m_cbGameScene->Map(0, NULL, reinterpret_cast<void**>(&m_pcbGameScene));
	m_cbGameSceneData = make_unique<cbGameScene>();
}

void LobbyScene::CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 카메라
	m_camera = make_shared<Camera>();
	m_camera->CreateShaderVariable(device, commandList);
	m_camera->SetEye(XMFLOAT3{ 0.0f, 35.0f, 50.0f });
	m_camera->SetAt(Vector3::Normalize(Vector3::Sub(XMFLOAT3{ 0.0f, 20.0f, 0.0f }, m_camera->GetEye())));

	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(g_width) / static_cast<float>(g_height), 1.0f, 2500.0f));
	m_camera->SetProjMatrix(projMatrix);

	// 스카이박스
	m_skybox = make_unique<Skybox>();
	m_skybox->SetMesh(s_meshes["SKYBOX"]);
	m_skybox->SetShader(s_shaders["SKYBOX"]);
	m_skybox->SetTexture(s_textures["SKYBOX"]);
	m_skybox->SetCamera(m_camera);

	// 바닥
	auto floor{ make_unique<GameObject>() };
	floor->SetMesh(s_meshes["FLOOR"]);
	floor->SetShader(s_shaders["DEFAULT"]);
	m_gameObjects.push_back(move(floor));

	// 비행기
	auto dropship{ make_unique<GameObject>() };
	dropship->SetMesh(s_meshes["OBJECT9"]);
	dropship->SetShader(s_shaders["MODEL"]);
	dropship->SetPosition(XMFLOAT3{ -500.0f, 0.0f, -900.0f });
	m_gameObjects.push_back(move(dropship));

	// 플레이어
	m_player = make_unique<Player>(TRUE);
	m_player->SetMesh(s_meshes["PLAYER"]);
	m_player->SetShader(s_shaders["ANIMATION"]);
	m_player->SetShadowShader(s_shaders["SHADOW_ANIMATION"]);
	m_player->SetGunMesh(s_meshes["AR"]);
	m_player->SetGunShader(s_shaders["LINK"]);
	m_player->SetGunShadowShader(s_shaders["SHADOW_LINK"]);
	m_player->SetGunType(eGunType::AR);
	m_player->PlayAnimation("IDLE");
	m_player->PlayAnimation("RELOAD");
	m_player->SetCamera(m_camera);
}

void LobbyScene::CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{

}

void LobbyScene::CreateTextObjects(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	constexpr auto readyText{ TEXT("준비") };
	constexpr auto readyOkText{ TEXT("준비 완료") };

	constexpr auto readyCallBack = 
	[](MenuTextObject* readyTextObject)
	{
#ifdef NETWORK
		//cs_packet_ready packet{};
		//packet.size = sizeof(packet);
		//packet.type = CS_PACKET_READY;

		//wstring text{ readyTextObject->GetText() };
		//if (text == readyText)
		//{
		//	readyTextObject->SetBrush("BLUE");
		//	readyTextObject->SetMouseOverBrush("BLUE");
		//	readyTextObject->SetText(readyOkText);
		//	packet.state = true;
		//}
		//else
		//{
		//	readyTextObject->SetBrush("BLACK");
		//	readyTextObject->SetMouseOverBrush("BLACK");
		//	readyTextObject->SetText(readyText);
		//	packet.state = false;
		//}
		//readyTextObject->SetPosition(XMFLOAT2{ 0.0f, -30.0f });
		//send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), NULL);
		g_gameFramework.SetNextScene(eScene::GAME);
#else
		g_gameFramework.SetNextScene(eScene::GAME);
#endif
	};

	auto leftCallBack =
	[&](MenuTextObject* readyTextObject)
	{
		if (readyTextObject->GetText() == readyOkText)
			return;

		eGunType type{ m_player->GetGunType() };
		switch (type)
		{
		case eGunType::AR:
			m_player->SetGunMesh(s_meshes["MG"]);
			m_player->SetGunType(eGunType::MG);
			break;
		case eGunType::SG:
			m_player->SetGunMesh(s_meshes["AR"]);
			m_player->SetGunType(eGunType::AR);
			break;
		case eGunType::MG:
			m_player->SetGunMesh(s_meshes["SG"]);
			m_player->SetGunType(eGunType::SG);
			break;
		}
		m_player->PlayAnimation("RELOAD");
	};

	auto rightCallBack =
	[&](MenuTextObject* readyTextObject)
	{
		if (readyTextObject->GetText() == readyOkText)
			return;

		eGunType type{ m_player->GetGunType() };
		switch (type)
		{
		case eGunType::AR:
			m_player->SetGunMesh(s_meshes["SG"]);
			m_player->SetGunType(eGunType::SG);
			break;
		case eGunType::SG:
			m_player->SetGunMesh(s_meshes["MG"]);
			m_player->SetGunType(eGunType::MG);
			break;
		case eGunType::MG:
			m_player->SetGunMesh(s_meshes["AR"]);
			m_player->SetGunType(eGunType::AR);
			break;
		}
		m_player->PlayAnimation("RELOAD");
	};

	auto ready{ make_unique<MenuTextObject>() };
	ready->SetBrush("BLACK");
	ready->SetMouseOverBrush("BLACK");
	ready->SetFormat("MENU");
	ready->SetText(TEXT("준비"));
	ready->SetPivot(ePivot::CENTERBOT);
	ready->SetScreenPivot(ePivot::CENTERBOT);
	ready->SetPosition(XMFLOAT2{ 0.0f, -30.0f });
	ready->SetMouseClickCallBack(bind(readyCallBack, ready.get()));
	
	auto leftText{ make_unique<MenuTextObject>() };
	leftText->SetBrush("BLACK");
	leftText->SetMouseOverBrush("BLUE");
	leftText->SetFormat("MENU");
	leftText->SetText(TEXT("<"));
	leftText->SetPivot(ePivot::CENTERBOT);
	leftText->SetScreenPivot(ePivot::CENTERBOT);
	leftText->SetPosition(XMFLOAT2{ -120.0f, -30.0f });
	leftText->SetMouseClickCallBack(bind(leftCallBack, ready.get()));
	m_textObjects.push_back(move(leftText));

	auto rightText{ make_unique<MenuTextObject>() };
	rightText->SetBrush("BLACK");
	rightText->SetMouseOverBrush("BLUE");
	rightText->SetFormat("MENU");
	rightText->SetText(TEXT(">"));
	rightText->SetPivot(ePivot::CENTERBOT);
	rightText->SetScreenPivot(ePivot::CENTERBOT);
	rightText->SetPosition(XMFLOAT2{ 120.0f, -30.0f });
	rightText->SetMouseClickCallBack(bind(rightCallBack, ready.get()));
	m_textObjects.push_back(move(rightText));
	m_textObjects.push_back(move(ready));

	auto exit{ make_unique<MenuTextObject>() };
	exit->SetBrush("BLACK");
	exit->SetMouseOverBrush("BLUE");
	exit->SetFormat("MENU");
	exit->SetText(TEXT("나가기"));
	exit->SetPivot(ePivot::RIGHTBOT);
	exit->SetScreenPivot(ePivot::RIGHTBOT);
	exit->SetPosition(XMFLOAT2{ -50.0f, -30.0f });
	exit->SetMouseClickCallBack(
		[]()
		{
			g_gameFramework.SetNextScene(eScene::MAIN);
		}
	);
	m_textObjects.push_back(move(exit));
}

void LobbyScene::CreateLights() const
{
	m_cbGameSceneData->shadowLight.color = XMFLOAT3{ 0.1f, 0.1f, 0.1f };
	m_cbGameSceneData->shadowLight.direction = Vector3::Normalize(XMFLOAT3{ -0.687586f, -0.716385f, 0.118001f });

	XMFLOAT4X4 lightViewMatrix, lightProjMatrix;
	XMFLOAT3 shadowLightPos{ Vector3::Mul(m_cbGameSceneData->shadowLight.direction, -1500.0f) };
	XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
	XMStoreFloat4x4(&lightViewMatrix, XMMatrixLookAtLH(XMLoadFloat3(&shadowLightPos), XMLoadFloat3(&m_cbGameSceneData->shadowLight.direction), XMLoadFloat3(&up)));
	XMStoreFloat4x4(&lightProjMatrix, XMMatrixOrthographicLH(3000.0f, 3000.0f, 0.0f, 5000.0f));
	m_cbGameSceneData->shadowLight.lightViewMatrix[Setting::SHADOWMAP_COUNT - 1] = Matrix::Transpose(lightViewMatrix);
	m_cbGameSceneData->shadowLight.lightProjMatrix[Setting::SHADOWMAP_COUNT - 1] = Matrix::Transpose(lightProjMatrix);

	m_cbGameSceneData->ligths[0].color = XMFLOAT3{ 0.05f, 0.05f, 0.05f };
	m_cbGameSceneData->ligths[0].direction = XMFLOAT3{ 0.0f, -6.0f, 10.0f };
}

void LobbyScene::CloseWindow()
{
	if (!m_windowObjects.empty())
		m_windowObjects.pop_back();
}

void LobbyScene::Update(FLOAT deltaTime)
{
	erase_if(m_windowObjects, [](unique_ptr<WindowObject>& object) { return object->isDeleted(); });
	UpdateShadowMatrix();
}

void LobbyScene::UpdateShadowMatrix()

{
	constexpr array<float, Setting::SHADOWMAP_COUNT> casecade{ 0.0f, 0.05f, 0.2f, 0.4f };
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

	XMFLOAT4X4 toWorldMatrix{ Matrix::Inverse(Matrix::Mul(m_camera->GetViewMatrix(), m_camera->GetProjMatrix())) };
	for (auto& v : frustum)
		v = Vector3::TransformCoord(v, toWorldMatrix);

	for (int i = 0; i < casecade.size() - 1; ++i)
	{
		XMFLOAT3 tFrustum[8];
		for (int j = 0; j < 8; ++j)
			tFrustum[j] = frustum[j];

		for (int j = 0; j < 4; ++j)
		{
			XMFLOAT3 v{ Vector3::Sub(tFrustum[j + 4], tFrustum[j]) };
			XMFLOAT3 n{ Vector3::Mul(v, casecade[i]) };
			XMFLOAT3 f{ Vector3::Mul(v, casecade[i + 1]) };

			tFrustum[j + 4] = Vector3::Add(tFrustum[j], f);
			tFrustum[j] = Vector3::Add(tFrustum[j], n);
		}

		XMFLOAT3 center{};
		for (const auto& v : tFrustum)
			center = Vector3::Add(center, v);
		center = Vector3::Mul(center, 1.0f / 8.0f);

		float radius{};
		for (const auto& v : tFrustum)
			radius = max(radius, Vector3::Length(Vector3::Sub(v, center)));

		float value{ max(750.0f, radius * 2.5f) };
		XMFLOAT3 shadowLightPos{ Vector3::Add(center, Vector3::Mul(m_cbGameSceneData->shadowLight.direction, -value)) };

		XMFLOAT4X4 lightViewMatrix, lightProjMatrix;
		XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
		XMStoreFloat4x4(&lightViewMatrix, XMMatrixLookAtLH(XMLoadFloat3(&shadowLightPos), XMLoadFloat3(&center), XMLoadFloat3(&up)));
		XMStoreFloat4x4(&lightProjMatrix, XMMatrixOrthographicLH(radius * 2.0f, radius * 2.0f, 0.0f, 5000.0f));

		m_cbGameSceneData->shadowLight.lightViewMatrix[i] = Matrix::Transpose(lightViewMatrix);
		m_cbGameSceneData->shadowLight.lightProjMatrix[i] = Matrix::Transpose(lightProjMatrix);
	}
}

void LobbyScene::RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
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
	for (const auto& o : m_gameObjects)
	{
		auto shadowShader{ o->GetShadowShader() };
		if (shadowShader)
			o->Render(commandList, shadowShader);
	}
	if (m_player) m_player->RenderToShadowMap(commandList);
	for (const auto& p : m_multiPlayers)
		if (p) p->RenderToShadowMap(commandList);

	// 리소스배리어 설정(셰이더에서 읽기)
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMap->GetShadowMap().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void LobbyScene::ProcessPacket()
{
	char buf[2]{};
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD recvByte{ 0 }, recvFlag{ 0 };
	if (WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr) == SOCKET_ERROR)
		error_display("RecvSizeType");

	UCHAR size{ static_cast<UCHAR>(buf[0]) };
	UCHAR type{ static_cast<UCHAR>(buf[1]) };

	switch (type)
	{
	case SC_PACKET_LOGIN_OK:
		RecvLoginOkPacket();
		break;
	case SC_PACKET_READY:
		RecvReady();
		break;
	}
}

void LobbyScene::RecvLoginOkPacket()
{
	// 플레이어정보 + 닉네임
	char buf[sizeof(PlayerData) + 10]{};
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	if (!m_player) return;

	PlayerData data{};
	char name[10]{};
	memcpy(&data, buf, sizeof(data));
	memcpy(&name, &buf[sizeof(PlayerData)], sizeof(name));
	OutputDebugStringA(("RECV LOGIN OK PACKET (id : " + to_string(data.id) + ")\n").c_str());

	if (m_player->GetId() == -1)
	{
		m_player->SetId(static_cast<int>(data.id));
		return;
	}

	if (m_player->GetId() != data.id)
		for (auto& p : m_multiPlayers)
		{
			if (p) continue;
			p = make_unique<Player>(TRUE);
			p->SetMesh(s_meshes["PLAYER"]);
			p->SetShader(s_shaders["ANIMATION"]);
			p->SetShadowShader(s_shaders["SHADOW_ANIMATION"]);
			p->SetGunMesh(s_meshes["AR"]);
			p->SetGunShader(s_shaders["LINK"]);
			p->SetGunShadowShader(s_shaders["SHADOW_LINK"]);
			p->SetGunType(eGunType::AR);
			p->PlayAnimation("IDLE");
			p->SetId(static_cast<int>(data.id));
			p->ApplyServerData(data);
			if (m_loginCount == 0)
			{
				p->Move(XMFLOAT3{ 25.0f, 0.0f, -20.0f });
				++m_loginCount;
			}
			else if (m_loginCount == 1)
			{
				p->Move(XMFLOAT3{ -25.0f, 0.0f, -20.0f });
			}
			break;
		}
}

void LobbyScene::RecvReady()
{
	// 아이디, 준비상태
	char buf[1 + 1]{};
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	int id{ static_cast<int>(buf[0]) };
	bool state{ static_cast<bool>(buf[1]) };
}
