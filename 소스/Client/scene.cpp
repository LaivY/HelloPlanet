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
	m_shadowMap = make_unique<ShadowMap>(device, 1 << 14, 1 << 14);

	// 블러 필터 생성
	//m_blurFilter = make_unique<BlurFilter>(device);

	// 조명, 재질 생성
	CreateLights();

	// UI 오브젝트 생성
	CreateUIObjects(device, commandList);

	// 게임오브젝트 생성
	CreateGameObjects(device, commandList);

	// 맵 로딩
	LoadMapObjects(device, commandList, Utile::PATH("map.txt", Utile::RESOURCE));
}

void Scene::OnMouseEvent(HWND hWnd, UINT width, UINT height, FLOAT deltaTime)
{
	// 화면 가운데 좌표 계산
	RECT rect; GetWindowRect(hWnd, &rect);
	POINT oldMousePosition{ static_cast<LONG>(rect.left + width / 2), static_cast<LONG>(rect.top + height / 2) };

	// 움직인 마우스 좌표
	POINT newMousePosition; GetCursorPos(&newMousePosition);

	// 움직인 정도에 비례해서 회전
	float sensitive{ 2.5f };
	int dx = newMousePosition.x - oldMousePosition.x;
	int dy = newMousePosition.y - oldMousePosition.y;
	if (dx == 0 && dy == 0) return;
	
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

void Scene::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (m_camera) m_camera->OnMouseEvent(hWnd, message, wParam, lParam);
	if (m_player) m_player->OnMouseEvent(hWnd, message, wParam, lParam);

	// 총알 발사 테스트
	switch (message)
	{
	case WM_LBUTTONDOWN:
	{
		if (m_player->GetCurrAnimationName() == "RUNNING")
			break;
		if (m_player->GetUpperCurrAnimationName() == "RELOAD")
			break;

		CreateBullet();
		break;
	}
	}
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
	if (wParam == VK_ESCAPE)
	{
		PostMessage(hWnd, WM_QUIT, 0, 0);
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
	for (auto& ui : m_uiObjects)
		ui->Update(deltaTime);
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

	// 몬스터 관련 로딩
	m_meshes["GAROO"] = make_shared<Mesh>();
	m_meshes["GAROO"]->LoadMesh(device, commandList, Utile::PATH("Mob/AlienGaroo/AlienGaroo.txt", Utile::RESOURCE));
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/attack.txt", Utile::RESOURCE), "ATTACK");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/die.txt", Utile::RESOURCE), "DIE");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/hit.txt", Utile::RESOURCE), "HIT");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/idle.txt", Utile::RESOURCE), "IDLE");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/run.txt", Utile::RESOURCE), "RUNNING");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/walkBack.txt", Utile::RESOURCE), "WALKBACK");
	m_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/walking.txt", Utile::RESOURCE), "WALKING");

	// 게임오브젝트 관련 로딩
	m_meshes["FLOOR"] = make_shared<RectMesh>(device, commandList, 2000.0f, 0.0f, 2000.0f, XMFLOAT3{}, XMFLOAT4{ 0.8f, 0.8f, 0.8f, 1.0f });
	m_meshes["BULLET"] = make_shared<CubeMesh>(device, commandList, 0.1f, 0.1f, 10.0f, XMFLOAT3{}, XMFLOAT4{ 39.0f / 255.0f, 151.0f / 255.0f, 255.0f / 255.0f, 1.0f });

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
	m_meshes["BB_MOB"] = make_shared<CubeMesh>(device, commandList, 7.0f, 7.0f, 10.0f, XMFLOAT3{ 0.0f, 8.0f, 0.0f }, XMFLOAT4{ 0.8f, 0.0f, 0.0f, 1.0f });
	m_meshes["BB_SMALLROCK"] = make_shared<CubeMesh>(device, commandList, 100.0f, 100.0f, 100.0f, XMFLOAT3{}, XMFLOAT4{ 0.8f, 0.0f, 0.0f, 1.0f });
}

