﻿#include "mainScene.h"
#include "framework.h"

MainScene::MainScene() : m_pcbGameScene{ nullptr }
{

}

void MainScene::OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature, const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	CreateShaderVariable(device, commandList);
	CreateGameObjects(device, commandList);
	CreateUIObjects(device, commandList);
	CreateTextObjects(d2dDeivceContext, dWriteFactory);
	CreateLights();
	m_shadowMap = make_unique<ShadowMap>(device, 1 << 12, 1 << 12, Setting::SHADOWMAP_COUNT);
}

void MainScene::OnResize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	m_viewport = D3D12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(g_width), static_cast<float>(g_height), 0.0f, 1.0f };
	m_scissorRect = D3D12_RECT{ 0, 0, static_cast<long>(g_width), static_cast<long>(g_height) };

	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(g_width) / static_cast<float>(g_height), 1.0f, 2500.0f));
	m_camera->SetProjMatrix(projMatrix);

	XMStoreFloat4x4(&projMatrix, XMMatrixOrthographicLH(static_cast<float>(g_width), static_cast<float>(g_height), 0.0f, 1.0f));
	m_uiCamera->SetProjMatrix(projMatrix);

	// UI, 텍스트 오브젝트들 재배치
	for (auto& ui : m_uiObjects)
		ui->SetPosition(ui->GetPivotPosition());
	for (auto& t : m_textObjects)
		t->SetPosition(t->GetPivotPosition());
	for (auto& w : m_windowObjects)
		w->OnResize(hWnd, message, wParam, lParam);
}

void MainScene::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (!m_windowObjects.empty())
	{
		m_windowObjects.back()->OnMouseEvent(hWnd, message, wParam, lParam);
		return;
	}

	for (auto& t : m_textObjects)
		t->OnMouseEvent(hWnd, message, wParam, lParam);
}

void MainScene::OnMouseEvent(HWND hWnd, FLOAT deltaTime)
{
	if (!m_windowObjects.empty())
		m_windowObjects.back()->OnMouseEvent(hWnd, deltaTime);
	for (auto& t : m_textObjects)
		t->OnMouseEvent(hWnd, deltaTime);
}

void MainScene::OnKeyboardEvent(FLOAT deltaTime)
{
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
		g_gameFramework.SetNextScene(eScene::GAME);
}

void MainScene::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

}

void MainScene::OnUpdate(FLOAT deltaTime)
{
	Update(deltaTime);
	if (m_camera) m_camera->Update(deltaTime);
	if (m_skybox) m_skybox->Update(deltaTime);
	for (auto& o : m_gameObjects)
		o->Update(deltaTime);
	for (auto& ui : m_uiObjects)
		ui->Update(deltaTime);
	for (auto& t : m_textObjects)
		t->Update(deltaTime);
	for (auto& w : m_windowObjects)
		w->Update(deltaTime);
}

void MainScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	memcpy(m_pcbGameScene, m_cbGameSceneData.get(), sizeof(cbGameScene));
	commandList->SetGraphicsRootConstantBufferView(3, m_cbGameScene->GetGPUVirtualAddress());
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
}

void MainScene::PreRender(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	RenderToShadowMap(commandList);
}

void MainScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);
	commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

	if (m_skybox) m_skybox->Render(commandList);
	for (const auto& p : m_players)
		p->Render(commandList);
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

void MainScene::Render2D(const ComPtr<ID2D1DeviceContext2>& device)
{
	for (const auto& t : m_textObjects)
		t->Render(device);
	for (const auto& w : m_windowObjects)
		w->Render2D(device);
}

void MainScene::Update(FLOAT deltaTime)
{
	UpdateCameraPosition(deltaTime);
	UpdateShadowMatrix();
}

void MainScene::UpdateCameraPosition(FLOAT deltaTime)
{
	constexpr XMFLOAT3 target{ 0.0f, 0.0f, 0.0f };
	constexpr float speed{ 10.0f };
	constexpr float radius{ 200.0f };
	static float angle{ 0.0f };

	XMFLOAT3 eye{ 0.0f, 100.0f, 0.0f };
	eye.x = cosf(angle) * radius;
	eye.z = sinf(angle) * radius;

	XMFLOAT3 at{ Vector3::Normalize(Vector3::Sub(target, eye)) };
	m_camera->SetEye(eye);
	m_camera->SetAt(at);

	angle += XMConvertToRadians(speed * deltaTime);
}

void MainScene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_cbGameScene = Utile::CreateBufferResource(device, commandList, NULL, Utile::GetConstantBufferSize<cbGameScene>(), 1, D3D12_HEAP_TYPE_UPLOAD, {});
	m_cbGameScene->Map(0, NULL, reinterpret_cast<void**>(&m_pcbGameScene));
	m_cbGameSceneData = make_unique<cbGameScene>();
}

