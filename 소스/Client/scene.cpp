#include "scene.h"

Scene::Scene()
	: m_viewport{ 0.0f, 0.0f, static_cast<float>(Setting::SCREEN_WIDTH), static_cast<float>(Setting::SCREEN_HEIGHT), 0.0f, 1.0f },
	  m_scissorRect{ 0, 0, Setting::SCREEN_WIDTH, Setting::SCREEN_HEIGHT },
	  m_pcbScene{ nullptr }
{

}

Scene::~Scene()
{
	if (m_cbScene) m_cbScene->Unmap(0, NULL);
}

void Scene::OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
				   const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature)
{
	// 셰이더 변수 생성
	CreateShaderVariable(device, commandList);

	// 메쉬 생성
	CreateMeshes(device, commandList);

	// 셰이더 생성
	CreateShaders(device, rootSignature, postProcessRootSignature);

	// 텍스쳐 생성
	CreateTextures(device, commandList);

	// 그림자맵 생성
	//m_shadowMap = make_unique<ShadowMap>(device, 1024 * 16, 1024 * 16);

	// 블러 필터 생성
	//m_blurFilter = make_unique<BlurFilter>(device);

	// 조명, 재질 생성
	CreateLights();

	// 게임오브젝트 생성
	CreateGameObjects(device, commandList);
}

void Scene::OnMouseEvent(HWND hWnd, UINT width, UINT height, FLOAT deltaTime)
{
	// 화면 가운데 좌표 계산
	RECT rect; GetWindowRect(hWnd, &rect);
	POINT oldMousePosition{ rect.left + width / 2, rect.top + height / 2 };

	// 움직인 마우스 좌표
	POINT newMousePosition; GetCursorPos(&newMousePosition);

	// 움직인 정도에 비례해서 회전
	int dx = newMousePosition.x - oldMousePosition.x;
	int dy = newMousePosition.y - oldMousePosition.y;
	float sensitive{ 2.5f };
	
#ifdef FREEVIEW
	if (m_camera) m_camera->Rotate(0.0f, dy * sensitive * deltaTime, dx * sensitive * deltaTime);
#endif
#ifdef FIRSTVIEW
	if (m_camera) m_camera->Rotate(0.0f, dy * sensitive * deltaTime, dx * sensitive * deltaTime);
#endif
#ifdef THIRDVIEW
	if (m_player) m_player->Rotate(0.0f, dy * sensitive * deltaTime, dx * sensitive * deltaTime);
#endif

	// 마우스를 화면 가운데로 이동
	SetCursorPos(oldMousePosition.x, oldMousePosition.y);
}

void Scene::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (m_camera) m_camera->OnMouseEvent(hWnd, message, wParam, lParam);
	if (m_player) m_player->OnMouseEvent(hWnd, message, wParam, lParam);
}

void Scene::OnKeyboardEvent(FLOAT deltaTime)
{
#ifdef FREEVIEW
	const float speed{ 10.0f * deltaTime };
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

void Scene::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (m_player) m_player->OnKeyboardEvent(hWnd, message, wParam, lParam);
	if (wParam == VK_ESCAPE)
	{
		exit(0);
	}
}

void Scene::OnUpdate(FLOAT deltaTime)
{
	Update(deltaTime);
	if (m_player) m_player->Update(deltaTime);
	if (m_camera) m_camera->Update(deltaTime);
	if (m_skybox) m_skybox->Update(deltaTime);
	for (auto& p : m_multiPlayers)
		if (p) p->Update(deltaTime);
	for (auto& o : m_gameObjects)
		o->Update(deltaTime);
}

void Scene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_cbScene = Utile::CreateBufferResource(device, commandList, NULL, Utile::GetConstantBufferSize<cbScene>(), 1, D3D12_HEAP_TYPE_UPLOAD, {});
	m_cbScene->Map(0, NULL, reinterpret_cast<void**>(&m_pcbScene));
	m_cbSceneData = make_unique<cbScene>();
}

