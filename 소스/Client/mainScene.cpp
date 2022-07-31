#include "stdafx.h"
#include "mainScene.h"
#include "audioEngine.h"
#include "camera.h"
#include "framework.h"
#include "object.h"
#include "player.h"
#include "textObject.h"
#include "uiObject.h"
#include "windowObject.h"
#include "texture.h"

MainScene::MainScene() : m_pcbGameScene{ nullptr }
{
	
}

MainScene::~MainScene()
{
	if (m_cbGameScene) m_cbGameScene->Unmap(0, NULL);
}

void MainScene::OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
					   const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature,
					   const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	CreateShaderVariable(device, commandList);
	CreateGameObjects(device, commandList);
	CreateUIObjects(device, commandList);
	CreateTextObjects(d2dDeivceContext, dWriteFactory);
	CreateLights();
	LoadMapObjects(device, commandList, Utile::PATH("map.txt"));
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

void MainScene::OnMouseEvent(HWND hWnd, FLOAT deltaTime)
{
	if (!m_windowObjects.empty())
	{
		m_windowObjects.back()->OnMouseEvent(hWnd, deltaTime);
		return;
	}
	for (auto& t : m_textObjects)
		t->OnMouseEvent(hWnd, deltaTime);
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
	for (const auto& o : m_gameObjects)
		o->Render(commandList);
	for (const auto& p : m_players)
		p->Render(commandList);
	if (m_uiCamera)
	{
		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
		m_uiCamera->UpdateShaderVariable(commandList);
		for (const auto& ui : m_uiObjects)
			ui->Render(commandList);
		for (const auto& w : m_windowObjects)
			w->Render(commandList);
	}
}

void MainScene::Render2D(const ComPtr<ID2D1DeviceContext2>& device) const
{
	for (const auto& t : m_textObjects)
		t->Render(device);
	for (const auto& w : m_windowObjects)
		w->Render2D(device);
}

void MainScene::Update(FLOAT deltaTime)
{
	erase_if(m_windowObjects, [](unique_ptr<WindowObject>& object) { return !object->isValid(); });
	UpdateCameraPosition(deltaTime);
	UpdateShadowMatrix();
}

void MainScene::CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 카메라
	XMFLOAT4X4 projMatrix{};
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(g_width) / static_cast<float>(g_height), 1.0f, 2500.0f));
	m_camera = make_shared<Camera>();
	m_camera->CreateShaderVariable(device, commandList);
	m_camera->SetProjMatrix(projMatrix);

	// UI 카메라 생성
	XMStoreFloat4x4(&projMatrix, XMMatrixOrthographicLH(static_cast<float>(g_width), static_cast<float>(g_height), 0.0f, 1.0f));
	m_uiCamera = make_unique<Camera>();
	m_uiCamera->CreateShaderVariable(device, commandList);
	m_uiCamera->SetProjMatrix(projMatrix);

	// 스카이박스
	m_skybox = make_unique<Skybox>();
	m_skybox->SetCamera(m_camera);

	// 바닥
	auto floor{ make_unique<GameObject>() };
	floor->SetMesh(s_meshes["FLOOR"]);
	floor->SetShader(s_shaders["DEFAULT"]);
	m_gameObjects.push_back(move(floor));

	auto player{ make_unique<Player>(TRUE) };
	player->SetWeaponType(eWeaponType::AR);
	m_players.push_back(move(player));
}

void MainScene::CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	auto title{ make_unique<UIObject>(801.0f, 377.0f) };
	title->SetShader(s_shaders["UI_ATC"]);
	title->SetTexture(s_textures["TITLE"]);
	title->SetPivot(ePivot::LEFTCENTER);
	title->SetScreenPivot(ePivot::LEFTCENTER);
	title->SetPosition(XMFLOAT2{ 50.0f, 60.0f });
	title->SetScale(XMFLOAT2{ 0.5f, 0.5f });
	title->SetFitToScreen(TRUE);
	m_uiObjects.push_back(move(title));
}