void MainScene::CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 카메라
	m_camera = make_shared<Camera>();
	m_camera->CreateShaderVariable(device, commandList);
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

	// 뒷 배경
	//auto object{ make_unique<GameObject>() };
	//object->SetMesh(s_meshes["TREE"]);
	//object->SetShader(s_shaders["MODEL"]);
	//object->SetTexture(s_textures["OBJECT2"]);
	//m_gameObjects.push_back(move(object));

	auto player{ make_unique<Player>(TRUE) };
	player->SetMesh(s_meshes["PLAYER"]);
	player->SetShader(s_shaders["ANIMATION"]);
	player->SetShadowShader(s_shaders["SHADOW_ANIMATION"]);
	player->SetGunMesh(s_meshes["AR"]);
	player->SetGunShader(s_shaders["LINK"]);
	player->SetGunShadowShader(s_shaders["SHADOW_LINK"]);
	player->SetGunType(eGunType::AR);
	player->PlayAnimation("IDLE");
	m_players.push_back(move(player));
}

void MainScene::CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// UI 카메라 생성
	XMFLOAT4X4 projMatrix{};
	m_uiCamera = make_unique<Camera>();
	m_uiCamera->CreateShaderVariable(device, commandList);
	XMStoreFloat4x4(&projMatrix, XMMatrixOrthographicLH(static_cast<float>(g_width), static_cast<float>(g_height), 0.0f, 1.0f));
	m_uiCamera->SetProjMatrix(projMatrix);

	auto title{ make_unique<UIObject>(801.0f, 377.0f) };
	title->SetMesh(s_meshes["UI"]);
	title->SetShader(s_shaders["UI_ATC"]);
	title->SetTexture(s_textures["TITLE"]);
	title->SetPivot(ePivot::LEFTCENTER);
	title->SetScreenPivot(ePivot::LEFTCENTER);
	title->SetPosition(XMFLOAT2{ 50.0f, 0.0f });
	title->SetScale(XMFLOAT2{ 0.5f, 0.5f });
	title->SetFitToScreen(TRUE);
	m_uiObjects.push_back(move(title));
}

void MainScene::CreateTextObjects(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	auto gameStartText{ make_unique<MenuTextObject>() };
	gameStartText->SetBrush("BLACK");
	gameStartText->SetMouseOverBrush("WHITE");
	gameStartText->SetFormat("MENU");
	gameStartText->SetText(TEXT("게임시작"));
	gameStartText->SetPivot(ePivot::LEFTBOT);
	gameStartText->SetScreenPivot(ePivot::LEFTBOT);
	gameStartText->SetPosition(XMFLOAT2{ 50.0f, -200.0f });
	gameStartText->SetMouseClickCallBack(
		[]()
		{
			g_gameFramework.SetNextScene(eScene::GAME);
		});
	m_textObjects.push_back(move(gameStartText));

	auto settingText{ make_unique<MenuTextObject>() };
	settingText->SetBrush("BLACK");
	settingText->SetMouseOverBrush("WHITE");
	settingText->SetFormat("MENU");
	settingText->SetText(TEXT("설정"));
	settingText->SetPivot(ePivot::LEFTBOT);
	settingText->SetScreenPivot(ePivot::LEFTBOT);
	settingText->SetPosition(XMFLOAT2{ 50.0f, -140.0f });
	settingText->SetMouseClickCallBack(bind(&MainScene::CreateSettingWindow, this));
	m_textObjects.push_back(move(settingText));

	auto exitText{ make_unique<MenuTextObject>() };
	exitText->SetBrush("BLACK");
	exitText->SetMouseOverBrush("WHITE");
	exitText->SetFormat("MENU");
	exitText->SetText(TEXT("종료"));
	exitText->SetPivot(ePivot::LEFTBOT);
	exitText->SetScreenPivot(ePivot::LEFTBOT);
	exitText->SetPosition(XMFLOAT2{ 50.0f, -80.0f });
	exitText->SetMouseClickCallBack(
		[]()
		{
			PostMessage(NULL, WM_QUIT, 0, 0);
		}
	);
	m_textObjects.push_back(move(exitText));
}

void MainScene::CreateLights() const
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