void Scene::CreateMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	vector<pair<string, string>> animations
	{
		{ "idle", "IDLE" }, { "walking", "WALKING" }, {"walkLeft", "WALKLEFT" }, { "walkRight", "WALKRIGHT" },
		{ "walkBack", "WALKBACK" }, {"running", "RUNNING" }, {"firing", "FIRING" }, { "reload", "RELOAD" }
	};

	m_meshes["PLAYER"] = make_shared<Mesh>();
	m_meshes["PLAYER"]->LoadMesh(device, commandList, Utile::PATH("player.txt", Utile::RESOURCE));
	for (const string& weaponName : { "AR", "SG", "MG" })
		for (const auto& [fileName, animationName] : animations)
			m_meshes["PLAYER"]->LoadAnimation(device, commandList, Utile::PATH(weaponName + "/" + fileName + ".txt", Utile::RESOURCE), weaponName + "/" + animationName);

	m_meshes["AR"] = make_shared<Mesh>();
	m_meshes["AR"]->LoadMesh(device, commandList, Utile::PATH("AR/AR.txt", Utile::RESOURCE));
	m_meshes["AR"]->Link(m_meshes["PLAYER"]);

	m_meshes["SG"] = make_shared<Mesh>();
	m_meshes["SG"]->LoadMesh(device, commandList, Utile::PATH("SG/SG.txt", Utile::RESOURCE));
	m_meshes["SG"]->Link(m_meshes["PLAYER"]);

	m_meshes["MG"] = make_shared<Mesh>();
	m_meshes["MG"]->LoadMesh(device, commandList, Utile::PATH("MG/MG.txt", Utile::RESOURCE));
	m_meshes["MG"]->Link(m_meshes["PLAYER"]);

	m_meshes["SKYBOX"] = make_shared<Mesh>();
	m_meshes["SKYBOX"]->LoadMesh(device, commandList, Utile::PATH("Skybox/Skybox.txt", Utile::RESOURCE));

	m_meshes["GAROO"] = make_shared<Mesh>();
	m_meshes["GAROO"]->LoadMesh(device, commandList, Utile::PATH("Mob/AlienGaroo/AlienGaroo.txt", Utile::RESOURCE));
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/attack.txt", Utile::RESOURCE), "ATTACK");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/die.txt", Utile::RESOURCE), "DIE");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/hit.txt", Utile::RESOURCE), "HIT");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/idle.txt", Utile::RESOURCE), "IDLE");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/run.txt", Utile::RESOURCE), "RUNNING");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/walkBack.txt", Utile::RESOURCE), "WALKBACK");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/walking.txt", Utile::RESOURCE), "WALKING");

	m_meshes["FLOOR"] = make_shared<RectMesh>(device, commandList, 1500.0f, 0.0f, 1500.0f);

	m_meshes["BB_PLAYER"] = make_shared<CubeMesh>(device, commandList, 8.0f, 32.5f, 8.0f, XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT4{ 0.0f, 0.8f, 0.0f, 1.0f });
	m_meshes["BB_MOB"] = make_shared<CubeMesh>(device, commandList, 7.0f, 7.0f, 10.0f, XMFLOAT3{ 0.0f, 8.0f, 0.0f }, XMFLOAT4{ 0.8f, 0.0f, 0.0f, 1.0f });
}

void Scene::CreateShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature)
{
	m_shaders["DEFAULT"] = make_shared<Shader>(device, rootSignature, Utile::PATH(TEXT("default.hlsl"), Utile::SHADER), "VS", "PS");
	m_shaders["SKYBOX"] = make_shared<SkyboxShader>(device, rootSignature, Utile::PATH(TEXT("model.hlsl"), Utile::SHADER), "VS", "PS");
	m_shaders["MODEL"] = make_shared<ModelShader>(device, rootSignature, Utile::PATH(TEXT("model.hlsl"), Utile::SHADER), "VS", "PS");
	m_shaders["ANIMATION"] = make_shared<AnimationShader>(device, rootSignature, Utile::PATH(TEXT("animation.hlsl"), Utile::SHADER), "VS", "PS");
	m_shaders["LINK"] = make_shared<LinkShader>(device, rootSignature, Utile::PATH(TEXT("link.hlsl"), Utile::SHADER), "VS", "PS");
	m_shaders["BOUNDINGBOX"] = make_shared<BoundingBoxShader>(device, rootSignature, Utile::PATH(TEXT("default.hlsl"), Utile::SHADER), "VS", "PS");
}

