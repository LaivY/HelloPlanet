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
	if (m_player) m_player->Rotate(0.0f, dy * sensitive * deltaTime, dx * sensitive * deltaTime);

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
	const int speed = 1.0f;
	if (GetAsyncKeyState('W') & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetFront(), speed * deltaTime));
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetLocalXAxis(), -speed * deltaTime));
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetFront(), -speed * deltaTime));
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		m_player->AddVelocity(Vector3::Mul(m_player->GetLocalXAxis(), speed * deltaTime));
	}
}

void Scene::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// 지형 와이어프레임 ON, OFF
	static bool drawAsWireframe{ false };
	if (wParam == 'q' || wParam == 'Q')
	{
		drawAsWireframe = !drawAsWireframe;
		for (auto& terrain : m_terrains)
			if (drawAsWireframe)
				terrain->SetShader(m_shaders.at("TERRAINTESSWIRE"));
			else
				terrain->SetShader(m_shaders.at("TERRAINTESS"));
	}

	// 실내로 이동
	else if (wParam == 'i' || wParam == 'I')
	{
		// right up look 초기화
		XMFLOAT3 rpy{ m_player->GetRollPitchYaw() };
		m_player->Rotate(-rpy.x, -rpy.y, -rpy.z);
		
		XMFLOAT4X4 worldMatrix{ Matrix::Identity() };
		worldMatrix._11 = 1.0f; worldMatrix._12 = 0.0f; worldMatrix._13 = 0.0f;
		worldMatrix._21 = 0.0f; worldMatrix._22 = 1.0f; worldMatrix._23 = 0.0f;
		worldMatrix._31 = 0.0f; worldMatrix._32 = 0.0f; worldMatrix._33 = 1.0f;
		m_player->SetWorldMatrix(worldMatrix);
		m_player->SetPosition({ 0.0f, 500.0f - 15.0f, 0.0f });
	}

	// 지형 위로 이동
	else if (wParam == 'e' || wParam == 'E')
	{
		m_player->SetPosition({ 0.0f, 0.0f, 0.0f });
	}

	// 종료
	else if (wParam == VK_ESCAPE)
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
	for (auto& particle : m_translucences)
		particle->Update(deltaTime);
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
	m_meshes["TANK"] = make_shared<Mesh>(device, commandList, sPATH("result.bin"));
	//m_meshes["CUBE"] = make_shared<CubeMesh>(device, commandList, 1.0f, 1.0f, 1.0f);
	//m_meshes["INDOOR"] = make_shared<ReverseCubeMesh>(device, commandList, 15.0f, 15.0f, 15.0f);
	//m_meshes["BULLET"] = make_shared<CubeMesh>(device, commandList, 0.1f, 0.1f, 0.1f);
	//m_meshes["EXPLOSION"] = make_shared<BillboardMesh>(device, commandList, XMFLOAT3{}, XMFLOAT2{ 5.0f, 5.0f });
	//m_meshes["SMOKE"] = make_shared<BillboardMesh>(device, commandList, XMFLOAT3{}, XMFLOAT2{ 5.0f, 5.0f });
	//m_meshes["MIRROR"] = make_shared<TextureRectMesh>(device, commandList, 15.0f, 0.0f, 15.0f, XMFLOAT3{ 0.0f, 0.0f, 0.1f });
	//m_meshes["PARTICLE"] = make_shared<ParticleMesh>(device, commandList, XMFLOAT2{ 0.05f, 5.0f });

	//m_meshes["SKYBOX_FRONT"] = make_shared<TextureRectMesh>(device, commandList, 20.0f, 0.0f, 20.0f, XMFLOAT3{ 0.0f, 0.0f, 10.0f });
	//m_meshes["SKYBOX_LEFT"] = make_shared<TextureRectMesh>(device, commandList, 0.0f, 20.0f, 20.0f, XMFLOAT3{ -10.0f, 0.0f, 0.0f });
	//m_meshes["SKYBOX_RIGHT"] = make_shared<TextureRectMesh>(device, commandList, 0.0f, 20.0f, 20.0f, XMFLOAT3{ 10.0f, 0.0f, 0.0f });
	//m_meshes["SKYBOX_BACK"] = make_shared<TextureRectMesh>(device, commandList, 20.0f, 0.0f, 20.0f, XMFLOAT3{ 0.0f, 0.0f, -10.0f });
	//m_meshes["SKYBOX_TOP"] = make_shared<TextureRectMesh>(device, commandList, 20.0f, 20.0f, 0.0f, XMFLOAT3{ 0.0f, 10.0f, 0.0f });
	//m_meshes["SKYBOX_BOT"] = make_shared<TextureRectMesh>(device, commandList, 20.0f, 20.0f, 0.0f, XMFLOAT3{ 0.0f, -10.0f, 0.0f });
}

