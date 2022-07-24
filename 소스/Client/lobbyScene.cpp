#include "stdafx.h"
#include "lobbyScene.h"
#include "camera.h"
#include "framework.h"
#include "object.h"
#include "player.h"
#include "textObject.h"
#include "texture.h"
#include "uiObject.h"
#include "windowObject.h"

LobbyScene::LobbyScene() : m_isReadyToPlay{ FALSE }, m_isLogout{ FALSE },
						   m_leftSlotPlayerId{ -1 }, m_rightSlotPlayerId{ -1 }, m_leftSlotReadyText{ nullptr }, m_rightSlotReadyText{ nullptr }, m_pcbGameScene{ nullptr }
{

}

LobbyScene::~LobbyScene()
{
	if (m_cbGameScene) m_cbGameScene->Unmap(0, NULL);
#ifdef NETWORK
	if (m_isReadyToPlay)
	{
		// 모두 준비완료가 되서 게임 씬으로 넘어가는 경우
		if (g_networkThread.joinable())
			g_networkThread.join();
	}
	else
	{
		// 나가기 버튼을 누른 경우
		cs_packet_logout packet{};
		packet.size = sizeof(packet);
		packet.type = CS_PACKET_LOGOUT;
		send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), NULL);

		if (g_networkThread.joinable())
			g_networkThread.join();
		closesocket(g_socket);
		WSACleanup();
	}
#endif
}

void LobbyScene::OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
						const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature,
						const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	CreateShaderVariable(device, commandList);
	CreateGameObjects(device, commandList);
	CreateTextObjects(d2dDeivceContext, dWriteFactory);
	CreateLights();
	LoadMapObjects(device, commandList, Utile::PATH("map.txt"));

#ifdef NETWORK
	g_networkThread = thread{ &LobbyScene::RecvPacket, this };
#endif
}

void LobbyScene::OnDestroy()
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
	for (auto& o : m_gameObjects)
		o->Update(deltaTime);
	for (auto& ui : m_uiObjects)
		ui->Update(deltaTime);
	for (auto& t : m_textObjects)
		t->Update(deltaTime);
	for (auto& w : m_windowObjects)
		w->Update(deltaTime);

	unique_lock<mutex> lock{ g_mutex };
	for (auto& p : m_multiPlayers)
		if (p) p->Update(deltaTime);
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
	for (const auto& o : m_gameObjects)
		o->Render(commandList);
	for (const auto& p : m_multiPlayers)
		if (p) p->Render(commandList);
	if (m_player) m_player->Render(commandList);
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

void LobbyScene::Render2D(const ComPtr<ID2D1DeviceContext2>& device) const
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

	while (!m_isReadyToPlay && !m_isLogout)
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
	XMFLOAT4X4 projMatrix{};
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(g_width) / static_cast<float>(g_height), 1.0f, 2500.0f));
	m_camera = make_shared<Camera>();
	m_camera->CreateShaderVariable(device, commandList);
	m_camera->SetEye(XMFLOAT3{ 0.0f, 35.0f, 50.0f });
	m_camera->SetAt(Vector3::Normalize(Vector3::Sub(XMFLOAT3{ 0.0f, 20.0f, 0.0f }, m_camera->GetEye())));
	m_camera->SetProjMatrix(projMatrix);

	// 스카이박스
	m_skybox = make_unique<Skybox>();
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
	// 평범하게 렌더링하기 위해 멀티플레이어로 만듦
	m_player = make_unique<Player>(TRUE);
	m_player->SetWeaponType(eWeaponType::AR);
	m_player->PlayAnimation("RELOAD");
	m_player->SetCamera(m_camera.get());
}

void LobbyScene::CreateTextObjects(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	constexpr auto readyText{ TEXT("준비") };
	constexpr auto readyOkText{ TEXT("준비 완료") };

	constexpr auto readyCallBack = 
	[](MenuTextObject* readyTextObject)
	{
#ifdef NETWORK
		cs_packet_ready packet{};
		packet.size = sizeof(packet);
		packet.type = CS_PACKET_READY;

		wstring text{ readyTextObject->GetText() };
		if (text == readyText)
		{
			readyTextObject->SetBrush("BLUE");
			readyTextObject->SetMouseOverBrush("BLUE");
			readyTextObject->SetText(readyOkText);
			packet.isReady = true;
		}
		else
		{
			readyTextObject->SetBrush("BLACK");
			readyTextObject->SetMouseOverBrush("BLACK");
			readyTextObject->SetText(readyText);
			packet.isReady = false;
		}
		readyTextObject->SetPosition(XMFLOAT2{ 0.0f, -30.0f });
		send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), NULL);
