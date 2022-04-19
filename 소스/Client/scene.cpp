#include "scene.h"
#include "framework.h"

Scene::Scene()
	: m_viewport{ 0.0f, 0.0f, static_cast<float>(Setting::SCREEN_WIDTH), static_cast<float>(Setting::SCREEN_HEIGHT), 0.0f, 1.0f },
	  m_scissorRect{ 0, 0, Setting::SCREEN_WIDTH, Setting::SCREEN_HEIGHT }, m_pcbScene{ nullptr }
{

}

Scene::~Scene()
{
	if (m_cbScene) m_cbScene->Unmap(0, NULL);
}

void Scene::OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
				   const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature,
				   const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
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
	m_shadowMap = make_unique<ShadowMap>(device, 1 << 12, 1 << 12, Setting::SHADOWMAP_COUNT);

	// 블러 필터 생성
	//m_blurFilter = make_unique<BlurFilter>(device);

	// 게임오브젝트 생성
	CreateGameObjects(device, commandList);

	// UI 오브젝트 생성
	CreateUIObjects(device, commandList);

	// 텍스트 오브젝트 생성
	CreateTextObjects(d2dDeivceContext, dWriteFactory);

	// 조명 생성
	CreateLights();

	// 맵 로딩
	LoadMapObjects(device, commandList, Utile::PATH("map.txt", Utile::RESOURCE));
}

void Scene::OnMouseEvent(HWND hWnd, FLOAT deltaTime)
{
	// 화면 가운데 좌표 계산
	RECT rect; GetWindowRect(hWnd, &rect);
	POINT oldMousePosition{ static_cast<LONG>(rect.left + Setting::SCREEN_WIDTH / 2), static_cast<LONG>(rect.top + Setting::SCREEN_HEIGHT / 2) };

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

void Scene::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (m_camera) m_camera->OnMouseEvent(hWnd, message, wParam, lParam);
	if (m_player) m_player->OnMouseEvent(hWnd, message, wParam, lParam);
}

void Scene::OnKeyboardEvent(FLOAT deltaTime)
{
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

void Scene::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
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
			PostMessage(hWnd, WM_QUIT, 0, 0);
			break;
		}
		break;
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
	for (auto& [_, m] : m_monsters)
		m->Update(deltaTime);
	for (auto& ui : m_uiObjects)
		ui->Update(deltaTime);
	for (auto& t : m_textObjects)
		t->Update(deltaTime);
}

void Scene::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_cbScene = Utile::CreateBufferResource(device, commandList, NULL, Utile::GetConstantBufferSize<cbScene>(), 1, D3D12_HEAP_TYPE_UPLOAD, {});
	m_cbScene->Map(0, NULL, reinterpret_cast<void**>(&m_pcbScene));
	m_cbSceneData = make_unique<cbScene>();
}