void MainScene::CreateTextObjects(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	auto gameStartText{ make_unique<MenuTextObject>() };
	gameStartText->SetBrush("BLACK");
	gameStartText->SetMouseOverBrush("WHITE");
	gameStartText->SetFormat("48R");
	gameStartText->SetText(TEXT("게임시작"));
	gameStartText->SetPivot(ePivot::LEFTBOT);
	gameStartText->SetScreenPivot(ePivot::LEFTBOT);
	gameStartText->SetPosition(XMFLOAT2{ 50.0f, -230.0f });
	gameStartText->SetMouseClickCallBack(
		[]()
		{
#ifdef NETWORK
			if (!g_gameFramework.ConnectServer())
			{
				MessageBox(NULL, TEXT("서버와 연결할 수 없습니다."), TEXT("알림"), MB_OK);
				return;
			}
#endif
			g_gameFramework.SetNextScene(eSceneType::LOBBY);
		});
	m_textObjects.push_back(move(gameStartText));

	auto manualText{ make_unique<MenuTextObject>() };
	manualText->SetBrush("BLACK");
	manualText->SetMouseOverBrush("WHITE");
	manualText->SetFormat("48R");
	manualText->SetText(TEXT("조작법"));
	manualText->SetPivot(ePivot::LEFTBOT);
	manualText->SetScreenPivot(ePivot::LEFTBOT);
	manualText->SetPosition(XMFLOAT2{ 50.0f, -170.0f });
	manualText->SetMouseClickCallBack(bind(&MainScene::CreateManualWindow, this));
	m_textObjects.push_back(move(manualText));

	auto settingText{ make_unique<MenuTextObject>() };
	settingText->SetBrush("BLACK");
	settingText->SetMouseOverBrush("WHITE");
	settingText->SetFormat("48R");
	settingText->SetText(TEXT("설정"));
	settingText->SetPivot(ePivot::LEFTBOT);
	settingText->SetScreenPivot(ePivot::LEFTBOT);
	settingText->SetPosition(XMFLOAT2{ 50.0f, -110.0f });
	settingText->SetMouseClickCallBack(bind(&MainScene::CreateSettingWindow, this));
	m_textObjects.push_back(move(settingText));

	auto exitText{ make_unique<MenuTextObject>() };
	exitText->SetBrush("BLACK");
	exitText->SetMouseOverBrush("WHITE");
	exitText->SetFormat("48R");
	exitText->SetText(TEXT("종료"));
	exitText->SetPivot(ePivot::LEFTBOT);
	exitText->SetScreenPivot(ePivot::LEFTBOT);
	exitText->SetPosition(XMFLOAT2{ 50.0f, -50.0f });
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
	m_cbGameSceneData->shadowLight.color = XMFLOAT3{ 0.5f, 0.5f, 0.5f };
	m_cbGameSceneData->shadowLight.direction = Vector3::Normalize(XMFLOAT3{ 0.2f, -1.0f, 0.2f });

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

void MainScene::LoadMapObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& mapFile)
{
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
	}
}

