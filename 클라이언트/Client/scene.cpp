﻿#include "scene.h"
#define FREEVIEW

Scene::Scene()
	: m_viewport{ 0.0f, 0.0f, static_cast<float>(SCREEN_WIDTH), static_cast<float>(SCREEN_HEIGHT), 0.0f, 1.0f },
	  m_scissorRect{ 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT },
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
	//CreateTextures(device, commandList);

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
#else
	if (m_player) m_player->Rotate(0.0f, dy * sensitive * deltaTime, dx * sensitive * deltaTime);
#endif

	// 마우스를 화면 가운데로 이동
	SetCursorPos(oldMousePosition.x, oldMousePosition.y);
}

void Scene::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_MOUSEWHEEL:
	{
		if (!m_camera) break;
		ThirdPersonCamera* camera{ reinterpret_cast<ThirdPersonCamera*>(m_camera.get()) };
		if ((SHORT)HIWORD(wParam) > 0)
			camera->SetDistance(camera->GetDistance() - 1.0f);
		else
			camera->SetDistance(camera->GetDistance() + 1.0f);
		break;
	}
	}
}

void Scene::OnKeyboardEvent(FLOAT deltaTime)
{
#ifdef FREEVIEW
	static const float speed{ 10.0f * deltaTime };
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
#endif
}

void Scene::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (wParam == 'r' || wParam == 'R')
	{
		if (message == WM_KEYDOWN && m_player->GetAnimationInfo()->animationName != "RELOAD")
		{
			m_player->PlayAnimation("RELOAD");
		}
	}
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
	if (m_skybox) m_skybox->Update();
	for (auto& object : m_gameObjects)
		object->Update(deltaTime);
}

void Scene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	ComPtr<ID3D12Resource> dummy;
	UINT cbSceneByteSize{ (sizeof(cbScene) + 255) & ~255 };
	m_cbScene = CreateBufferResource(device, commandList, NULL, cbSceneByteSize, 1, D3D12_HEAP_TYPE_UPLOAD, {}, dummy);
	m_cbScene->Map(0, NULL, reinterpret_cast<void**>(&m_pcbScene));
	m_cbSceneData = make_unique<cbScene>();
}

void Scene::CreateMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_meshes["PLAYER"] = make_shared<Mesh>();
	m_meshes["PLAYER"]->LoadMesh(device, commandList, PATH("player.txt"));
	m_meshes["PLAYER"]->LoadAnimation(device, commandList, PATH("animation.txt"), "AIMING");

	m_meshes["GUN"] = make_shared<Mesh>();
	m_meshes["GUN"]->LoadMesh(device, commandList, PATH("gun.txt"));
	m_meshes["GUN"]->LoadAnimation(device, commandList, PATH("animation.txt"), "AIMING");

	m_meshes["FLOOR"] = make_shared<RectMesh>(device, commandList, 100.0f, 100.0f);
}

void Scene::CreateShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature)
{
	m_shaders["DEFAULT"] = make_shared<Shader>(device, rootSignature);
	m_shaders["MODEL"] = make_shared<ModelShader>(device, rootSignature);
	m_shaders["ANIMATION"] = make_shared<AnimationShader>(device, rootSignature);
	m_shaders["LINK"] = make_shared<LinkShader>(device, rootSignature);
}

void Scene::CreateTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{

}

void Scene::CreateLights()
{

}

void Scene::CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 카메라 생성
#ifdef FREEVIEW
	auto camera{ make_shared<Camera>() };
#else
	auto camera{ make_shared<ThirdPersonCamera>() };
#endif
	camera->CreateShaderVariable(device, commandList);
	SetCamera(camera);

	// 카메라 투영 행렬 설정
	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), 1.0f, 5000.0f));
	camera->SetProjMatrix(projMatrix);

	// 플레이어 생성
	auto player{ make_shared<Player>() };
	player->SetMesh(m_meshes.at("PLAYER"));
	player->SetShader(m_shaders.at("ANIMATION"));
	player->PlayAnimation("AIMING");
	SetPlayer(player);

	// 카메라, 플레이어 서로 설정
	camera->SetPlayer(player);
	player->SetCamera(camera);

	// 총
	auto gun{ make_unique<GameObject>() };
	gun->SetMesh(m_meshes.at("GUN"));
	gun->SetShader(m_shaders.at("LINK"));
	gun->PlayAnimation("AIMING");
	m_gameObjects.push_back(move(gun));

	// 바닥
	auto floor{ make_unique<GameObject>() };
	floor->Rotate(0.0f, 90.0f, 0.0f);
	floor->SetMesh(m_meshes.at("FLOOR"));
	floor->SetShader(m_shaders.at("DEFAULT"));
	m_gameObjects.push_back(move(floor));
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

	// 게임오브젝트 렌더링
	for (const auto& gameObject : m_gameObjects)
		gameObject->Render(commandList);
}

void Scene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	// 씬 셰이더 변수 최신화
	commandList->SetGraphicsRootConstantBufferView(3, m_cbScene->GetGPUVirtualAddress());

	// 카메라 셰이더 변수 최신화
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
}

void Scene::UpdateLights(FLOAT deltaTime)
{
	// 0 ~ 2pi
	static float t{ 0.0f };

	float radius{ 50.0f };
	XMFLOAT3 position{ radius * cosf(t), 0.0f, radius * sinf(t) };
	XMFLOAT3 direction{ Vector3::Normalize(Vector3::Mul(position, -1)) };

	// 데이터 설정
	m_cbSceneData->ligths[0].position = position;
	m_cbSceneData->ligths[0].direction = direction;
	memcpy(m_pcbScene, m_cbSceneData.get(), sizeof(cbScene));

	t += 0.25f * deltaTime;
	if (t >= 2.0f * 3.141592f)
	{
		t = 0.0f;
	}
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

void Scene::SetSkybox(unique_ptr<Skybox>& skybox)
{
	if (m_skybox) m_skybox.reset();
	m_skybox = move(skybox);
}

void Scene::SetPlayer(const shared_ptr<Player>& player)
{
	if (m_player) m_player.reset();
	m_player = player;
}

void Scene::SetCamera(const shared_ptr<Camera>& camera)
{
	if (m_camera) m_camera.reset();
	m_camera = camera;
}