void Scene::CreateShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature)
{
	m_shaders["COLOR"] = make_shared<Shader>(device, rootSignature);
	m_shaders["TEXTURE"] = make_shared<TextureShader>(device, rootSignature);
	m_shaders["SKYBOX"] = make_shared<SkyboxShader>(device, rootSignature);
	m_shaders["TERRAIN"] = make_shared<TerrainShader>(device, rootSignature);
	m_shaders["TERRAINTESS"] = make_shared<TerrainTessShader>(device, rootSignature);
	m_shaders["TERRAINTESSWIRE"] = make_shared<TerrainTessWireShader>(device, rootSignature);
	m_shaders["BLENDING"] = make_shared<BlendingShader>(device, rootSignature);
	m_shaders["BLENDINGDEPTH"] = make_shared<BlendingDepthShader>(device, rootSignature);
	m_shaders["STENCIL"] = make_shared<StencilShader>(device, rootSignature);
	m_shaders["MIRROR"] = make_shared<MirrorShader>(device, rootSignature);
	m_shaders["MIRRORTEXTURE"] = make_shared<MirrorTextureShader>(device, rootSignature);
	m_shaders["MODEL"] = make_shared<ModelShader>(device, rootSignature);
	m_shaders["SHADOW"] = make_shared<ShadowShader>(device, rootSignature);
	m_shaders["HORZBLUR"] = make_shared<HorzBlurShader>(device, postProcessRootSignature);
	m_shaders["VERTBLUR"] = make_shared<VertBlurShader>(device, postProcessRootSignature);
	m_shaders["STREAM"] = make_shared<StreamShader>(device, rootSignature);
}

