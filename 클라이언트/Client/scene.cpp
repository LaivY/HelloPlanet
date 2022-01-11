#include "scene.h"

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
	CreateLightAndMeterial();

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
	
	if (m_camera) m_camera->Rotate(0.0f, dy * sensitive * deltaTime, dx * sensitive * deltaTime);
	//if (m_player) m_player->Rotate(0.0f, dy * sensitive * deltaTime, dx * sensitive * deltaTime);

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
	if (GetAsyncKeyState('W') & 0x8000)
	{
		m_camera->Move(m_camera->GetAt());
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		XMFLOAT3 right{ Vector3::Cross(m_camera->GetUp(), m_camera->GetAt()) };
		m_camera->Move(Vector3::Mul(right, -1));
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		m_camera->Move(Vector3::Mul(m_camera->GetAt(), -1));
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		XMFLOAT3 right{ Vector3::Cross(m_camera->GetUp(), m_camera->GetAt()) };
		m_camera->Move(right);
	}
	if (GetAsyncKeyState(' ') & 0x8000)
	{
		m_camera->Move(XMFLOAT3{ 0.0f, 1.0f, 0.0f });
	}
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
	{
		m_camera->Move(XMFLOAT3{ 0.0f, -1.0f, 0.0f });
	}
}

void Scene::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
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
	m_meshes["PLAYER"]->LoadMesh(device, commandList, PATH("Player.txt"));
	m_meshes["PLAYER"]->LoadAnimation(device, commandList, PATH("Player_Reload.txt"), "RELOAD");
	m_meshes["PLAYER"]->LoadAnimation(device, commandList, PATH("Player_Run.txt"), "RUN");
}

void Scene::CreateShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature)
{
	m_shaders["DEFAULT"] = make_shared<Shader>(device, rootSignature);
	//m_shaders["TEXTURE"] = make_shared<TextureShader>(device, rootSignature);
	//m_shaders["SKYBOX"] = make_shared<SkyboxShader>(device, rootSignature);
	//m_shaders["TERRAIN"] = make_shared<TerrainShader>(device, rootSignature);
	//m_shaders["TERRAINTESS"] = make_shared<TerrainTessShader>(device, rootSignature);
	//m_shaders["TERRAINTESSWIRE"] = make_shared<TerrainTessWireShader>(device, rootSignature);
	//m_shaders["BLENDING"] = make_shared<BlendingShader>(device, rootSignature);
	//m_shaders["BLENDINGDEPTH"] = make_shared<BlendingDepthShader>(device, rootSignature);
	//m_shaders["STENCIL"] = make_shared<StencilShader>(device, rootSignature);
	//m_shaders["MIRROR"] = make_shared<MirrorShader>(device, rootSignature);
	//m_shaders["MIRRORTEXTURE"] = make_shared<MirrorTextureShader>(device, rootSignature);
	//m_shaders["MODEL"] = make_shared<ModelShader>(device, rootSignature);
	//m_shaders["SHADOW"] = make_shared<ShadowShader>(device, rootSignature);
	//m_shaders["HORZBLUR"] = make_shared<HorzBlurShader>(device, postProcessRootSignature);
	//m_shaders["VERTBLUR"] = make_shared<VertBlurShader>(device, postProcessRootSignature);
	//m_shaders["STREAM"] = make_shared<StreamShader>(device, rootSignature);
}

void Scene::CreateTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{

}