void Scene::CreateShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature)
{
	m_shaders["DEFAULT"] = make_shared<Shader>(device, rootSignature, Utile::PATH(TEXT("default.hlsl"), Utile::SHADER), "VS", "PS");
	m_shaders["SKYBOX"] = make_shared<NoDepthShader>(device, rootSignature, Utile::PATH(TEXT("model.hlsl"), Utile::SHADER), "VS", "PSSkybox");
	m_shaders["MODEL"] = make_shared<Shader>(device, rootSignature, Utile::PATH(TEXT("model.hlsl"), Utile::SHADER), "VS", "PS");
	m_shaders["ANIMATION"] = make_shared<Shader>(device, rootSignature, Utile::PATH(TEXT("animation.hlsl"), Utile::SHADER), "VS", "PS");
	m_shaders["LINK"] = make_shared<Shader>(device, rootSignature, Utile::PATH(TEXT("link.hlsl"), Utile::SHADER), "VS", "PS");
	m_shaders["UI"] = make_shared<BlendingShader>(device, rootSignature, Utile::PATH(TEXT("default.hlsl"), Utile::SHADER), "VS", "PS");

	// 그림자 셰이더
	m_shaders["SHADOW_MODEL"] = make_shared<ShadowShader>(device, rootSignature, Utile::PATH(TEXT("shadow.hlsl"), Utile::SHADER), "VS_MODEL");
	m_shaders["SHADOW_LINK"] = make_shared<ShadowShader>(device, rootSignature, Utile::PATH(TEXT("shadow.hlsl"), Utile::SHADER), "VS_LINK");
	m_shaders["SHADOW_ANIMATION"] = make_shared<ShadowShader>(device, rootSignature, Utile::PATH(TEXT("shadow.hlsl"), Utile::SHADER), "VS_ANIMATION");

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
}

void Scene::CreateLights() const
{
	// 첫번째 조명은 그림자를 만듦
	m_cbSceneData->ligths[0].color = XMFLOAT3{ 0.1f, 0.1f, 0.1f };
	m_cbSceneData->ligths[0].direction = Vector3::Normalize(XMFLOAT3{ -0.687586f, -0.716385f, 0.118001f });

	XMFLOAT3 at{ m_cbSceneData->ligths[0].direction };
	XMFLOAT3 pos{ Vector3::Mul(at, -2000.0f) };
	XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };
	XMFLOAT4X4 lightViewMatrix, lightProjMatrix;
	XMStoreFloat4x4(&lightViewMatrix, XMMatrixLookAtLH(XMLoadFloat3(&pos), XMLoadFloat3(&at), XMLoadFloat3(&up)));
	XMStoreFloat4x4(&lightProjMatrix, XMMatrixOrthographicLH(3000.0f, 3000.0f, 0.0f, 5000.0f));
	m_cbSceneData->ligths[0].lightViewMatrix = Matrix::Transpose(lightViewMatrix);
	m_cbSceneData->ligths[0].lightProjMatrix = Matrix::Transpose(lightProjMatrix);

	m_cbSceneData->ligths[1].color = XMFLOAT3{ 0.05f, 0.05f, 0.05f };
	m_cbSceneData->ligths[1].direction = XMFLOAT3{ 0.0f, -6.0f, 10.0f };
}