void Scene::CreateMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 플레이어 관련 메쉬 로딩
	vector<pair<string, string>> animations
	{
		{ "idle", "IDLE" }, { "walking", "WALKING" }, {"walkLeft", "WALKLEFT" }, { "walkRight", "WALKRIGHT" },
		{ "walkBack", "WALKBACK" }, { "running", "RUNNING" }, {"firing", "FIRING" }, { "reload", "RELOAD" }
	};
	m_meshes["PLAYER"] = make_shared<Mesh>();
	m_meshes["PLAYER"]->LoadMeshBinary(device, commandList, Utile::PATH("player.bin", Utile::RESOURCE));
	for (const string& weaponName : { "AR", "SG", "MG" })
		for (const auto& [fileName, animationName] : animations)
				m_meshes["PLAYER"]->LoadAnimationBinary(device, commandList, Utile::PATH(weaponName + "/" + fileName + ".bin", Utile::RESOURCE), weaponName + "/" + animationName);

	m_meshes["ARM"] = make_shared<Mesh>();
	m_meshes["ARM"]->LoadMeshBinary(device, commandList, Utile::PATH("arm.bin", Utile::RESOURCE));
	m_meshes["ARM"]->Link(m_meshes["PLAYER"]);
	m_meshes["AR"] = make_shared<Mesh>();
	m_meshes["AR"]->LoadMeshBinary(device, commandList, Utile::PATH("AR/AR.bin", Utile::RESOURCE));
	m_meshes["AR"]->Link(m_meshes["PLAYER"]);
	m_meshes["SG"] = make_shared<Mesh>();
	m_meshes["SG"]->LoadMeshBinary(device, commandList, Utile::PATH("SG/SG.bin", Utile::RESOURCE));
	m_meshes["SG"]->Link(m_meshes["PLAYER"]);
	m_meshes["MG"] = make_shared<Mesh>();
	m_meshes["MG"]->LoadMeshBinary(device, commandList, Utile::PATH("MG/MG.bin", Utile::RESOURCE));
	m_meshes["MG"]->Link(m_meshes["PLAYER"]);

	// 몬스터 관련 로딩
	m_meshes["GAROO"] = make_shared<Mesh>();
	m_meshes["GAROO"]->LoadMesh(device, commandList, Utile::PATH("Mob/AlienGaroo/AlienGaroo.txt", Utile::RESOURCE));
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/attack.txt", Utile::RESOURCE), "ATTACK");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/die.txt", Utile::RESOURCE), "DIE");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/hit.txt", Utile::RESOURCE), "HIT");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/idle.txt", Utile::RESOURCE), "IDLE");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/running.txt", Utile::RESOURCE), "RUNNING");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/walkBack.txt", Utile::RESOURCE), "WALKBACK");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/walking.txt", Utile::RESOURCE), "WALKING");

	// 게임오브젝트 관련 로딩
	m_meshes["FLOOR"] = make_shared<RectMesh>(device, commandList, 2000.0f, 0.0f, 2000.0f, XMFLOAT3{}, XMFLOAT4{ 0.8f, 0.8f, 0.8f, 1.0f });
	m_meshes["BULLET"] = make_shared<CubeMesh>(device, commandList, 0.05f, 0.05f, 10.0f, XMFLOAT3{ 0.0f, 0.0f, 5.0f }, XMFLOAT4{ 39.0f / 255.0f, 151.0f / 255.0f, 255.0f / 255.0f, 1.0f });

	// 맵 오브젝트 관련 로딩
	m_meshes["MOUNTAIN"] = make_shared<Mesh>();
	m_meshes["MOUNTAIN"]->LoadMesh(device, commandList, Utile::PATH(("Object/mountain.txt"), Utile::RESOURCE));
	m_meshes["PLANT"] = make_shared<Mesh>();
	m_meshes["PLANT"]->LoadMesh(device, commandList, Utile::PATH(("Object/bigPlant.txt"), Utile::RESOURCE));
	m_meshes["TREE"] = make_shared<Mesh>();
	m_meshes["TREE"]->LoadMesh(device, commandList, Utile::PATH(("Object/tree.txt"), Utile::RESOURCE));
	m_meshes["ROCK1"] = make_shared<Mesh>();
	m_meshes["ROCK1"]->LoadMesh(device, commandList, Utile::PATH(("Object/rock1.txt"), Utile::RESOURCE));
	m_meshes["ROCK2"] = make_shared<Mesh>();
	m_meshes["ROCK2"]->LoadMesh(device, commandList, Utile::PATH(("Object/rock2.txt"), Utile::RESOURCE));
	m_meshes["ROCK3"] = make_shared<Mesh>();
	m_meshes["ROCK3"]->LoadMesh(device, commandList, Utile::PATH(("Object/rock3.txt"), Utile::RESOURCE));
	m_meshes["SMALLROCK"] = make_shared<Mesh>();
	m_meshes["SMALLROCK"]->LoadMesh(device, commandList, Utile::PATH(("Object/smallRock.txt"), Utile::RESOURCE));
	m_meshes["ROCKGROUP1"] = make_shared<Mesh>();
	m_meshes["ROCKGROUP1"]->LoadMesh(device, commandList, Utile::PATH(("Object/rockGroup1.txt"), Utile::RESOURCE));
	m_meshes["ROCKGROUP2"] = make_shared<Mesh>();
	m_meshes["ROCKGROUP2"]->LoadMesh(device, commandList, Utile::PATH(("Object/rockGroup2.txt"), Utile::RESOURCE));
	m_meshes["DROPSHIP"] = make_shared<Mesh>();
	m_meshes["DROPSHIP"]->LoadMesh(device, commandList, Utile::PATH(("Object/dropship.txt"), Utile::RESOURCE));
	m_meshes["MUSHROOMS"] = make_shared<Mesh>();
	m_meshes["MUSHROOMS"]->LoadMesh(device, commandList, Utile::PATH(("Object/mushrooms.txt"), Utile::RESOURCE));
	m_meshes["SKULL"] = make_shared<Mesh>();
	m_meshes["SKULL"]->LoadMesh(device, commandList, Utile::PATH(("Object/skull.txt"), Utile::RESOURCE));
	m_meshes["RIBS"] = make_shared<Mesh>();
	m_meshes["RIBS"]->LoadMesh(device, commandList, Utile::PATH(("Object/ribs.txt"), Utile::RESOURCE));
	m_meshes["ROCK4"] = make_shared<Mesh>();
	m_meshes["ROCK4"]->LoadMesh(device, commandList, Utile::PATH(("Object/rock4.txt"), Utile::RESOURCE));
	m_meshes["ROCK5"] = make_shared<Mesh>();
	m_meshes["ROCK5"]->LoadMesh(device, commandList, Utile::PATH(("Object/rock5.txt"), Utile::RESOURCE));

	// 게임 시스템 관련 로딩
	m_meshes["SKYBOX"] = make_shared<Mesh>();
	m_meshes["SKYBOX"]->LoadMesh(device, commandList, Utile::PATH("Skybox/Skybox.txt", Utile::RESOURCE));
	m_meshes["UI"] = make_shared<RectMesh>(device, commandList, 1.0f, 1.0f, 0.0f, XMFLOAT3{ 0.0f, 0.0f, 1.0f });
	m_meshes["HPBAR"] = make_shared<RectMesh>(device, commandList, 1.0f, 1.0f, 0.0f, XMFLOAT3{ 0.0f, 0.0f, 1.0f }, XMFLOAT4{ 1.0f, 1.0f, 1.0f, 0.5f });

	// 디버그 바운딩박스 로딩
	m_meshes["BB_PLAYER"] = make_shared<CubeMesh>(device, commandList, 8.0f, 32.5f, 8.0f, XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT4{ 0.0f, 0.8f, 0.0f, 1.0f });
	m_meshes["BB_GAROO"] = make_shared<CubeMesh>(device, commandList, 7.0f, 7.0f, 10.0f, XMFLOAT3{ 0.0f, 8.0f, 0.0f }, XMFLOAT4{ 0.8f, 0.0f, 0.0f, 1.0f });
	m_meshes["BB_SMALLROCK"] = make_shared<CubeMesh>(device, commandList, 100.0f, 100.0f, 100.0f, XMFLOAT3{}, XMFLOAT4{ 0.8f, 0.0f, 0.0f, 1.0f });
}