#else
		g_gameFramework.SetNextScene(eSceneType::GAME);
#endif
	};

	auto leftCallBack =
	[&](MenuTextObject* readyTextObject)
	{
		if (readyTextObject->GetText() == readyOkText)
			return;

		eWeaponType type{ m_player->GetWeaponType() };
		if (type == eWeaponType::AR) type = eWeaponType::MG;
		else if (type == eWeaponType::SG) type = eWeaponType::AR;
		else if (type == eWeaponType::MG) type = eWeaponType::SG;
		m_player->SetWeaponType(type);
		m_player->PlayAnimation("IDLE");
		m_player->PlayAnimation("RELOAD");

		cs_packet_select_weapon packet{};
		packet.size = sizeof(packet);
		packet.type = CS_PACKET_SELECT_WEAPON;
		packet.weaponType = type;
		send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), NULL);
	};

	auto rightCallBack =
	[&](MenuTextObject* readyTextObject)
	{
		if (readyTextObject->GetText() == readyOkText)
			return;

		eWeaponType type{ m_player->GetWeaponType() };
		if (type == eWeaponType::AR) type = eWeaponType::SG;
		else if (type == eWeaponType::SG) type = eWeaponType::MG;
		else if (type == eWeaponType::MG) type = eWeaponType::AR;
		m_player->SetWeaponType(type);
		m_player->PlayAnimation("IDLE");
		m_player->PlayAnimation("RELOAD");

		cs_packet_select_weapon packet{};
		packet.size = sizeof(packet);
		packet.type = CS_PACKET_SELECT_WEAPON;
		packet.weaponType = type;
		send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), NULL);
	};

	auto ready{ make_unique<MenuTextObject>() };
	ready->SetBrush("BLACK");
	ready->SetMouseOverBrush("BLACK");
	ready->SetFormat("48R");
	ready->SetText(TEXT("준비"));
	ready->SetPivot(ePivot::CENTERBOT);
	ready->SetScreenPivot(ePivot::CENTERBOT);
	ready->SetPosition(XMFLOAT2{ 0.0f, -30.0f });
	ready->SetMouseClickCallBack(bind(readyCallBack, ready.get()));
	
	auto leftText{ make_unique<MenuTextObject>() };
	leftText->SetBrush("BLACK");
	leftText->SetMouseOverBrush("BLUE");
	leftText->SetFormat("48R");
	leftText->SetText(TEXT("<"));
	leftText->SetPivot(ePivot::CENTERBOT);
	leftText->SetScreenPivot(ePivot::CENTERBOT);
	leftText->SetPosition(XMFLOAT2{ -120.0f, -30.0f });
	leftText->SetMouseClickCallBack(bind(leftCallBack, ready.get()));
	m_textObjects.push_back(move(leftText));

	auto rightText{ make_unique<MenuTextObject>() };
	rightText->SetBrush("BLACK");
	rightText->SetMouseOverBrush("BLUE");
	rightText->SetFormat("48R");
	rightText->SetText(TEXT(">"));
	rightText->SetPivot(ePivot::CENTERBOT);
	rightText->SetScreenPivot(ePivot::CENTERBOT);
	rightText->SetPosition(XMFLOAT2{ 120.0f, -30.0f });
	rightText->SetMouseClickCallBack(bind(rightCallBack, ready.get()));
	m_textObjects.push_back(move(rightText));
	m_textObjects.push_back(move(ready));

	auto exit{ make_unique<MenuTextObject>() };
	exit->SetBrush("RED");
	exit->SetMouseOverBrush("RED");
	exit->SetFormat("48R");
	exit->SetText(TEXT("나가기"));
	exit->SetPivot(ePivot::RIGHTBOT);
	exit->SetScreenPivot(ePivot::RIGHTBOT);
	exit->SetPosition(XMFLOAT2{ -50.0f, -30.0f });
	exit->SetMouseClickCallBack(
		[]()
		{
			g_gameFramework.SetNextScene(eSceneType::MAIN);
		}
	);
	m_textObjects.push_back(move(exit));

	auto leftReadyText{ make_unique<TextObject>() };
	leftReadyText->SetBrush("BLACK");
	leftReadyText->SetFormat("48R");
	leftReadyText->SetText(TEXT("대기중"));
	leftReadyText->SetPivot(ePivot::CENTERBOT);
	leftReadyText->SetScreenPivot(ePivot::CENTERBOT);
	leftReadyText->SetPosition(XMFLOAT2{ -0.25f * g_width, -0.13f * g_height });
	m_leftSlotReadyText = leftReadyText.get();
	m_textObjects.push_back(move(leftReadyText));

	auto rightReadyText{ make_unique<TextObject>() };
	rightReadyText->SetBrush("BLACK");
	rightReadyText->SetFormat("48R");
	rightReadyText->SetText(TEXT("대기중"));
	rightReadyText->SetPivot(ePivot::CENTERBOT);
	rightReadyText->SetScreenPivot(ePivot::CENTERBOT);
	rightReadyText->SetPosition(XMFLOAT2{ 0.25f * g_width, -0.13f * g_height });
	m_rightSlotReadyText = rightReadyText.get();
	m_textObjects.push_back(move(rightReadyText));
}

