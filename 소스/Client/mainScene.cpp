#include "mainScene.h"
#include "framework.h"

void MainScene::OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature, const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	CreateGameObjects(device, commandList);
	CreateUIObjects(device, commandList);
	CreateTextObjects(d2dDeivceContext, dWriteFactory);
}

void MainScene::OnResize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	UINT width{ g_gameFramework.GetWidth() }, height{ g_gameFramework.GetHeight() };
	m_viewport = D3D12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
	m_scissorRect = D3D12_RECT{ 0, 0, static_cast<long>(width), static_cast<long>(height) };

	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(width) / static_cast<float>(height), 1.0f, 2500.0f));
	m_camera->SetProjMatrix(projMatrix);

	XMStoreFloat4x4(&projMatrix, XMMatrixOrthographicLH(static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f));
	m_uiCamera->SetProjMatrix(projMatrix);

	// UI, 텍스트 오브젝트들 재배치
	for (auto& ui : m_uiObjects)
		ui->SetPosition(ui->GetPivotPosition());
	for (auto& t : m_textObjects)
		t->SetPosition(t->GetPivotPosition());
}

void MainScene::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_LBUTTONDOWN:
		break;
	}
}

void MainScene::OnMouseEvent(HWND hWnd, FLOAT deltaTime)
{
	for (auto& t : m_textObjects)
		t->OnMouseEvent(hWnd, deltaTime);
}

void MainScene::OnKeyboardEvent(FLOAT deltaTime)
{

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
}

void MainScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
}

void MainScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);
	commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

	if (m_skybox) m_skybox->Render(commandList);

	for (const auto& o : m_gameObjects)
		o->Render(commandList);

	if (m_uiCamera)
	{
		m_uiCamera->UpdateShaderVariable(commandList);
		for (const auto& ui : m_uiObjects)
			ui->Render(commandList);
	}
}

void MainScene::Render2D(const ComPtr<ID2D1DeviceContext2>& device)
{
	for (const auto& t : m_textObjects)
		t->Render(device);
}

void MainScene::Update(FLOAT deltaTime)
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

void MainScene::CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 카메라
	m_camera = make_shared<Camera>();
	m_camera->CreateShaderVariable(device, commandList);
	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(Setting::SCREEN_WIDTH) / static_cast<float>(Setting::SCREEN_HEIGHT), 1.0f, 2500.0f));
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
	player->SetGunMesh(s_meshes["AR"]);
	player->SetGunShader(s_shaders["LINK"]);
	player->SetGunType(eGunType::AR);
	player->PlayAnimation("IDLE");
	m_gameObjects.push_back(move(player));
}

void MainScene::CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// UI 카메라 생성
	XMFLOAT4X4 projMatrix{};
	m_uiCamera = make_unique<Camera>();
	m_uiCamera->CreateShaderVariable(device, commandList);
	XMStoreFloat4x4(&projMatrix, XMMatrixOrthographicLH(static_cast<float>(Setting::SCREEN_WIDTH), static_cast<float>(Setting::SCREEN_HEIGHT), 0.0f, 1.0f));
	m_uiCamera->SetProjMatrix(projMatrix);

	// 테스트
	//auto testObject{ make_unique<UIObject>(150.0f, 50.0f) };
	//testObject->SetMesh(s_meshes["UI"]);
	//testObject->SetShader(s_shaders["UI"]);
	//testObject->SetTexture(s_textures["WHITE"]);
	//testObject->SetPivot(ePivot::CENTER);
	//testObject->SetScreenPivot(ePivot::CENTER);
	//m_uiObjects.push_back(move(testObject));
}

void MainScene::CreateTextObjects(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	auto gameStartText{ make_unique<MenuTextObject>() };
	gameStartText->SetBrush("BLACK");
	gameStartText->SetMouseOverBrush("BLUE");
	gameStartText->SetFormat("HP");
	gameStartText->SetText(TEXT("게임시작"));
	gameStartText->SetScreenPivot(ePivot::LEFTBOT);
	gameStartText->SetPosition(XMFLOAT2{ 50.0f, -180.0f });
	m_textObjects.push_back(move(gameStartText));

	auto settingText{ make_unique<MenuTextObject>() };
	settingText->SetBrush("BLACK");
	settingText->SetMouseOverBrush("BLUE");
	settingText->SetFormat("HP");
	settingText->SetText(TEXT("설정"));
	settingText->SetScreenPivot(ePivot::LEFTBOT);
	settingText->SetPosition(XMFLOAT2{ 50.0f, -130.0f });
	m_textObjects.push_back(move(settingText));

	auto exitText{ make_unique<MenuTextObject>() };
	exitText->SetBrush("BLACK");
	exitText->SetMouseOverBrush("BLUE");
	exitText->SetFormat("HP");
	exitText->SetText(TEXT("종료"));
	exitText->SetScreenPivot(ePivot::LEFTBOT);
	exitText->SetPosition(XMFLOAT2{ 50.0f, -80.0f });
	m_textObjects.push_back(move(exitText));
}