void Scene::CreateShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature)
{
	m_shaders["DEFAULT"] = make_shared<Shader>(device, rootSignature, Utile::PATH(TEXT("default.hlsl"), Utile::SHADER), "VS", "PS");
	m_shaders["SKYBOX"] = make_shared<NoDepthShader>(device, rootSignature, Utile::PATH(TEXT("model.hlsl"), Utile::SHADER), "VS", "PSSkybox");
	m_shaders["MODEL"] = make_shared<Shader>(device, rootSignature, Utile::PATH(TEXT("model.hlsl"), Utile::SHADER), "VS", "PS");
	m_shaders["ANIMATION"] = make_shared<Shader>(device, rootSignature, Utile::PATH(TEXT("animation.hlsl"), Utile::SHADER), "VS", "PS");
	m_shaders["LINK"] = make_shared<Shader>(device, rootSignature, Utile::PATH(TEXT("link.hlsl"), Utile::SHADER), "VS", "PS");
	m_shaders["UI"] = make_shared<BlendingShader>(device, rootSignature, Utile::PATH(TEXT("ui.hlsl"), Utile::SHADER), "VS", "PS");

	// 그림자 셰이더
	m_shaders["SHADOW_MODEL"] = make_shared<ShadowShader>(device, rootSignature, Utile::PATH(TEXT("shadow.hlsl"), Utile::SHADER), "VS_MODEL", "GS");
	m_shaders["SHADOW_ANIMATION"] = make_shared<ShadowShader>(device, rootSignature, Utile::PATH(TEXT("shadow.hlsl"), Utile::SHADER), "VS_ANIMATION", "GS");
	m_shaders["SHADOW_LINK"] = make_shared<ShadowShader>(device, rootSignature, Utile::PATH(TEXT("shadow.hlsl"), Utile::SHADER), "VS_LINK", "GS");

	// 디버그
	m_shaders["WIREFRAME"] = make_shared<WireframeShader>(device, rootSignature, Utile::PATH(TEXT("default.hlsl"), Utile::SHADER), "VS", "PS");
}

void Scene::CreateTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	m_textures["SKYBOX"] = make_shared<Texture>();
	m_textures["SKYBOX"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("Skybox/skybox.dds"), Utile::RESOURCE));
	m_textures["SKYBOX"]->CreateTexture(device);

	m_textures["GAROO"] = make_shared<Texture>();
	m_textures["GAROO"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("Mob/AlienGaroo/texture.dds"), Utile::RESOURCE));
	m_textures["GAROO"]->CreateTexture(device);

	m_textures["FLOOR"] = make_shared<Texture>();
	m_textures["FLOOR"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("Object/floor.dds"), Utile::RESOURCE));
	m_textures["FLOOR"]->CreateTexture(device);
	m_textures["OBJECT1"] = make_shared<Texture>();
	m_textures["OBJECT1"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("Object/texture1.dds"), Utile::RESOURCE));
	m_textures["OBJECT1"]->CreateTexture(device);
	m_textures["OBJECT2"] = make_shared<Texture>();
	m_textures["OBJECT2"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("Object/texture2.dds"), Utile::RESOURCE));
	m_textures["OBJECT2"]->CreateTexture(device);
	m_textures["OBJECT3"] = make_shared<Texture>();
	m_textures["OBJECT3"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("Object/texture3.dds"), Utile::RESOURCE));
	m_textures["OBJECT3"]->CreateTexture(device);

	m_textures["CROSSHAIR"] = make_shared<Texture>();
	m_textures["CROSSHAIR"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("UI/crosshair.dds"), Utile::RESOURCE));
	m_textures["CROSSHAIR"]->CreateTexture(device);

	m_textures["HPBARBASE"] = make_shared<Texture>();
	m_textures["HPBARBASE"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("UI/HPBarBase.dds"), Utile::RESOURCE));
	m_textures["HPBARBASE"]->CreateTexture(device);

	m_textures["HPBAR"] = make_shared<Texture>();
	m_textures["HPBAR"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("UI/HPBar.dds"), Utile::RESOURCE));
	m_textures["HPBAR"]->CreateTexture(device);
}