void LobbyScene::CreateLights() const
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

void LobbyScene::LoadMapObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& mapFile)
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

void LobbyScene::CloseWindow()
{
	if (!m_windowObjects.empty())
		m_windowObjects.pop_back();
}

void LobbyScene::Update(FLOAT deltaTime)
{
	erase_if(m_windowObjects, [](unique_ptr<WindowObject>& object) { return !object->isValid(); });
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
	for (const auto& p : m_multiPlayers)
		if (p) p->RenderToShadowMap(commandList);
	if (m_player)
		m_player->RenderToShadowMap(commandList);

	// 리소스배리어 설정(셰이더에서 읽기)
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(shadowTexture->GetBuffer().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void LobbyScene::ProcessPacket()
{
	char buf[1 + 1]{};
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD recvByte{ 0 }, recvFlag{ 0 };
	if (WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr) == SOCKET_ERROR)
		error_display("RecvSizeType");

	UCHAR size{ static_cast<UCHAR>(buf[0]) };
	UCHAR type{ static_cast<UCHAR>(buf[1]) };
	switch (type)
	{
	case SC_PACKET_LOGIN_CONFIRM:
		RecvLoginOkPacket();
		break;
	case SC_PACKET_SELECT_WEAPON:
		RecvSelectWeaponPacket();
		break;
	case SC_PACKET_READY_CHECK:
		RecvReadyPacket();
		break;
	case SC_PACKET_CHANGE_SCENE:
		RecvChangeScenePacket();
		break;
	case SC_PACKET_LOGOUT_OK:
		RecvLogoutOkPacket();
		return;
	}
}

void LobbyScene::RecvLoginOkPacket()
{
	// 플레이어정보 + 닉네임 + 준비 상태 + 무기 종류
	char buf[sizeof(PlayerData) + MAX_NAME_SIZE + 1 + 1]{};
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	if (!m_player) return;

	PlayerData data{};
	char name[MAX_NAME_SIZE]{};
	memcpy(&data, buf, sizeof(data));
	memcpy(&name, &buf[sizeof(PlayerData)], sizeof(name));

	// 처음 들어왔을때 다른 플레이어들의 준비 상태와 무기 종류를 알 수 있게함
	bool isReady = buf[sizeof(PlayerData) + MAX_NAME_SIZE];
	auto weaponType = static_cast<eWeaponType>(buf[sizeof(PlayerData) + MAX_NAME_SIZE + 1]);
	
	if (m_player->GetId() == -1)
	{
		m_player->SetId(static_cast<int>(data.id));
		return;
	}
	if (m_player->GetId() == data.id)
		return;

	// 멀티플레이어가 들어왔으면 왼쪽, 오른쪽에 위치시킴
	for (auto& p : m_multiPlayers)
	{
		if (p) continue;
		p = make_shared<Player>(TRUE);
		p->SetId(static_cast<int>(data.id));
		p->SetWeaponType(weaponType);
		p->PlayAnimation("IDLE");
		if (m_leftSlotPlayerId == -1)
		{
			p->Move(XMFLOAT3{ 25.0f, 0.0f, -20.0f });
			m_leftSlotPlayerId = static_cast<int>(data.id);
			if (isReady)
			{
				m_leftSlotReadyText->SetBrush("BLUE");
				m_leftSlotReadyText->SetText(TEXT("준비완료"));
			}
			else
			{
				m_leftSlotReadyText->SetBrush("BLACK");
				m_leftSlotReadyText->SetText(TEXT("준비중"));
			}
			m_leftSlotReadyText->SetPosition(m_leftSlotReadyText->GetPivotPosition());
		}
		else if (m_rightSlotPlayerId == -1)
		{
			p->Move(XMFLOAT3{ -25.0f, 0.0f, -20.0f });
			m_rightSlotPlayerId = static_cast<int>(data.id);
			if (isReady)
			{
				m_rightSlotReadyText->SetBrush("BLUE");
				m_rightSlotReadyText->SetText(TEXT("준비완료"));
			}
			else
			{
				m_rightSlotReadyText->SetBrush("BLACK");
				m_rightSlotReadyText->SetText(TEXT("준비중"));
			}
			m_rightSlotReadyText->SetPosition(m_rightSlotReadyText->GetPivotPosition());
		}
		break;
	}
}

void LobbyScene::RecvReadyPacket()
{
	// 아이디, 준비상태
	char buf[1 + 1]{};
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	int id{ static_cast<int>(buf[0]) };
	bool state{ static_cast<bool>(buf[1]) };

	if (m_leftSlotPlayerId == id)
	{
		if (state)
		{
			m_leftSlotReadyText->SetBrush("BLUE");
			m_leftSlotReadyText->SetText(TEXT("준비완료"));
		}
		else
		{
			m_leftSlotReadyText->SetBrush("BLACK");
			m_leftSlotReadyText->SetText(TEXT("준비중"));
		}
		m_leftSlotReadyText->SetPosition(m_leftSlotReadyText->GetPivotPosition());
	}
	else if (m_rightSlotPlayerId == id)
	{
		if (state)
		{
			m_rightSlotReadyText->SetBrush("BLUE");
			m_rightSlotReadyText->SetText(TEXT("준비완료"));
		}
		else
		{
			m_rightSlotReadyText->SetBrush("BLACK");
			m_rightSlotReadyText->SetText(TEXT("준비중"));
		}
		m_rightSlotReadyText->SetPosition(m_rightSlotReadyText->GetPivotPosition());
	}
}

void LobbyScene::RecvChangeScenePacket()
{
	// 씬 타입
	char buf{};
	WSABUF wsabuf{ sizeof(buf), &buf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);
	
	// 타입은 사용하지 않음.
	// 이 패킷이 왔다는 것은 모든 플레이어가 준비완료 했다는 것
	eSceneType type{ static_cast<eSceneType>(buf) };

	m_isReadyToPlay = TRUE;
	g_gameFramework.SetNextScene(eSceneType::GAME);
}

void LobbyScene::RecvSelectWeaponPacket()
{
	// 아이디, 무기타입
	char buf[1 + 1]{};
	WSABUF wsabuf{ sizeof(buf), buf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	int id{ static_cast<int>(buf[0]) };
	eWeaponType type{ static_cast<eWeaponType>(buf[1]) };

	for (auto& p : m_multiPlayers)
	{
		if (!p) continue;
		if (p->GetId() != id) continue;

		switch (type)
		{
		case eWeaponType::AR:
			p->SetGunMesh(s_meshes["AR"]);
			p->SetWeaponType(eWeaponType::AR);
			break;
		case eWeaponType::SG:
			p->SetGunMesh(s_meshes["SG"]);
			p->SetWeaponType(eWeaponType::SG);
			break;
		case eWeaponType::MG:
			p->SetGunMesh(s_meshes["MG"]);
			p->SetWeaponType(eWeaponType::MG);
			break;
		}
		p->PlayAnimation("RELOAD");
		break;
	}
}

void LobbyScene::RecvLogoutOkPacket()
{
	char buf{};
	WSABUF wsabuf{ sizeof(buf), &buf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	int id{ static_cast<int>(buf) };
	if (m_player->GetId() == id)
	{
		m_isLogout = TRUE;
		return;
	}

	for (auto& p : m_multiPlayers)
	{
		if (!p) continue;
		if (p->GetId() != id) continue;

		if (m_leftSlotPlayerId == id)
		{
			m_leftSlotPlayerId = -1;
			m_leftSlotReadyText->SetBrush("BLACK");
			m_leftSlotReadyText->SetText(TEXT("대기중"));
			m_leftSlotReadyText->SetPosition(m_leftSlotReadyText->GetPivotPosition());
		}
		else if (m_rightSlotPlayerId == id)
		{
			m_rightSlotPlayerId = -1;
			m_rightSlotReadyText->SetBrush("BLACK");
			m_rightSlotReadyText->SetText(TEXT("대기중"));
			m_rightSlotReadyText->SetPosition(m_rightSlotReadyText->GetPivotPosition());
		}

		unique_lock<mutex> lock{ g_mutex };
		p.reset();
		return;
	}
}

unique_ptr<Player>& LobbyScene::GetPlayer()
{
	return m_player;
}

array<shared_ptr<Player>, Setting::MAX_PLAYERS>& LobbyScene::GetMultiPlayers()
{
	return m_multiPlayers;
}