void Scene::CreateLightAndMeterial()
{
	// 씬 전체를 감싸는 조명
	m_cbSceneData->ligths[0].isActivate = true;
	m_cbSceneData->ligths[0].type = DIRECTIONAL_LIGHT;
	m_cbSceneData->ligths[0].strength = XMFLOAT3{ 1.0f, 1.0f, 1.0f };
	m_cbSceneData->ligths[0].direction = XMFLOAT3{ 1.0f, -1.0f, 1.0f };

	// 탱크 재질
	m_cbSceneData->materials[0].diffuseAlbedo = XMFLOAT4{ 0.1f, 0.1f, 0.1f, 1.0f };
	m_cbSceneData->materials[0].fresnelR0 = XMFLOAT3{ 0.95f, 0.93f, 0.88f };
	m_cbSceneData->materials[0].roughness = 0.05f;

	// 지형 재질
	m_cbSceneData->materials[1].diffuseAlbedo = XMFLOAT4{ 0.1f, 0.1f, 0.1f, 1.0f };
	m_cbSceneData->materials[1].fresnelR0 = XMFLOAT3{ 0.0f, 0.0f, 0.0f };
	m_cbSceneData->materials[1].roughness = 0.95f;
}

void Scene::CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 카메라 생성
	auto camera{ make_shared<Camera>() };
	camera->CreateShaderVariable(device, commandList);
	SetCamera(camera);

	// 카메라 투영 행렬 설정
	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), 1.0f, 5000.0f));
	camera->SetProjMatrix(projMatrix);

	// 플레이어 생성
	auto player{ make_shared<Player>() };
	player->SetMesh(m_meshes.at("PLAYER"));
	player->SetShader(m_shaders.at("DEFAULT"));
	player->PlayAnimation("RUN");
	SetPlayer(player);

	// 카메라, 플레이어 서로 설정
	camera->SetPlayer(player);
	player->SetCamera(camera);
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
	//RemoveDeletedObjects();
	//UpdateObjectsTerrain();
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
	// 씬을 모두 감싸는 바운딩 구
	BoundingSphere sceneSphere{ XMFLOAT3{ 0.0f, 0.0f, 0.0f }, 128.5f * sqrt(2.0f) };

	// 메쉬 정점을 조명 좌표계로 바꿔주는 행렬 계산
	XMFLOAT3 lightDir{ 1.0f, -1.0f, 1.0f };
	XMFLOAT3 lightPos{ Vector3::Mul(lightDir, -2.0f * sceneSphere.Radius) };
	XMFLOAT3 targetPos{ sceneSphere.Center };
	XMFLOAT3 lightUp{ 0.0f, 1.0f, 0.0f };

	XMFLOAT4X4 lightViewMatrix;
	XMStoreFloat4x4(&lightViewMatrix, XMMatrixLookAtLH(XMLoadFloat3(&lightPos), XMLoadFloat3(&targetPos), XMLoadFloat3(&lightUp)));

	// 조명 좌표계에서의 씬 구 원점
	XMFLOAT3 sphereCenterLS{ Vector3::TransformCoord(targetPos, lightViewMatrix) };

	// 직교 투영 행렬 설정
	float l = sphereCenterLS.x - sceneSphere.Radius;
	float b = sphereCenterLS.y - sceneSphere.Radius;
	float n = sphereCenterLS.z - sceneSphere.Radius;
	float r = sphereCenterLS.x + sceneSphere.Radius;
	float t = sphereCenterLS.y + sceneSphere.Radius;
	float f = sphereCenterLS.z + sceneSphere.Radius;
	XMFLOAT4X4 lightProjMatrix;
	XMStoreFloat4x4(&lightProjMatrix, XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f));

	// 상수 버퍼 데이터에 저장
	m_cbSceneData->lightViewMatrix = Matrix::Transpose(lightViewMatrix);
	m_cbSceneData->lightProjMatrix = Matrix::Transpose(lightProjMatrix);
	m_cbSceneData->NDCToTextureMatrix = Matrix::Transpose(
	{
		0.5f,  0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f,  0.0f, 1.0f, 0.0f,
		0.5f,  0.5f, 0.0f, 1.0f
	});

	// 셰이더로 복사
	memcpy(m_pcbScene, m_cbSceneData.get(), sizeof(cbScene));
	commandList->SetGraphicsRootConstantBufferView(3, m_cbScene->GetGPUVirtualAddress());

	// 카메라 셰이더 변수 최신화
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
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