void Scene::CreateLights() const
{
	m_cbSceneData->shadowLight.color = XMFLOAT3{ 0.1f, 0.1f, 0.1f };
	m_cbSceneData->shadowLight.direction = Vector3::Normalize(XMFLOAT3{ -0.687586f, -0.716385f, 0.118001f });

	// 그림자를 만드는 조명의 마지막 뷰, 투영 변환 행렬의 경우 씬을 덮는 영역으로, 업데이트할 필요 없음
	XMFLOAT4X4 lightViewMatrix, lightProjMatrix;
	XMFLOAT3 shadowLightPos{ Vector3::Mul(m_cbSceneData->shadowLight.direction, -1500.0f) };
	XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
	XMStoreFloat4x4(&lightViewMatrix, XMMatrixLookAtLH(XMLoadFloat3(&shadowLightPos), XMLoadFloat3(&m_cbSceneData->shadowLight.direction), XMLoadFloat3(&up)));
	XMStoreFloat4x4(&lightProjMatrix, XMMatrixOrthographicLH(3000.0f, 3000.0f, 0.0f, 5000.0f));
	m_cbSceneData->shadowLight.lightViewMatrix[Setting::SHADOWMAP_COUNT - 1] = Matrix::Transpose(lightViewMatrix);
	m_cbSceneData->shadowLight.lightProjMatrix[Setting::SHADOWMAP_COUNT - 1] = Matrix::Transpose(lightProjMatrix);

	m_cbSceneData->ligths[0].color = XMFLOAT3{ 0.05f, 0.05f, 0.05f };
	m_cbSceneData->ligths[0].direction = XMFLOAT3{ 0.0f, -6.0f, 10.0f };
}

void Scene::CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// UI 카메라 생성
	XMFLOAT4X4 projMatrix{};
	m_uiCamera = make_unique<Camera>();
	m_uiCamera->CreateShaderVariable(device, commandList);
	XMStoreFloat4x4(&projMatrix, XMMatrixOrthographicLH(static_cast<float>(Setting::SCREEN_WIDTH), static_cast<float>(Setting::SCREEN_HEIGHT), 0.0f, 1.0f));
	m_uiCamera->SetProjMatrix(projMatrix);

	// 조준점
	auto crossHair{ make_unique<UIObject>(30.0f, 30.0f) };
	crossHair->SetMesh(m_meshes["UI"]);
	crossHair->SetShader(m_shaders["UI"]);
	crossHair->SetTexture(m_textures["CROSSHAIR"]);
	m_uiObjects.push_back(move(crossHair));

	// 체력바 베이스
	auto hpBarBase{ make_unique<UIObject>(200.0f, 30.0f) };
	hpBarBase->SetMesh(m_meshes["UI"]);
	hpBarBase->SetShader(m_shaders["UI"]);
	hpBarBase->SetTexture(m_textures["HPBARBASE"]);
	hpBarBase->SetPivot(ePivot::LEFTBOT);
	hpBarBase->SetPosition(-Setting::SCREEN_WIDTH / 2.0f + 50.0f, -Setting::SCREEN_HEIGHT / 2.0f + 40.0f);
	m_uiObjects.push_back(move(hpBarBase));

	// 체력바
	auto hpBar{ make_unique<HpUIObject>(200.0f, 30.0f) };
	hpBar->SetMesh(m_meshes["UI"]);
	hpBar->SetShader(m_shaders["UI"]);
	hpBar->SetTexture(m_textures["HPBAR"]);
	hpBar->SetPivot(ePivot::LEFTBOT);
	hpBar->SetPosition(-Setting::SCREEN_WIDTH / 2.0f + 50.0f, -Setting::SCREEN_HEIGHT / 2.0f + 40.0f);
	hpBar->SetPlayer(m_player);
	m_uiObjects.push_back(move(hpBar));
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
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(Setting::SCREEN_WIDTH) / static_cast<float>(Setting::SCREEN_HEIGHT), 1.0f, 2500.0f));
	m_camera->SetProjMatrix(projMatrix);

	// 바운딩박스
	SharedBoundingBox bbPlayer{ make_shared<DebugBoundingBox>(XMFLOAT3{ 0.0f, 32.5f / 2.0f, 0.0f }, XMFLOAT3{ 8.0f / 2.0f, 32.5f / 2.0f, 8.0f / 2.0f }, XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f }) };
	bbPlayer->SetMesh(m_meshes["BB_PLAYER"]);
	bbPlayer->SetShader(m_shaders["WIREFRAME"]);

	// 플레이어 생성
	m_player = make_shared<Player>();
	m_player->SetMesh(m_meshes["ARM"]);
	m_player->SetShader(m_shaders["ANIMATION"]);
	m_player->SetShadowShader(m_shaders["SHADOW_ANIMATION"]);
	m_player->SetGunMesh(m_meshes["AR"]);
	m_player->SetGunShader(m_shaders["LINK"]);
	m_player->SetGunShadowShader(m_shaders["SHADOW_LINK"]);
	m_player->SetGunType(eGunType::AR);
	m_player->PlayAnimation("IDLE");
	m_player->AddBoundingBox(bbPlayer);

	// 카메라, 플레이어 서로 설정
	m_camera->SetPlayer(m_player);
	m_player->SetCamera(m_camera);

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
	floor->SetTexture(m_textures["FLOOR"]);
	m_gameObjects.push_back(move(floor));
}