void Scene::CreateTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_textures["SKYBOX"] = make_shared<Texture>();
	m_textures["SKYBOX"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("Skybox/Skybox.dds"), Utile::RESOURCE));
	m_textures["SKYBOX"]->CreateTexture(device);

	m_textures["GAROO"] = make_shared<Texture>();
	m_textures["GAROO"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("Mob/AlienGaroo/texture.dds"), Utile::RESOURCE));
	m_textures["GAROO"]->CreateTexture(device);
}

void Scene::CreateLights()
{

}

void Scene::CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
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
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(Setting::SCREEN_WIDTH) / static_cast<float>(Setting::SCREEN_HEIGHT), 1.0f, 5000.0f));
	m_camera->SetProjMatrix(projMatrix);

	// 바운딩박스
	shared_ptr<DebugBoundingBox> bbPlayer{ make_unique<DebugBoundingBox>(XMFLOAT3{}, XMFLOAT3{}, XMFLOAT4{}) };
	bbPlayer->SetMesh(m_meshes["BB_PLAYER"]);
	bbPlayer->SetShader(m_shaders["BOUNDINGBOX"]);

	shared_ptr<DebugBoundingBox> bbMob{ make_unique<DebugBoundingBox>(XMFLOAT3{}, XMFLOAT3{}, XMFLOAT4{}) };
	bbMob->SetMesh(m_meshes["BB_MOB"]);
	bbMob->SetShader(m_shaders["BOUNDINGBOX"]);

	// 플레이어 생성
	m_player = make_shared<Player>();
#ifdef FIRSTVIEW
	m_player->SetMesh(m_meshes["SG"]);
	m_player->SetShader(m_shaders["LINK"]);
#else
	m_player->SetMesh(m_meshes["PLAYER"]);
	m_player->SetShader(m_shaders["ANIMATION"]);
	m_player->SetGunMesh(m_meshes["SG"]);
	m_player->SetGunShader(m_shaders["LINK"]);
#endif
	m_player->SetGunType(ePlayerGunType::SG);
	m_player->PlayAnimation("IDLE");
	m_player->SetBoundingBox(bbPlayer);

	// 카메라, 플레이어 서로 설정
	m_camera->SetPlayer(m_player);
	m_player->SetCamera(m_camera);

	// 멀티플레이어
	float pos{ 20.0f };
	for (unique_ptr<Player>& p : m_multiPlayers)
	{
		p = make_unique<Player>(TRUE);
		p->SetMesh(m_meshes["PLAYER"]);
		p->SetShader(m_shaders["ANIMATION"]);
		p->SetGunMesh(m_meshes["AR"]);
		p->SetGunShader(m_shaders["LINK"]);
		p->SetGunType(ePlayerGunType::AR);
		p->PlayAnimation("IDLE");
		p->SetPosition(XMFLOAT3{ 0.0f, 0.0f, pos });
		p->SetBoundingBox(bbPlayer);
		pos += 20.0f;
	}

	// 스카이박스
	m_skybox = make_unique<Skybox>();
	m_skybox->SetMesh(m_meshes["SKYBOX"]);
	m_skybox->SetShader(m_shaders["SKYBOX"]);
	m_skybox->SetTexture(m_textures["SKYBOX"]);
	m_skybox->SetCamera(m_camera);

	// 바닥
	auto floor{ make_unique<GameObject>() };
	floor->SetMesh(m_meshes["FLOOR"]);
	floor->SetShader(m_shaders["DEFAULT"]);
	m_gameObjects.push_back(move(floor));

	// 몬스터
	auto mob{ make_unique<GameObject>() };
	mob->SetMesh(m_meshes["GAROO"]);
	mob->SetShader(m_shaders["ANIMATION"]);
	mob->SetTexture(m_textures["GAROO"]);
	mob->Move(XMFLOAT3{ 15.0f, 0.0f, 15.0f });
	mob->PlayAnimation("ATTACK");
	mob->SetBoundingBox(bbMob);
	m_gameObjects.push_back(move(mob));
}