void MainScene::CreateSettingWindow()
{
	auto title{ make_unique<TextObject>() };
	title->SetBrush("BLACK");
	title->SetFormat("36R");
	title->SetText(TEXT("설정"));
	title->SetPivot(ePivot::LEFTCENTER);
	title->SetScreenPivot(ePivot::LEFTTOP);
	title->SetPosition(XMFLOAT2{ 0.0f, title->GetHeight() / 2.0f + 2.0f });

	auto resolution{ make_unique<TextObject>() };
	resolution->SetBrush("BLACK");
	resolution->SetFormat("36R");
	resolution->SetText(TEXT("해상도"));
	resolution->SetPivot(ePivot::CENTER);
	resolution->SetScreenPivot(ePivot::CENTERTOP);
	resolution->SetPosition(XMFLOAT2{ -150.0f, -resolution->GetHeight() / 2.0f - 10.0f });

	auto volume{ make_unique<TextObject>() };
	volume->SetBrush("BLACK");
	volume->SetFormat("36R");
	volume->SetText(TEXT("사운드"));
	volume->SetPivot(ePivot::CENTER);
	volume->SetScreenPivot(ePivot::CENTERTOP);
	volume->SetPosition(XMFLOAT2{ 150.0f, -volume->GetHeight() / 2.0f - 10.0f });

	auto close{ make_unique<MenuTextObject>() };
	close->SetBrush("BLACK");
	close->SetMouseOverBrush("BLUE");
	close->SetFormat("36R");
	close->SetText(TEXT("확인"));
	close->SetScreenPivot(ePivot::CENTERBOT);
	close->SetPivot(ePivot::CENTERBOT);
	close->SetPosition(XMFLOAT2{ 0.0f, -close->GetHeight() / 2.0f + 10.0f });
	close->SetMouseClickCallBack(
		[&]()
		{
			m_windowObjects.back()->Delete();
		});

	auto windowSizeText1{ make_unique<MenuTextObject>() };
	windowSizeText1->SetBrush("BLACK");
	windowSizeText1->SetMouseOverBrush("BLUE");
	windowSizeText1->SetFormat("32R");
	windowSizeText1->SetText(TEXT("1280x720"));
	windowSizeText1->SetPivot(ePivot::CENTER);
	windowSizeText1->SetScreenPivot(ePivot::CENTER);
	windowSizeText1->SetPosition(XMFLOAT2{ -150.0f, 40.0f });
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
	windowSizeText2->SetFormat("32R");
	windowSizeText2->SetText(TEXT("1680x1050"));
	windowSizeText2->SetPivot(ePivot::CENTER);
	windowSizeText2->SetScreenPivot(ePivot::CENTER);
	windowSizeText2->SetPosition(XMFLOAT2{ -150.0f, 0.0f });
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
	windowSizeText3->SetFormat("32R");
	windowSizeText3->SetText(TEXT("전체화면"));
	windowSizeText3->SetPivot(ePivot::CENTER);
	windowSizeText3->SetScreenPivot(ePivot::CENTER);
	windowSizeText3->SetPosition(XMFLOAT2{ -150.0f, -40.0f });
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

	auto music{ make_unique<TextObject>() };
	music->SetBrush("BLACK");
	music->SetFormat("32R");
	music->SetText(TEXT("배경음 : "));
	music->SetPivot(ePivot::CENTER);
	music->SetScreenPivot(ePivot::CENTER);

	auto sound{ make_unique<TextObject>() };
	sound->SetBrush("BLACK");
	sound->SetFormat("32R");
	sound->SetText(TEXT("효과음 : "));
	sound->SetPivot(ePivot::CENTER);
	sound->SetScreenPivot(ePivot::CENTER);

	int musicVolume{ g_audioEngine.GetVolume(eAudioType::MUSIC) };
	auto musicOnOff{ make_unique<MenuTextObject>() };
	musicOnOff->SetBrush("BLUE");
	musicOnOff->SetMouseOverBrush("BLUE");
	musicOnOff->SetFormat("32R");
	musicOnOff->SetText(to_wstring(musicVolume) + TEXT("%"));
	musicOnOff->SetPivot(ePivot::CENTER);
	musicOnOff->SetScreenPivot(ePivot::CENTER);
	musicOnOff->SetValue(musicVolume);
	musicOnOff->SetMouseClickCallBack(bind(
		[](MenuTextObject* object)
		{
			int volume{ object->GetValue() };
			volume = (volume + 10) % 110;
			if (volume == 0)
			{
				object->SetBrush("RED");
				object->SetMouseOverBrush("RED");
			}
			else
			{
				object->SetBrush("BLUE");
				object->SetMouseOverBrush("BLUE");
			}
			object->SetText(to_wstring(volume) + TEXT("%"));
			object->SetValue(volume);
			g_audioEngine.SetVolume(eAudioType::MUSIC, static_cast<float>(volume) / 100.0f);
		}, musicOnOff.get()));

	int soundVolume{ g_audioEngine.GetVolume(eAudioType::SOUND) };
	auto soundOnOff{ make_unique<MenuTextObject>() };
	soundOnOff->SetBrush("BLUE");
	soundOnOff->SetMouseOverBrush("BLUE");
	soundOnOff->SetFormat("32R");
	soundOnOff->SetText(to_wstring(soundVolume) + TEXT("%"));
	soundOnOff->SetPivot(ePivot::CENTER);
	soundOnOff->SetScreenPivot(ePivot::CENTER);
	soundOnOff->SetValue(soundVolume);
	soundOnOff->SetMouseClickCallBack(bind(
		[](MenuTextObject* object)
		{
			int volume{ object->GetValue() };
			volume = (volume + 10) % 110;
			if (volume == 0)
			{
				object->SetBrush("RED");
				object->SetMouseOverBrush("RED");
			}
			else
			{
				object->SetBrush("BLUE");
				object->SetMouseOverBrush("BLUE");
			}
			object->SetText(to_wstring(volume) + TEXT("%"));
			object->SetValue(volume);
			g_audioEngine.SetVolume(eAudioType::SOUND, static_cast<float>(volume) / 100.0f);
		}, soundOnOff.get()));

	music->SetPosition(XMFLOAT2{ 150.0f - musicOnOff->GetWidth() / 2.0f,  20.0f });
	sound->SetPosition(XMFLOAT2{ 150.0f - soundOnOff->GetWidth() / 2.0f, -20.0f });
	musicOnOff->SetPosition(XMFLOAT2{ 150.0f + (music->GetWidth() + 20.0f) / 2.0f,  20.0f });
	soundOnOff->SetPosition(XMFLOAT2{ 150.0f + (sound->GetWidth() + 20.0f) / 2.0f, -20.0f });

	auto setting{ make_unique<WindowObject>(600.0f, 320.0f) };
	setting->SetTexture(s_textures["WHITE"]);
	setting->SetPosition(XMFLOAT2{});
	setting->Add(title);
	setting->Add(resolution);
	setting->Add(volume);
	setting->Add(close);
	setting->Add(windowSizeText1);
	setting->Add(windowSizeText2);
	setting->Add(windowSizeText3);
	setting->Add(music);
	setting->Add(sound);
	setting->Add(musicOnOff);
	setting->Add(soundOnOff);
	m_windowObjects.push_back(move(setting));
}