void Scene::CreateTextObjects(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	// Create D2D/DWrite objects for rendering text.
	DX::ThrowIfFailed(d2dDeivceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black, 0.8f), &TextObject::s_brushes["BLACK"]));
	DX::ThrowIfFailed(d2dDeivceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red, 0.8f), &TextObject::s_brushes["RED"]));
	DX::ThrowIfFailed(d2dDeivceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DeepSkyBlue, 0.8f), &TextObject::s_brushes["BLUE"]));
	DX::ThrowIfFailed(dWriteFactory->CreateTextFormat(
		TEXT("나눔바른고딕OTF"),
		NULL,
		DWRITE_FONT_WEIGHT_ULTRA_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		36,
		TEXT("ko-kr"),
		&TextObject::s_formats["BULLETCOUNT"]
	));
	DX::ThrowIfFailed(TextObject::s_formats["BULLETCOUNT"]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING));
	DX::ThrowIfFailed(TextObject::s_formats["BULLETCOUNT"]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR));

	DX::ThrowIfFailed(dWriteFactory->CreateTextFormat(
		TEXT("나눔바른고딕OTF"),
		NULL,
		DWRITE_FONT_WEIGHT_ULTRA_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		24,
		TEXT("ko-kr"),
		&TextObject::s_formats["MAXBULLETCOUNT"]
	));
	DX::ThrowIfFailed(TextObject::s_formats["MAXBULLETCOUNT"]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING));
	DX::ThrowIfFailed(TextObject::s_formats["MAXBULLETCOUNT"]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR));

	auto bulletText{ make_unique<BulletTextObject>() };
	bulletText->SetPlayer(m_player);
	bulletText->SetPosition(XMFLOAT2{ Setting::SCREEN_WIDTH - 150.0f, Setting::SCREEN_HEIGHT - 75.0f });
	m_textObjects.push_back(move(bulletText));

	auto hpText{ make_unique<HPTextObject>() };
	hpText->SetPlayer(m_player);
	hpText->SetPosition(XMFLOAT2{ 60.0f, Setting::SCREEN_HEIGHT - 115.0f });
	m_textObjects.push_back(move(hpText));
}