void Scene::CreateTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	auto rockTexture{ make_shared<Texture>() };
	rockTexture->LoadTextureFile(device, commandList, 2, wPATH("Rock.dds"));
	rockTexture->CreateSrvDescriptorHeap(device);
	rockTexture->CreateShaderResourceView(device);
	m_textures["ROCK"] = rockTexture;

	auto terrainTexture{ make_shared<Texture>() };
	terrainTexture->LoadTextureFile(device, commandList, 2, wPATH("BaseTerrain.dds"));
	terrainTexture->LoadTextureFile(device, commandList, 3, wPATH("DetailTerrain.dds"));
	terrainTexture->CreateSrvDescriptorHeap(device);
	terrainTexture->CreateShaderResourceView(device);
	m_textures["TERRAIN"] = terrainTexture;

	auto explosionTexture{ make_shared<Texture>() };
	for (int i = 1; i <= 33; ++i)
		explosionTexture->LoadTextureFile(device, commandList, 2, wPATH("explosion (" + to_string(i) + ").dds"));
	explosionTexture->CreateSrvDescriptorHeap(device);
	explosionTexture->CreateShaderResourceView(device);
	m_textures["EXPLOSION"] = explosionTexture;

	auto smokeTexture{ make_shared<Texture>() };
	for (int i = 1; i <= 91; ++i)
		smokeTexture->LoadTextureFile(device, commandList, 2, wPATH("smoke (" + to_string(i) + ").dds"));
	smokeTexture->CreateSrvDescriptorHeap(device);
	smokeTexture->CreateShaderResourceView(device);
	m_textures["SMOKE"] = smokeTexture;

	auto indoorTexture{ make_shared<Texture>() };
	indoorTexture->LoadTextureFile(device, commandList, 2, wPATH("Wall.dds"));
	indoorTexture->CreateSrvDescriptorHeap(device);
	indoorTexture->CreateShaderResourceView(device);
	m_textures["INDOOR"] = indoorTexture;

	auto mirrorTexture{ make_shared<Texture>() };
	mirrorTexture->LoadTextureFile(device, commandList, 2, wPATH("Mirror.dds"));
	mirrorTexture->CreateSrvDescriptorHeap(device);
	mirrorTexture->CreateShaderResourceView(device);
	m_textures["MIRROR"] = mirrorTexture;

	auto skyboxFrontTexture{ make_shared<Texture>() };
	skyboxFrontTexture->LoadTextureFile(device, commandList, 2, wPATH("SkyboxFront.dds"));
	skyboxFrontTexture->CreateSrvDescriptorHeap(device);
	skyboxFrontTexture->CreateShaderResourceView(device);
	m_textures["SKYBOX_FRONT"] = skyboxFrontTexture;

	auto skyboxLeftTexture{ make_shared<Texture>() };
	skyboxLeftTexture->LoadTextureFile(device, commandList, 2, wPATH("SkyboxLeft.dds"));
	skyboxLeftTexture->CreateSrvDescriptorHeap(device);
	skyboxLeftTexture->CreateShaderResourceView(device);
	m_textures["SKYBOX_LEFT"] = skyboxLeftTexture;

	auto skyboxRightTexture{ make_shared<Texture>() };
	skyboxRightTexture->LoadTextureFile(device, commandList, 2, wPATH("SkyboxRight.dds"));
	skyboxRightTexture->CreateSrvDescriptorHeap(device);
	skyboxRightTexture->CreateShaderResourceView(device);
	m_textures["SKYBOX_RIGHT"] = skyboxRightTexture;

	auto skyboxBackTexture{ make_shared<Texture>() };
	skyboxBackTexture->LoadTextureFile(device, commandList, 2, wPATH("SkyboxBack.dds"));
	skyboxBackTexture->CreateSrvDescriptorHeap(device);
	skyboxBackTexture->CreateShaderResourceView(device);
	m_textures["SKYBOX_BACK"] = skyboxBackTexture;

	auto skyboxTopTexture{ make_shared<Texture>() };
	skyboxTopTexture->LoadTextureFile(device, commandList, 2, wPATH("SkyboxTop.dds"));
	skyboxTopTexture->CreateSrvDescriptorHeap(device);
	skyboxTopTexture->CreateShaderResourceView(device);
	m_textures["SKYBOX_TOP"] = skyboxTopTexture;

	auto skyboxBotTexture{ make_shared<Texture>() };
	skyboxBotTexture->LoadTextureFile(device, commandList, 2, wPATH("SkyboxBot.dds"));
	skyboxBotTexture->CreateSrvDescriptorHeap(device);
	skyboxBotTexture->CreateShaderResourceView(device);
	m_textures["SKYBOX_BOT"] = skyboxBotTexture;
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
	auto camera{ make_shared<ThirdPersonCamera>() };
	camera->CreateShaderVariable(device, commandList);
	SetCamera(camera);

	// 카메라 투영 행렬 설정
	XMFLOAT4X4 projMatrix;
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT), 1.0f, 5000.0f));
	camera->SetProjMatrix(projMatrix);

	// 플레이어 생성
	auto player{ make_shared<Player>() };
	player->SetMesh(m_meshes.at("TANK"));
	player->SetShader(m_shaders.at("MODEL"));
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

	// 반사상, 거울 렌더링
	if (m_mirror && m_player) RenderMirror(commandList, dsvHandle);

	// 스카이박스 렌더링
	if (m_skybox) m_skybox->Render(commandList);

	// 플레이어 렌더링
	if (m_player) m_player->Render(commandList);

	// 게임오브젝트 렌더링
	for (const auto& gameObject : m_gameObjects)
		gameObject->Render(commandList);

	// 지형 렌더링
	for (const auto& terrain : m_terrains)
		terrain->Render(commandList);

	// 불투명 오브젝트 렌더링
	for (const auto& translucence : m_translucences)
		translucence->Render(commandList);

	// 파티클 렌더링
	for (const auto& particle : m_particles)
		particle->Render(commandList);
}