void Scene::CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 체력바
	auto hpBar{ make_unique<UIObject>(300.0f, 40.0f) };
	hpBar->SetMesh(m_meshes["HPBAR"]);
	hpBar->SetShader(m_shaders["UI"]);
	hpBar->SetPivot(eUIPivot::LEFTBOT);
	hpBar->SetPosition(-static_cast<float>(Setting::SCREEN_WIDTH) / 2.0f + 20.0f,
					   -static_cast<float>(Setting::SCREEN_HEIGHT) / 2.0f + 20.0f);
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
	XMStoreFloat4x4(&projMatrix, XMMatrixPerspectiveFovLH(0.25f * XM_PI, static_cast<float>(Setting::SCREEN_WIDTH) / static_cast<float>(Setting::SCREEN_HEIGHT), 1.0f, 5000.0f));
	m_camera->SetProjMatrix(projMatrix);

	// UI 카메라 생성
	m_uiCamera = make_unique<Camera>();
	m_uiCamera->CreateShaderVariable(device, commandList);
	XMStoreFloat4x4(&projMatrix, XMMatrixOrthographicLH(static_cast<float>(Setting::SCREEN_WIDTH), static_cast<float>(Setting::SCREEN_HEIGHT), 0.0f, 1.0f));
	m_uiCamera->SetProjMatrix(projMatrix);

	// 바운딩박스
	SharedBoundingBox bbPlayer{ make_shared<DebugBoundingBox>(XMFLOAT3{ 0.0f, 32.5f / 2.0f, 0.0f }, XMFLOAT3{ 8.0f / 2.0f, 32.5f / 2.0f, 8.0f / 2.0f }, XMFLOAT4{ 0.0f, 0.0f, 0.0f, 1.0f }) };
	bbPlayer->SetMesh(m_meshes["BB_PLAYER"]);
	bbPlayer->SetShader(m_shaders["WIREFRAME"]);

	SharedBoundingBox bbMob{ make_shared<DebugBoundingBox>(XMFLOAT3{}, XMFLOAT3{}, XMFLOAT4{}) };
	bbMob->SetMesh(m_meshes["BB_MOB"]);
	bbMob->SetShader(m_shaders["WIREFRAME"]);

	// 플레이어 생성
	m_player = make_shared<Player>();
	m_player->SetMesh(m_meshes["PLAYER"]);
	m_player->SetShader(m_shaders["ANIMATION"]);
	m_player->SetShadowShader(m_shaders["SHADOW_ANIMATION"]);
	m_player->SetGunMesh(m_meshes["SG"]);
	m_player->SetGunShader(m_shaders["LINK"]);
	m_player->SetGunShadowShader(m_shaders["SHADOW_LINK"]);
	m_player->SetGunType(ePlayerGunType::SG);
	m_player->PlayAnimation("IDLE");
	m_player->AddBoundingBox(bbPlayer);

	// 카메라, 플레이어 서로 설정
	m_camera->SetPlayer(m_player);
	m_player->SetCamera(m_camera);

	// 멀티플레이어
	for (unique_ptr<Player>& p : m_multiPlayers)
	{
		p = make_unique<Player>(TRUE);
		p->SetMesh(m_meshes["PLAYER"]);
		p->SetShader(m_shaders["ANIMATION"]);
		p->SetShadowShader(m_shaders["SHADOW_ANIMATION"]);
		p->SetGunMesh(m_meshes["AR"]);
		p->SetGunShader(m_shaders["LINK"]);
		p->SetGunShadowShader(m_shaders["SHADOW_LINK"]);
		p->SetGunType(ePlayerGunType::AR);
		p->PlayAnimation("IDLE");
		p->AddBoundingBox(bbPlayer);
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
	floor->SetTexture(m_textures["FLOOR"]);
	m_gameObjects.push_back(move(floor));
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
			//object->SetMesh(m_meshes["PLANT"]);
			//object->SetTexture(m_textures["OBJECT1"]);
			break;
		case eMapObjectType::TREE:
			//object->SetMesh(m_meshes["TREE"]);
			//object->SetTexture(m_textures["OBJECT2"]);
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
			//object->SetMesh(m_meshes["DROPSHIP"]);
			//object->SetTexture(m_textures["OBJECT3"]);
			break;
		case eMapObjectType::MUSHROOMS:
			//object->SetMesh(m_meshes["MUSHROOMS"]);
			//object->SetTexture(m_textures["OBJECT1"]);
			break;
		case eMapObjectType::SKULL:
			//object->SetMesh(m_meshes["SKULL"]);
			//object->SetTexture(m_textures["OBJECT2"]);
			break;
		case eMapObjectType::RIBS:
			//object->SetMesh(m_meshes["RIBS"]);
			//object->SetTexture(m_textures["OBJECT2"]);
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
	// 게임오브젝트 삭제
	m_gameObjects.erase(remove_if(m_gameObjects.begin(), m_gameObjects.end(),
		[](unique_ptr<GameObject>& object) {
			return object->isDeleted();
		}), m_gameObjects.end());

	// 플레이어 충돌판정
	for (const auto& object : m_gameObjects)
	{
		const auto& boundingBoxes{ object->GetBoundingBox() };
		for (const auto& bb : boundingBoxes)
		{
			const auto& pb{ m_player->GetBoundingBox() };
			BoundingOrientedBox a{ *pb.front() };
			BoundingOrientedBox b{ *bb };

			a.Transform(a, XMLoadFloat4x4(&m_player->GetWorldMatrix()));
			b.Transform(b, XMLoadFloat4x4(&object->GetWorldMatrix()));

			if (a.Intersects(b))
			{
				XMFLOAT3 v{ Vector3::Sub(m_player->GetPosition(), object->GetPosition()) };
				v = Vector3::Normalize(v);

				m_player->Move(Vector3::Mul(v, Vector3::Length(m_player->GetVelocity()) * deltaTime));

				static int i = 0;
				string debug{ "INTERSECT! (" + to_string(i++) + ")\n" };
				OutputDebugStringA(debug.c_str());
			}
		}
	}
}