void MainScene::CreateSettingWindow()
{
	auto close{ make_unique<MenuUIObject>(25.0f, 25.0f) };
	close->SetMesh(s_meshes["UI"]);
	close->SetShader(s_shaders["UI"]);
	close->SetTexture(s_textures["HPBAR"]);
	close->SetScreenPivot(ePivot::LEFTTOP);
	close->SetPivot(ePivot::LEFTTOP);
	close->SetPosition(XMFLOAT2{ 25.0f, -25.0f });
	close->SetMouseClickCallBack(bind(&MainScene::CloseWindow, this));

	auto text{ make_unique<TextObject>() };
	text->SetBrush("BLACK");
	text->SetFormat("MENU");
	text->SetText(TEXT("설정"));
	text->SetPivot(ePivot::CENTERTOP);
	text->SetScreenPivot(ePivot::CENTERTOP);
	text->SetPosition(XMFLOAT2{ 0.0f, -25.0f });

	auto windowSizeText1{ make_unique<MenuTextObject>() };
	windowSizeText1->SetBrush("BLACK");
	windowSizeText1->SetMouseOverBrush("BLUE");
	windowSizeText1->SetFormat("MENU");
	windowSizeText1->SetText(TEXT("1280x720"));
	windowSizeText1->SetPivot(ePivot::CENTER);
	windowSizeText1->SetScreenPivot(ePivot::CENTER);
	windowSizeText1->SetPosition(XMFLOAT2{ 0.0f, -75.0f });
	windowSizeText1->SetMouseClickCallBack(
		[]()
		{
			g_gameFramework.SetIsFullScreen(FALSE);
			RECT lastWindowRect{ g_gameFramework.GetLastWindowRect() };
			SetWindowLong(g_gameFramework.GetWindow(), GWL_STYLE, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION);
			SetWindowPos(g_gameFramework.GetWindow(), HWND_TOP, lastWindowRect.left, lastWindowRect.top, 1280, 720, SWP_SHOWWINDOW);
		});

	auto windowSizeText2{ make_unique<MenuTextObject>() };
	windowSizeText2->SetBrush("BLACK");
	windowSizeText2->SetMouseOverBrush("BLUE");
	windowSizeText2->SetFormat("MENU");
	windowSizeText2->SetText(TEXT("1680x1050"));
	windowSizeText2->SetPivot(ePivot::CENTER);
	windowSizeText2->SetScreenPivot(ePivot::CENTER);
	windowSizeText2->SetPosition(XMFLOAT2{ 0.0f, 0.0f });
	windowSizeText2->SetMouseClickCallBack(
		[]()
		{
			g_gameFramework.SetIsFullScreen(FALSE);
			RECT lastWindowRect{ g_gameFramework.GetLastWindowRect() };
			SetWindowLong(g_gameFramework.GetWindow(), GWL_STYLE, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION);
			SetWindowPos(g_gameFramework.GetWindow(), HWND_TOP, lastWindowRect.left, lastWindowRect.top, 1680, 1050, SWP_SHOWWINDOW);
		});

	auto windowSizeText3{ make_unique<MenuTextObject>() };
	windowSizeText3->SetBrush("BLACK");
	windowSizeText3->SetMouseOverBrush("BLUE");
	windowSizeText3->SetFormat("MENU");
	windowSizeText3->SetText(TEXT("전체화면"));
	windowSizeText3->SetPivot(ePivot::CENTER);
	windowSizeText3->SetScreenPivot(ePivot::CENTER);
	windowSizeText3->SetPosition(XMFLOAT2{ 0.0f, 75.0f });
	windowSizeText3->SetMouseClickCallBack(
		[]()
		{
			g_gameFramework.SetIsFullScreen(TRUE);
			RECT fullScreenRect{ g_gameFramework.GetFullScreenRect() };
			SetWindowPos(g_gameFramework.GetWindow(), HWND_TOP, 0, 0, fullScreenRect.right, fullScreenRect.bottom, SWP_SHOWWINDOW);

			LONG style = GetWindowLong(g_gameFramework.GetWindow(), GWL_STYLE);
			style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
			SetWindowLong(g_gameFramework.GetWindow(), GWL_STYLE, style);
			SetWindowPos(g_gameFramework.GetWindow(), NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
		});

	auto setting{ make_unique<WindowObject>(500.0f, 500.0f) };
	setting->SetMesh(s_meshes["UI"]);
	setting->SetShader(s_shaders["UI"]);
	setting->SetTexture(s_textures["WHITE"]);
	setting->Add(close);
	setting->Add(text);
	setting->Add(windowSizeText1);
	setting->Add(windowSizeText2);
	setting->Add(windowSizeText3);
	m_windowObjects.push_back(move(setting));
}

void MainScene::CloseWindow()
{
	if (!m_windowObjects.empty())
		m_windowObjects.pop_back();
}

void MainScene::RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
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
	for (const auto& p : m_players)
		p->RenderToShadowMap(commandList);

	// 리소스배리어 설정(셰이더에서 읽기)
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMap->GetShadowMap().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void MainScene::UpdateShadowMatrix()
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