void Scene::PostRenderProcess(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12Resource>& input)
{
	if (m_blurFilter) m_blurFilter->Excute(commandList, rootSignature, m_shaders["HORZBLUR"], m_shaders["VERTBLUR"], input);
}

void Scene::RemoveDeletedObjects()
{
	vector<unique_ptr<GameObject>> newParticles;

	auto pred = [&](unique_ptr<GameObject>& object) {
		if (object->isDeleted() && object->GetType() == GameObjectType::BULLET)
		{
			// 폭발 이펙트 생성
			auto textureInfo{ make_unique<TextureInfo>() };
			textureInfo->frameInterver *= 1.5f;
			textureInfo->isFrameRepeat = false;

			auto explosion{ make_unique<GameObject>() };
			explosion->SetPosition(object->GetPosition());
			explosion->SetMesh(m_meshes.at("EXPLOSION"));
			explosion->SetShader(m_shaders.at("BLENDING"));
			explosion->SetTexture(m_textures.at("EXPLOSION"));
			explosion->SetTextureInfo(textureInfo);
			newParticles.push_back(move(explosion));

			// 연기 이펙트 생성
			textureInfo = make_unique<TextureInfo>();
			textureInfo->frameInterver *= 3.0f;
			textureInfo->isFrameRepeat = false;

			auto smoke{ make_unique<GameObject>() };
			smoke->SetPosition(object->GetPosition());
			smoke->SetMesh(m_meshes.at("SMOKE"));
			smoke->SetShader(m_shaders.at("BLENDING"));
			smoke->SetTexture(m_textures.at("SMOKE"));
			smoke->SetTextureInfo(textureInfo);
			newParticles.push_back(move(smoke));
		}
		return object->isDeleted();
	};
	m_gameObjects.erase(remove_if(m_gameObjects.begin(), m_gameObjects.end(), pred), m_gameObjects.end());
	m_translucences.erase(remove_if(m_translucences.begin(), m_translucences.end(), pred), m_translucences.end());

	// 총알 삭제될 때 생기는 이펙트는 파티클 객체에 추가한다.
	for (auto& object : newParticles)
		m_translucences.push_back(move(object));
}

void Scene::UpdateObjectsTerrain()
{
	XMFLOAT3 pos{};
	auto pred = [&pos](unique_ptr<HeightMapTerrain>& terrain) {
		XMFLOAT3 tPos{ terrain->GetPosition() };
		XMFLOAT3 scale{ terrain->GetScale() };
		float width{ terrain->GetWidth() * scale.x };
		float length{ terrain->GetLength() * scale.z };

		// 하늘에서 +z축을 머리쪽으로 두고 지형을 봤을 때를 기준
		float left{ tPos.x };
		float right{ tPos.x + width };
		float top{ tPos.z + length };
		float bot{ tPos.z };

		if ((left <= pos.x && pos.x <= right) &&
			(bot <= pos.z && pos.z <= top))
			return true;
		return false;
	};

	if (m_player)
	{
		pos = m_player->GetPosition();
		auto terrain = find_if(m_terrains.begin(), m_terrains.end(), pred);
		m_player->SetTerrain(terrain != m_terrains.end() ? terrain->get() : nullptr);
	}
	if (m_camera)
	{
		pos = m_camera->GetEye();
		auto terrain = find_if(m_terrains.begin(), m_terrains.end(), pred);
		m_camera->SetTerrain(terrain != m_terrains.end() ? terrain->get() : nullptr);
	}
	for (auto& object : m_gameObjects)
	{
		pos = object->GetPosition();
		auto terrain = find_if(m_terrains.begin(), m_terrains.end(), pred);
		object->SetTerrain(terrain != m_terrains.end() ? terrain->get() : nullptr);
	}
	for (auto& object : m_translucences)
	{
		pos = object->GetPosition();
		auto terrain = find_if(m_terrains.begin(), m_terrains.end(), pred);
		object->SetTerrain(terrain != m_terrains.end() ? terrain->get() : nullptr);
	}
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
	commandList->SetGraphicsRootConstantBufferView(5, m_cbScene->GetGPUVirtualAddress());

	// 카메라 셰이더 변수 최신화
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
}