void MainScene::CreateManualWindow()
{
	auto title{ make_unique<TextObject>() };
	title->SetBrush("BLACK");
	title->SetFormat("36R");
	title->SetText(TEXT("조작법"));
	title->SetPivot(ePivot::LEFTCENTER);
	title->SetScreenPivot(ePivot::LEFTTOP);
	title->SetPosition(XMFLOAT2{ 0.0f, title->GetHeight() / 2.0f + 2.0f });

	auto close{ make_unique<MenuTextObject>() };
	close->SetBrush("BLACK");
	close->SetMouseOverBrush("BLUE");
	close->SetFormat("36R");
	close->SetText(TEXT("확인"));
	close->SetScreenPivot(ePivot::CENTERBOT);
	close->SetPivot(ePivot::CENTERBOT);
	close->SetPosition(XMFLOAT2{ 0.0f, -close->GetHeight() / 2.0f + 10.0f });
	close->SetMouseClickCallBack(
		[&]()
		{
			m_windowObjects.back()->Delete();
		});
	
	auto manual{ make_unique<UIObject>(628.2f, 301.2f) };
	manual->SetPosition(XMFLOAT2{ 0.0f, close->GetHeight() / 2.0f });
	manual->SetTexture(s_textures["MANUAL"]);

	auto window{ make_unique<WindowObject>(800.0f, 500.0f) };
	window->SetTexture(s_textures["WHITE"]);
	window->SetPosition(XMFLOAT2{});
	window->Add(title);
	window->Add(close);
	window->Add(manual);
	m_windowObjects.push_back(move(window));
}

void MainScene::UpdateCameraPosition(FLOAT deltaTime)
{
	constexpr XMFLOAT3 target{ 0.0f, 0.0f, 0.0f };
	constexpr float speed{ 5.0f };
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

void MainScene::RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	if (!s_textures.contains("SHADOW")) return;
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
	for (const auto& o : m_gameObjects)
	{
		auto shadowShader{ o->GetShadowShader() };
		if (shadowShader)
			o->Render(commandList, shadowShader);
	}
	for (const auto& p : m_players)
		p->RenderToShadowMap(commandList);

	// 리소스배리어 설정(셰이더에서 읽기)
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadowTexture->GetBuffer().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}