void Scene::LoadMapObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& mapFile)
{
	ifstream map{ mapFile };

	// 오브젝트 개수
	int count{ 0 }; map >> count;

	for (int i = 0; i < count; ++i)
	{
		// 타입, 스케일, 회전, 이동
		int type{}; map >> type;
		XMFLOAT3 scale{}; map >> scale.x >> scale.y >> scale.z;
		XMFLOAT3 rotat{}; map >> rotat.x >> rotat.y >> rotat.z;
		XMFLOAT3 trans{}; map >> trans.x >> trans.y >> trans.z;

		XMMATRIX worldMatrix{ XMMatrixIdentity() };
		XMMATRIX scaleMatrix{ XMMatrixScaling(scale.x, scale.y, scale.z) };
		XMMATRIX rotateMatrix{ XMMatrixRotationRollPitchYaw(XMConvertToRadians(rotat.x), XMConvertToRadians(rotat.y), XMConvertToRadians(rotat.z)) };
		XMMATRIX transMatrix{ XMMatrixTranslation(trans.x, trans.y, trans.z) };
		worldMatrix = worldMatrix * scaleMatrix * rotateMatrix * transMatrix;

		XMFLOAT4X4 world;
		XMStoreFloat4x4(&world, worldMatrix);

		unique_ptr<GameObject> object{ make_unique<GameObject>() };
		object->SetShader(m_shaders["MODEL"]);
		object->SetShadowShader(m_shaders["SHADOW_MODEL"]);
		object->SetWorldMatrix(world);

		eMapObjectType mot{ static_cast<eMapObjectType>(type) };
		switch (mot)
		{
		case eMapObjectType::MOUNTAIN:
		{
			object->SetMesh(m_meshes["MOUNTAIN"]);
			break;
		}
		case eMapObjectType::PLANT:
			object->SetMesh(m_meshes["PLANT"]);
			object->SetTexture(m_textures["OBJECT1"]);
			break;
		case eMapObjectType::TREE:
			object->SetMesh(m_meshes["TREE"]);
			object->SetTexture(m_textures["OBJECT2"]);
			break;
		case eMapObjectType::ROCK1:
		{
			object->SetMesh(m_meshes["ROCK1"]);
			object->SetTexture(m_textures["OBJECT3"]);
			break;
		}
		case eMapObjectType::ROCK2:
			object->SetMesh(m_meshes["ROCK2"]);
			object->SetTexture(m_textures["OBJECT3"]);
			break;
		case eMapObjectType::ROCK3:
			object->SetMesh(m_meshes["ROCK3"]);
			object->SetTexture(m_textures["OBJECT3"]);
			break;
		case eMapObjectType::SMALLROCK:
			object->SetMesh(m_meshes["SMALLROCK"]);
			object->SetTexture(m_textures["OBJECT3"]);
			break;
		case eMapObjectType::ROCKGROUP1:
		{
			object->SetMesh(m_meshes["ROCKGROUP1"]);
			object->SetTexture(m_textures["OBJECT3"]);

			SharedBoundingBox bb{ make_shared<DebugBoundingBox>(XMFLOAT3{ 0.0f, 50.0f, 0.0f }, XMFLOAT3{ 50.0f, 50.0f, 50.0f }, XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f }) };
			bb->SetMesh(m_meshes["BB_SMALLROCK"]);
			bb->SetShader(m_shaders["WIREFRAME"]);
			object->AddBoundingBox(bb);
			break;
		}
		case eMapObjectType::ROCKGROUP2:
		{
			object->SetMesh(m_meshes["ROCKGROUP2"]);
			object->SetTexture(m_textures["OBJECT3"]);
			break;
		}
		case eMapObjectType::DROPSHIP:
			object->SetMesh(m_meshes["DROPSHIP"]);
			object->SetTexture(m_textures["OBJECT3"]);
			break;
		case eMapObjectType::MUSHROOMS:
			object->SetMesh(m_meshes["MUSHROOMS"]);
			object->SetTexture(m_textures["OBJECT1"]);
			break;
		case eMapObjectType::SKULL:
			object->SetMesh(m_meshes["SKULL"]);
			object->SetTexture(m_textures["OBJECT2"]);
			break;
		case eMapObjectType::RIBS:
			object->SetMesh(m_meshes["RIBS"]);
			object->SetTexture(m_textures["OBJECT2"]);
			break;
		case eMapObjectType::ROCK4:
			object->SetMesh(m_meshes["ROCK4"]);
			object->SetTexture(m_textures["OBJECT1"]);
			break;
		case eMapObjectType::ROCK5:
			object->SetMesh(m_meshes["ROCK5"]);
			object->SetTexture(m_textures["OBJECT1"]);
			break;
		}
		m_gameObjects.push_back(move(object));
	}
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
	unique_lock<mutex> lock{ g_mutex };

	// 게임오브젝트 삭제
	erase_if(m_gameObjects, [](unique_ptr<GameObject>& object) { return object->isDeleted(); });

	// 몬스터 삭제
	erase_if(m_monsters, [](const auto& item) { return item.second->isDeleted(); });

	lock.unlock();

	// 충돌판정
	PlayerCollisionCheck(deltaTime);

	// 그림자맵 뷰, 투영 행렬 최신화
	UpdateShadowMatrix();
}

void Scene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	// 뷰포트, 가위사각형, 렌더타겟 설정
	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);
	commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

	// 스카이박스 렌더링
	if (m_skybox) m_skybox->Render(commandList);

	// 게임오브젝트 렌더링
	unique_lock<mutex> lock{ g_mutex };
	for (const auto& o : m_gameObjects)
		o->Render(commandList);
	lock.unlock();

	// 멀티플레이어 렌더링
	for (const auto& p : m_multiPlayers)
		if (p) p->Render(commandList);

	// 몬스터 렌더링
	lock.lock();
	for (const auto& [_, m] : m_monsters)
		m->Render(commandList);
	lock.unlock();

	// 플레이어 렌더링
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
	if (m_player) m_player->Render(commandList);

	// UI 렌더링
	if (m_uiCamera)
	{
		m_uiCamera->UpdateShaderVariable(commandList);
		for (const auto& ui : m_uiObjects)
			ui->Render(commandList);
	}
}

void Scene::Render2D(const ComPtr<ID2D1DeviceContext2>& device)
{
	for (const auto& t : m_textObjects)
		t->Render(device);
}