void Scene::RenderMirror(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	// 스텐실 버퍼 초기화
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	// 스텐실 참조값 설정
	commandList->OMSetStencilRef(1);

	// 거울 위치를 스텐실 버퍼에 표시
	m_mirror->Render(commandList, m_shaders.at("STENCIL"));

	// 반사 행렬
	XMVECTOR mirrorPlane{ XMVectorSet(0.0f, 0.0f, -1.0f, m_mirror->GetPosition().z) };
	XMFLOAT4X4 reflectMatrix;
	XMStoreFloat4x4(&reflectMatrix, XMMatrixReflect(mirrorPlane));

	// 실내 반사상 렌더링
	for (const auto& object : m_gameObjects)
	{
		XMFLOAT4X4 temp{ object->GetWorldMatrix() };
		object->SetWorldMatrix(Matrix::Mul(temp, reflectMatrix));
		object->Render(commandList, m_shaders.at("MIRRORTEXTURE"));
		object->SetWorldMatrix(temp);
	}

	// 플레이어 반사상 렌더링
	XMFLOAT4X4 originWorldMatrix{ m_player->GetWorldMatrix() };
	m_player->SetWorldMatrix(Matrix::Mul(originWorldMatrix, reflectMatrix));
	m_player->Render(commandList, m_shaders.at("MIRROR"));
	m_player->SetWorldMatrix(originWorldMatrix);

	// 거울 렌더링
	m_mirror->Render(commandList);

	// 스텐실 참조값 초기화
	commandList->OMSetStencilRef(0);
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

void Scene::CreateBullet()
{
	unique_ptr<Bullet> bullet{ make_unique<Bullet>(m_player->GetPosition(), m_player->GetLook(), m_player->GetNormal(), 100.0f) };
	bullet->SetPosition(Vector3::Add(m_player->GetPosition(), Vector3::Mul(m_player->GetNormal(), 0.5f)));
	bullet->SetMesh(m_meshes.at("BULLET"));
	bullet->SetShader(m_shaders.at("TEXTURE"));
	bullet->SetTexture(m_textures.at("ROCK"));
	m_gameObjects.push_back(move(bullet));
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

HeightMapTerrain* Scene::GetTerrain(FLOAT x, FLOAT z) const
{
	auto terrain = find_if(m_terrains.begin(), m_terrains.end(), [&x, &z](const unique_ptr<HeightMapTerrain>& t) {
		XMFLOAT3 scale{ t->GetScale() };
		XMFLOAT3 pos{ t->GetPosition() };
		float width{ t->GetWidth() * scale.x };
		float length{ t->GetLength() * scale.z };

		// 지형을 하늘에서 밑으로 봤을 때
		float left{ pos.x };			// 왼쪽
		float right{ pos.x + width };	// 오른쪽
		float top{ pos.z + length };	// 위
		float bot{ pos.z };				// 밑

		if ((left <= x && x <= right) && (bot <= z && z <= top))
			return true;
		return false;
		});

	return terrain != m_terrains.end() ? terrain->get() : nullptr;
}

ComPtr<ID3D12Resource> Scene::GetPostRenderProcessResult() const
{
	if (m_blurFilter) return m_blurFilter->GetResult();
	return nullptr;
}