void Scene::ReleaseUploadBuffer()
{
	for (auto& [_, mesh] : m_meshes)
		mesh->ReleaseUploadBuffer();
	for (auto& [_, texture] : m_textures)
		texture->ReleaseUploadBuffer();
}

void Scene::Update(FLOAT deltaTime)
{
	UpdateLights(deltaTime);
}

void Scene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	// 뷰포트, 가위사각형, 렌더타겟 설정
	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);
	commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

	// 스카이박스 렌더링
	if (m_skybox) m_skybox->Render(commandList);

	// 플레이어 렌더링
	if (m_player) m_player->Render(commandList);

	// 멀티플레이어 렌더링
	for (const auto& p : m_multiPlayers)
		if (p) p->Render(commandList);

	// 게임오브젝트 렌더링
	for (const auto& o : m_gameObjects)
		o->Render(commandList);
}

void Scene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	// 씬 셰이더 변수 최신화
	memcpy(m_pcbScene, m_cbSceneData.get(), sizeof(cbScene));
	commandList->SetGraphicsRootConstantBufferView(3, m_cbScene->GetGPUVirtualAddress());

	// 카메라 셰이더 변수 최신화
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
}

void Scene::UpdateLights(FLOAT deltaTime)
{
	// 0 ~ 2pi
	static float t{ 0.0f };
	const float radius{ 50.0f };

	XMFLOAT3 position{ radius * cosf(t), 0.0f, radius * sinf(t) };
	XMFLOAT3 direction{ Vector3::Normalize(Vector3::Mul(position, -1)) };

	// 데이터 설정
	m_cbSceneData->ligths[0].position = position;
	m_cbSceneData->ligths[0].direction = direction;

	t += 0.25f * deltaTime;
	if (t >= 2.0f * 3.141592f)
		t = 0.0f;
}