void Scene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	// 씬 셰이더 변수 최신화
	memcpy(m_pcbScene, m_cbSceneData.get(), sizeof(cbScene));
	commandList->SetGraphicsRootConstantBufferView(3, m_cbScene->GetGPUVirtualAddress());

	// 카메라 셰이더 변수 최신화
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
}

void Scene::RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	if (!m_shadowMap) return;

	// 뷰포트, 가위사각형 설정
	commandList->RSSetViewports(1, &m_shadowMap->GetViewport());
	commandList->RSSetScissorRects(1, &m_shadowMap->GetScissorRect());

	// 셰이더에 묶기
	ID3D12DescriptorHeap* ppHeaps[]{ m_shadowMap->GetSrvHeap().Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandList->SetGraphicsRootDescriptorTable(6, m_shadowMap->GetGpuSrvHandle(0));

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
		auto shadowShader{ object->GetShadowShader() };
		if (shadowShader)
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
		m_player->SetMesh(m_meshes.at("PLAYER"));
		m_player->RenderToShadowMap(commandList);
		m_player->SetMesh(m_meshes.at("ARM"));
	}

	// 리소스배리어 설정(셰이더에서 읽기)
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMap->GetShadowMap().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void Scene::PlayerCollisionCheck(FLOAT deltaTime)
{
	// 플레이어의 바운딩박스
	const auto& pPbb{ m_player->GetBoundingBox().front() };
	BoundingOrientedBox pbb;
	pPbb->Transform(pbb, XMLoadFloat4x4(&m_player->GetWorldMatrix()));

	// 플레이어와 게임오브젝트 충돌판정
	for (const auto& object : m_gameObjects)
	{
		const auto& boundingBoxes{ object->GetBoundingBox() };
		for (const auto& bb : boundingBoxes)
		{
			BoundingOrientedBox obb;
			bb->Transform(obb, XMLoadFloat4x4(&object->GetWorldMatrix()));
			if (pbb.Intersects(obb))
			{
				XMFLOAT3 v{ Vector3::Sub(m_player->GetPosition(), object->GetPosition()) };
				v = Vector3::Normalize(v);
				m_player->Move(Vector3::Mul(v, Vector3::Length(m_player->GetVelocity()) * deltaTime));
				m_player->SendPlayerData();
			}
		}
	}

	// 플레이어와 멀티플레이어 충돌판정
	for (const auto& p : m_multiPlayers)
	{
		if (!p) continue;
		BoundingOrientedBox mpbb;
		p->GetBoundingBox().front()->Transform(mpbb, XMLoadFloat4x4(&p->GetWorldMatrix()));
		if (pbb.Intersects(mpbb))
		{
			XMFLOAT3 v{ Vector3::Sub(m_player->GetPosition(), p->GetPosition()) };
			v = Vector3::Normalize(v);
			m_player->Move(Vector3::Mul(v, 3.0f * deltaTime));
			m_player->SendPlayerData();
		}
	}
}

void Scene::UpdateShadowMatrix()
{
	// 케스케이드 범위를 나눔
	constexpr array<float, Setting::SHADOWMAP_COUNT> casecade{ 0.0f, 0.05f, 0.2f, 0.4f };

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
		XMFLOAT3 tFrustum[8];
		for (int j = 0; j < 8; ++j)
			tFrustum[j] = frustum[j];

		for (int j = 0; j < 4; ++j)
		{
			// 앞쪽에서 뒤쪽으로 향하는 벡터
			XMFLOAT3 v{ Vector3::Sub(tFrustum[j + 4], tFrustum[j]) };

			// 구간 시작, 끝으로 만들어주는 벡터
			XMFLOAT3 n{ Vector3::Mul(v, casecade[i]) };
			XMFLOAT3 f{ Vector3::Mul(v, casecade[i + 1]) };

			// 구간 시작, 끝으로 설정
			tFrustum[j + 4] = Vector3::Add(tFrustum[j], f);
			tFrustum[j] = Vector3::Add(tFrustum[j], n);
		}

		// 해당 구간을 포함할 바운딩구의 중심을 계산
		XMFLOAT3 center{};
		for (const auto& v : tFrustum)
			center = Vector3::Add(center, v);
		center = Vector3::Mul(center, 1.0f / 8.0f);

		// 바운딩구의 반지름을 계산
		float radius{};
		for (const auto& v : tFrustum)
			radius = max(radius, Vector3::Length(Vector3::Sub(v, center)));

		// 그림자를 만들 조명의 좌표를 바운딩구의 중심에서 빛의 반대방향으로 적당히 움직이여야함
		// 이건 씬의 오브젝트를 고려해서 정말 적당한 수치만큼 움직여줘야함
		float value{ max(750.0f, radius * 2.5f)};
		XMFLOAT3 shadowLightPos{ Vector3::Add(center, Vector3::Mul(m_cbSceneData->shadowLight.direction, -value)) };

		XMFLOAT4X4 lightViewMatrix, lightProjMatrix;
		XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
		XMStoreFloat4x4(&lightViewMatrix, XMMatrixLookAtLH(XMLoadFloat3(&shadowLightPos), XMLoadFloat3(&center), XMLoadFloat3(&up)));
		XMStoreFloat4x4(&lightProjMatrix, XMMatrixOrthographicLH(radius * 2.0f, radius * 2.0f, 0.0f, 5000.0f));
		//XMMatrixOrthographicOffCenterLH(-radius, radius, -radius, radius, 0.0f, radius * 2.0f);

		m_cbSceneData->shadowLight.lightViewMatrix[i] = Matrix::Transpose(lightViewMatrix);
		m_cbSceneData->shadowLight.lightProjMatrix[i] = Matrix::Transpose(lightProjMatrix);
	}
}