void Scene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	// 뷰포트, 가위사각형, 렌더타겟 설정
	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);
	commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

	// 스카이박스 렌더링
	if (m_skybox) m_skybox->Render(commandList);

	// 멀티플레이어 렌더링
	for (const auto& p : m_multiPlayers)
		if (p) p->Render(commandList);

	// 게임오브젝트 렌더링
	for (const auto& o : m_gameObjects)
		o->Render(commandList);

	// 플레이어 렌더링
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
	if (m_player) m_player->Render(commandList);

	// UI 렌더링
	//if (m_uiCamera)
	//{
	//	m_uiCamera->UpdateShaderVariable(commandList);
	//	for (const auto& ui : m_uiObjects)
	//		ui->Render(commandList);
	//}
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

	// 셰이더에 묶기
	ID3D12DescriptorHeap* ppHeaps[] = { m_shadowMap->GetSrvHeap().Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandList->SetGraphicsRootDescriptorTable(6, m_shadowMap->GetGpuSrvHandle());

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
	for (const auto& object : m_gameObjects)
	{
		auto shadowShader{ object->GetShadowShader() };
		if (shadowShader)
			object->Render(commandList, shadowShader);
	}
	for (const auto& player : m_multiPlayers)
	{
		if (!player) continue;
		auto shadowShader{ player->GetShadowShader() };
		if (shadowShader)
			player->RenderToShadowMap(commandList);
	}
	if (m_player) m_player->RenderToShadowMap(commandList);

	// 리소스배리어 설정(셰이더에서 읽기)
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_shadowMap->GetShadowMap().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
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
	DWORD recv_byte{ 0 }, recv_flag{ 0 };
	int error_code = WSARecv(g_socket, &wsabuf, 1, &recv_byte, &recv_flag, nullptr, nullptr);
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

	string debug{ to_string(g_isConnected) + "\n" };
	OutputDebugStringA(debug.c_str());
}

void Scene::RecvLoginOk()
{
	char subBuf[sizeof(PlayerData)]{};
	WSABUF wsabuf{ sizeof(subBuf), subBuf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	// 내 플레이어 id 설정
	if (m_player)
	{
		PlayerData data;
		memcpy(&data, subBuf, sizeof(data));
		m_player->SetId(static_cast<int>(data.id));
	}

	// 다른 플레이어 id 설정
	int id{ 0 };
	for (auto& p : m_multiPlayers)
	{
		if (id == m_player->GetId()) ++id;
		p->SetId(id++);
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
	// test size = 2 / char id + char type
	char subBuf[2 + 2]{};
	WSABUF wsabuf{ sizeof(subBuf), subBuf };
	DWORD recvByte{}, recvFlag{};
	WSARecv(g_socket, &wsabuf, 1, &recvByte, &recvFlag, nullptr, nullptr);

	cout << static_cast<int>(subBuf[0]) << ", " << static_cast<int>(subBuf[1]) << ", " <<
	static_cast<int>(subBuf[2]) << ", " << static_cast<int>(subBuf[3]) << ", " << endl;
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
	m_gameObjects.push_back(move(bullet));
}