void Scene::RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	if (!m_shadowMap) return;

	// 셰이더에 묶기
	ID3D12DescriptorHeap* ppHeaps[] = { m_shadowMap->GetSrvHeap().Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandList->SetGraphicsRootDescriptorTable(4, m_shadowMap->GetGpuSrvHandle());

	// 뷰포트, 가위사각형 설정
	commandList->RSSetViewports(1, &m_shadowMap->GetViewport());
	commandList->RSSetScissorRects(1, &m_shadowMap->GetScissorRect());

	// 리소스배리어 설정(깊이버퍼쓰기)
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMap->GetShadowMap().Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// 깊이스텐실 버퍼 초기화
	commandList->ClearDepthStencilView(m_shadowMap->GetCpuDsvHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	// 렌더타겟 설정
	commandList->OMSetRenderTargets(0, NULL, FALSE, &m_shadowMap->GetCpuDsvHandle());

	// 렌더링
	if (m_player) m_player->Render(commandList, m_shaders.at("SHADOW"));
	for (const auto& object : m_gameObjects)
		object->Render(commandList, m_shaders.at("SHADOW"));
	
	// 리소스배리어 설정(셰이더에서 읽기)
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMap->GetShadowMap().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Scene::ProcessClient(LPVOID arg)
{
	//auto pre_t = chrono::system_clock::now();
	while (g_isConnected)
	{
		/*auto cur_t = chrono::system_clock::now();
		float elapsed = chrono::duration_cast<chrono::milliseconds>(cur_t - pre_t).count() / float(1000);*/
		RecvPacket();
		/*pre_t = cur_t;
		if (chrono::system_clock::now() - pre_t < 32ms)
			this_thread::sleep_for(32ms - (chrono::system_clock::now() - cur_t));*/
	}
}

void Scene::RecvPacket()
{
	char recv_buf[5];
	WSABUF wsabuf;
	wsabuf.buf = recv_buf;
	wsabuf.len = sizeof(recv_buf);
	DWORD recv_byte;
	DWORD recv_flag = 0;
	int error_code = WSARecv(g_c_socket, &wsabuf, 1, &recv_byte, &recv_flag, 0, 0);
	if (error_code == SOCKET_ERROR) error_display("RecvSizeType");
	//cout << "[Packet recv] size: " << static_cast<int>(recv_buf[0]) << ", " << "type: " << static_cast<int>(recv_buf[1]) << endl;

	switch (recv_buf[1])
	{
	case SC_PACKET_LOGIN_OK: {
		sc_packet_login_ok packet;
		memcpy(reinterpret_cast<char*>(&packet), recv_buf, sizeof(recv_buf));
		/*int error_code = recv(g_c_socket, reinterpret_cast<char*>((&packet)) + 2, recv_size - 2, MSG_WAITALL);
		if (error_code == SOCKET_ERROR) error_display("RecvLOGIN");*/
		cout << "[Packet recv] size: " << static_cast<int>(packet.size) << ", " << "type: " << static_cast<int>(packet.type) << endl;
		cout << "Your ID is " << static_cast<int>(packet.data._id) << endl;
		break;
	}
	case SC_PACKET_UPDATE_CLIENT: {
		sc_packet_update_client packet;
		memcpy(reinterpret_cast<char*>(&packet), recv_buf, sizeof(recv_buf));
		/*int error_code = recv(g_c_socket, reinterpret_cast<char*>((&packet)) + 2, recv_size - 2, MSG_WAITALL);
		if (error_code == SOCKET_ERROR) error_display("RecvUPDATE");*/
		//cout << "[Packet recv] size: " << static_cast<int>(packet.size) << ", " << "type: " << static_cast<int>(packet.type) << endl;
		cout << "\rYour Legs State " << static_cast<int>(packet.data._state);
		break;
	}
	default:
		//char garbage[BUF_SIZE];
		//int error_code = recv(g_c_socket, garbage, recv_size - 2, MSG_WAITALL);
		//if (error_code == SOCKET_ERROR) error_display("RecvData");
		//cout << "Wrong Packet" << endl;
		break;
	}

	/*constexpr int head_size = 2;
	char head_buf[head_size];
	char net_buf[BUF_SIZE];
	
	int recv_result = recv(g_c_socket, head_buf, head_size, MSG_WAITALL);
	if (recv_result == SOCKET_ERROR)
	{
		error_display("recv()");
		while (1);
	}
	int packet_size = head_buf[0];
	int packet_type = head_buf[1];
	cout << "[packet]: " << packet_size << ", " << packet_type << endl;

	switch (packet_type)
	{
	case SC_PACKET_LOGIN_OK: 
	{
		sc_packet_login_ok recv_packet;
		recv_result += recv(g_c_socket, reinterpret_cast<char*>(&recv_packet) + head_size, packet_size - head_size, MSG_WAITALL);
		cout << "[SC_PACKET_LOGIN_OK] received" << endl;
		break;
	}
	case SC_PACKET_UPDATE_CLIENT: 
	{
		sc_packet_update_client recv_packet;
		recv_result += recv(g_c_socket, reinterpret_cast<char*>(&recv_packet) + head_size, packet_size - head_size, MSG_WAITALL);
		cout << "[SC_PACKET_UPDATE_CLIENT] received" << endl;
		break;
	}
	default: 
	{
		char garbage[20];
		recv_result += recv(g_c_socket, garbage, packet_size - 2, MSG_WAITALL);
		cout << "[Unknown_Packet] received" << endl;
		break;
	}
	}*/
}

void Scene::SendPacket(LPVOID lp_packet)
{
	const auto send_packet = static_cast<char*>(lp_packet);
	int send_result = send(g_c_socket, send_packet, *send_packet, 0);
	cout << "[Send_Packet] Type: " << *(send_packet + 1) << " , Byte: " << *send_packet << endl;
}