void Scene::CreateBullet()
{
	// 총알 시작 좌표
	XMFLOAT3 start{ m_camera->GetEye() };
	start = Vector3::Add(start, m_player->GetRight());
	start = Vector3::Add(start, Vector3::Mul(m_player->GetUp(), -0.5f));

	// 화면 정중앙 엄청 멀리
	XMFLOAT3 _far{ m_camera->GetEye() };
	_far = Vector3::Add(_far, Vector3::Mul(m_camera->GetAt(), 1000.0f));

	// 총 -> 화면 정중앙 멀리
	XMFLOAT3 target{ Vector3::Sub(_far, m_camera->GetEye()) };
	target = Vector3::Normalize(target);

	// 총구에서 나오도록
	start = Vector3::Add(start, Vector3::Mul(target, 8.0f));

	// 총알 생성
	unique_ptr<Bullet> bullet{ make_unique<Bullet>(target) };
	bullet->SetMesh(m_meshes["BULLET"]);
	bullet->SetShader(m_shaders["DEFAULT"]);
	bullet->SetPosition(start);
	m_gameObjects.push_back(move(bullet));

	// 총알 발사 정보 서버로 송신
	cs_packet_bullet_fire packet{};
	packet.size = sizeof(packet);
	packet.type = CS_PACKET_BULLET_FIRE;
	packet.data = { start, target };
	send(g_socket, reinterpret_cast<char*>(&packet), sizeof(packet), 0);
}

void Scene::ProcessClient(LPVOID arg)
{
	while (g_isConnected)
		RecvPacket();
}

void Scene::RecvPacket()
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
	case SC_PACKET_LOGIN_OK:
		RecvLoginOk();
		break;
	case SC_PACKET_UPDATE_CLIENT:
		RecvUpdateClient();
		break;
	case SC_PACKET_UPDATE_MONSTER:
		RecvUpdateMonster();
		break;
	case SC_PACKET_BULLET_FIRE:
		RecvBulletFire();
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

void Scene::RecvLoginOk()
{
	char subBuf[sizeof(PlayerData)]{};
	WSABUF wsabuf{ sizeof(subBuf), subBuf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	if (!m_player) return;

	PlayerData data;
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
			p->SetMesh(m_meshes["PLAYER"]);
			p->SetShader(m_shaders["ANIMATION"]);
			p->SetShadowShader(m_shaders["SHADOW_ANIMATION"]);
			p->SetGunMesh(m_meshes["SG"]);
			p->SetGunShader(m_shaders["LINK"]);
			p->SetGunShadowShader(m_shaders["SHADOW_LINK"]);
			p->SetGunType(eGunType::SG);
			p->PlayAnimation("IDLE");
			p->SetId(static_cast<int>(data.id));
			p->ApplyServerData(data);
			break;
		}
}

void Scene::RecvUpdateClient()
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

void Scene::RecvUpdateMonster()
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
			m_monsters[m.id]->SetMesh(m_meshes["GAROO"]);
			m_monsters[m.id]->SetShader(m_shaders["ANIMATION"]);
			m_monsters[m.id]->SetShadowShader(m_shaders["SHADOW_ANIMATION"]);
			m_monsters[m.id]->SetTexture(m_textures["GAROO"]);
			m_monsters[m.id]->PlayAnimation("IDLE");
		}
		m_monsters[m.id]->ApplyServerData(m);
	}
}

void Scene::RecvBulletFire()
{
	// pos, dir
	char subBuf[12 + 12]{};
	WSABUF wsabuf{ sizeof(subBuf), subBuf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	XMFLOAT3 pos{}, dir{};
	memcpy(&pos, &subBuf[0], sizeof(pos));
	memcpy(&dir, &subBuf[12], sizeof(dir));

	auto bullet{ make_unique<Bullet>(dir) };
	bullet->SetMesh(m_meshes["BULLET"]);
	bullet->SetShader(m_shaders["DEFAULT"]);
	bullet->SetPosition(pos);

	unique_lock<mutex> lock{ g_mutex };
	m_gameObjects.push_back(move(bullet));
}