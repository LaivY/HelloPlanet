#include "loadingScene.h"
#include "framework.h"
#include "textObject.h"

LoadingScene::LoadingScene() : m_isDone{ FALSE }
{

}

LoadingScene::~LoadingScene()
{
	for (auto& [_, m] : s_meshes)
		m->ReleaseUploadBuffer();
	for (auto& [_, t] : s_textures)
		t->ReleaseUploadBuffer();
}

void LoadingScene::OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, 
						  const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature,
						  const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	CreateMeshes(device, commandList);
	CreateShaders(device, rootSignature, postProcessRootSignature);
	CreateTextures(device, commandList);
	CreateUIObjects(device, commandList);

	// 쓰레드가 사용할 커맨드리스트 생성
	DX::ThrowIfFailed(g_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
	DX::ThrowIfFailed(g_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

	// 로딩 쓰레드 시작
	m_thread = thread{ &LoadingScene::LoadResources, this, device, m_commandList, rootSignature, postProcessRootSignature, d2dDeivceContext, dWriteFactory };
}

void LoadingScene::OnUpdate(FLOAT deltaTime)
{
	Update(deltaTime);
}

void LoadingScene::Update(FLOAT deltaTime)
{
	constexpr size_t allResourceCount{ 25 + 11 + 10 + 4 + 5 };
	size_t currResourceCount{};
	currResourceCount += s_meshes.size();
	currResourceCount += s_shaders.size();
	currResourceCount += s_textures.size();
	currResourceCount += TextObject::s_brushes.size();
	currResourceCount += TextObject::s_formats.size();

	if (m_loadingBarObject)
	{
		float width{ 200.0f * static_cast<float>(currResourceCount) / static_cast<float>(allResourceCount) };
		m_loadingBarObject->SetWidth(width);
	}

	if (m_isDone && m_thread.joinable())
	{
		m_thread.join();
		g_gameFramework.SetNextScene(eSceneType::MAIN);
	}
}

void LoadingScene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
	if (m_camera) m_camera->UpdateShaderVariable(commandList);
}

void LoadingScene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const
{
	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);
	commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

	for (const auto& ui : m_uiObjects)
		ui->Render(commandList);
	if (m_loadingBarObject)
		m_loadingBarObject->Render(commandList);
}

void LoadingScene::CreateMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	s_meshes["UI"] = make_shared<RectMesh>(device, commandList, 1.0f, 1.0f, 0.0f, XMFLOAT3{ 0.0f, 0.0f, 1.0f });
}

void LoadingScene::CreateShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature)
{
	s_shaders["UI"] = make_shared<BlendingShader>(device, rootSignature, Utile::PATH(TEXT("Shader/ui.hlsl")), "VS", "PS");
}

void LoadingScene::CreateTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	s_textures["HPBARBASE"] = make_shared<Texture>();
	s_textures["HPBARBASE"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("UI/HPBarBase.dds")));
	s_textures["HPBARBASE"]->CreateTexture(device);

	s_textures["HPBAR"] = make_shared<Texture>();
	s_textures["HPBAR"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("UI/HPBar.dds")));
	s_textures["HPBAR"]->CreateTexture(device);
}

void LoadingScene::CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMFLOAT4X4 projMatrix{};
	m_camera = make_unique<Camera>();
	m_camera->CreateShaderVariable(device, commandList);
	XMStoreFloat4x4(&projMatrix, XMMatrixOrthographicLH(static_cast<float>(g_width), static_cast<float>(g_height), 0.0f, 1.0f));
	m_camera->SetProjMatrix(projMatrix);

	// 로딩바 베이스
	auto loadingBarBase{ make_unique<UIObject>(200.0f, 10.0f) };
	loadingBarBase->SetMesh(s_meshes["UI"]);
	loadingBarBase->SetShader(s_shaders["UI"]);
	loadingBarBase->SetTexture(s_textures["HPBARBASE"]);
	loadingBarBase->SetPivot(ePivot::LEFTCENTER);
	loadingBarBase->SetScreenPivot(ePivot::RIGHTBOT);
	loadingBarBase->SetPosition(XMFLOAT2{ -250.0f, 50.0f });
	m_uiObjects.push_back(move(loadingBarBase));

	// 로딩바
	m_loadingBarObject = make_unique<UIObject>(200.0f, 10.0f);
	m_loadingBarObject->SetMesh(s_meshes["UI"]);
	m_loadingBarObject->SetShader(s_shaders["UI"]);
	m_loadingBarObject->SetTexture(s_textures["HPBAR"]);
	m_loadingBarObject->SetPivot(ePivot::LEFTCENTER);
	m_loadingBarObject->SetScreenPivot(ePivot::RIGHTBOT);
	m_loadingBarObject->SetPosition(XMFLOAT2{ -250.0f, 50.0f });
}

void LoadingScene::LoadResources(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, 
								 const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature,
								 const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory)
{
	LoadMeshes(device, commandList);
	LoadShaders(device, rootSignature, postProcessRootSignature);
	LoadTextures(device, commandList);
	LoadTextBurshes(d2dDeivceContext);
	LoadTextFormats(dWriteFactory);

	commandList->Close();
	ID3D12CommandList* ppCommandList[] = { commandList.Get() };
	g_gameFramework.GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);
	g_gameFramework.WaitForPreviousFrame();
	m_isDone = TRUE;
}

void LoadingScene::LoadMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 플레이어 관련 메쉬 로딩
	vector<pair<string, string>> animations
	{
		{ "idle", "IDLE" }, { "walking", "WALKING" }, {"walkLeft", "WALKLEFT" }, { "walkRight", "WALKRIGHT" },
		{ "walkBack", "WALKBACK" }, { "running", "RUNNING" }, {"firing", "FIRING" }, { "reload", "RELOAD" }
	};
	s_meshes["PLAYER"] = make_shared<Mesh>();
	s_meshes["PLAYER"]->LoadMeshBinary(device, commandList, Utile::PATH("player.bin"));
	for (const string& weaponName : { "AR", "SG", "MG" })
		for (const auto& [fileName, animationName] : animations)
			s_meshes["PLAYER"]->LoadAnimationBinary(device, commandList, Utile::PATH(weaponName + "/" + fileName + ".bin"), weaponName + "/" + animationName);

	s_meshes["ARM"] = make_shared<Mesh>();
	s_meshes["ARM"]->LoadMeshBinary(device, commandList, Utile::PATH("arm.bin"));
	s_meshes["ARM"]->Link(s_meshes["PLAYER"]);
	s_meshes["AR"] = make_shared<Mesh>();
	s_meshes["AR"]->LoadMeshBinary(device, commandList, Utile::PATH("AR/AR.bin"));
	s_meshes["AR"]->Link(s_meshes["PLAYER"]);
	s_meshes["SG"] = make_shared<Mesh>();
	s_meshes["SG"]->LoadMeshBinary(device, commandList, Utile::PATH("SG/SG.bin"));
	s_meshes["SG"]->Link(s_meshes["PLAYER"]);
	s_meshes["MG"] = make_shared<Mesh>();
	s_meshes["MG"]->LoadMeshBinary(device, commandList, Utile::PATH("MG/MG.bin"));
	s_meshes["MG"]->Link(s_meshes["PLAYER"]);

	// 몬스터 관련 로딩
	s_meshes["GAROO"] = make_shared<Mesh>();
	s_meshes["GAROO"]->LoadMesh(device, commandList, Utile::PATH("Mob/AlienGaroo/AlienGaroo.txt"));
	s_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/attack.txt"), "ATTACK");
	s_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/die.txt"), "DIE");
	s_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/hit.txt"), "HIT");
	s_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/idle.txt"), "IDLE");
	s_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/running.txt"), "RUNNING");
	s_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/walkBack.txt"), "WALKBACK");
	s_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/AlienGaroo/walking.txt"), "WALKING");

	// 게임오브젝트 관련 로딩
	s_meshes["FLOOR"] = make_shared<RectMesh>(device, commandList, 2000.0f, 0.0f, 2000.0f, XMFLOAT3{}, XMFLOAT4{ 217.0f / 255.0f, 112.0f / 255.0f, 61.0f / 255.0f, 1.0f });
	s_meshes["BULLET"] = make_shared<CubeMesh>(device, commandList, 0.05f, 0.05f, 10.0f, XMFLOAT3{ 0.0f, 0.0f, 5.0f }, XMFLOAT4{ 39.0f / 255.0f, 151.0f / 255.0f, 255.0f / 255.0f, 1.0f });

	// 맵 오브젝트 관련 로딩
	for (int i = 0; i <= 11; ++i)
	{
		string n{ to_string(i) };
		s_meshes["OBJECT" + n] = make_shared<Mesh>();
		s_meshes["OBJECT" + n]->LoadMeshBinary(device, commandList, Utile::PATH("Object/object" + n + ".bin"));
	}

	// 게임 시스템 관련 로딩
	s_meshes["SKYBOX"] = make_shared<Mesh>();
	s_meshes["SKYBOX"]->LoadMesh(device, commandList, Utile::PATH("Skybox/Skybox.txt"));

	// 히트박스 메쉬
	s_meshes["CUBE"] = make_shared<CubeMesh>(device, commandList, 1.0f, 1.0f, 1.0f, XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT4{ 0.8f, 0.0f, 0.0f, 1.0f });
}

void LoadingScene::LoadShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	s_shaders["DEFAULT"] = make_shared<Shader>(device, rootSignature, Utile::PATH(TEXT("Shader/default.hlsl")), "VS", "PS");
	s_shaders["SKYBOX"] = make_shared<NoDepthShader>(device, rootSignature, Utile::PATH(TEXT("Shader/model.hlsl")), "VS", "PSSkybox");
	s_shaders["MODEL"] = make_shared<Shader>(device, rootSignature, Utile::PATH(TEXT("Shader/model.hlsl")), "VS", "PS");
	s_shaders["ANIMATION"] = make_shared<Shader>(device, rootSignature, Utile::PATH(TEXT("Shader/animation.hlsl")), "VS", "PS");
	s_shaders["LINK"] = make_shared<Shader>(device, rootSignature, Utile::PATH(TEXT("Shader/link.hlsl")), "VS", "PS");
	s_shaders["UI"] = make_shared<BlendingShader>(device, rootSignature, Utile::PATH(TEXT("Shader/ui.hlsl")), "VS", "PS");
	s_shaders["UI_ATC"] = make_shared<BlendingShader>(device, rootSignature, Utile::PATH(TEXT("Shader/ui.hlsl")), "VS", "PS", true);

	// 그림자 셰이더
	s_shaders["SHADOW_MODEL"] = make_shared<ShadowShader>(device, rootSignature, Utile::PATH(TEXT("Shader/shadow.hlsl")), "VS_MODEL", "GS");
	s_shaders["SHADOW_ANIMATION"] = make_shared<ShadowShader>(device, rootSignature, Utile::PATH(TEXT("Shader/shadow.hlsl")), "VS_ANIMATION", "GS");
	s_shaders["SHADOW_LINK"] = make_shared<ShadowShader>(device, rootSignature, Utile::PATH(TEXT("Shader/shadow.hlsl")), "VS_LINK", "GS");

	// 외곽선
	s_shaders["OUTLINE"] = make_shared<OutlineShader>(device, postRootSignature, Utile::PATH(TEXT("Shader/outline.hlsl")), "CS");

	// 디버그
	s_shaders["WIREFRAME"] = make_shared<WireframeShader>(device, rootSignature, Utile::PATH(TEXT("Shader/default.hlsl")), "VS", "PS");
}

void LoadingScene::LoadTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	s_textures["SKYBOX"] = make_shared<Texture>();
	s_textures["SKYBOX"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("Skybox/skybox.dds")));
	s_textures["SKYBOX"]->CreateTexture(device);

	s_textures["GAROO"] = make_shared<Texture>();
	s_textures["GAROO"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("Mob/AlienGaroo/texture.dds")));
	s_textures["GAROO"]->CreateTexture(device);

	s_textures["OBJECT0"] = make_shared<Texture>();
	s_textures["OBJECT0"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("Object/texture0.dds")));
	s_textures["OBJECT0"]->CreateTexture(device);
	s_textures["OBJECT1"] = make_shared<Texture>();
	s_textures["OBJECT1"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("Object/texture1.dds")));
	s_textures["OBJECT1"]->CreateTexture(device);
	s_textures["OBJECT2"] = make_shared<Texture>();
	s_textures["OBJECT2"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("Object/texture2.dds")));
	s_textures["OBJECT2"]->CreateTexture(device);

	s_textures["WHITE"] = make_shared<Texture>();
	s_textures["WHITE"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("UI/white.dds")));
	s_textures["WHITE"]->CreateTexture(device);

	s_textures["TITLE"] = make_shared<Texture>();
	s_textures["TITLE"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("UI/title.dds")));
	s_textures["TITLE"]->CreateTexture(device);

	s_textures["HIT"] = make_shared<Texture>();
	s_textures["HIT"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("UI/hit1.dds")));
	s_textures["HIT"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("UI/hit2.dds")));
	s_textures["HIT"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("UI/hit3.dds")));
	s_textures["HIT"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("UI/hit4.dds")));
	s_textures["HIT"]->CreateTexture(device);

	//s_textures["HPBARBASE"] = make_shared<Texture>();
	//s_textures["HPBARBASE"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("UI/HPBarBase.dds")));
	//s_textures["HPBARBASE"]->CreateTexture(device);

	//s_textures["HPBAR"] = make_shared<Texture>();
	//s_textures["HPBAR"]->LoadTextureFile(device, commandList, 5, Utile::PATH(TEXT("UI/HPBar.dds")));
	//s_textures["HPBAR"]->CreateTexture(device);
}

void LoadingScene::LoadTextBurshes(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext)
{
	DX::ThrowIfFailed(d2dDeivceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &TextObject::s_brushes["BLACK"]));
	DX::ThrowIfFailed(d2dDeivceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &TextObject::s_brushes["RED"]));
	DX::ThrowIfFailed(d2dDeivceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DeepSkyBlue), &TextObject::s_brushes["BLUE"]));
	DX::ThrowIfFailed(d2dDeivceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &TextObject::s_brushes["WHITE"]));
}

void LoadingScene::LoadTextFormats(const ComPtr<IDWriteFactory>& dWriteFactory)
{
	// 왼쪽 정렬 포멧
	for (int size : { 24, 36 })
	{
		DX::ThrowIfFailed(dWriteFactory->CreateTextFormat(
			TEXT("나눔바른고딕OTF"), NULL,
			DWRITE_FONT_WEIGHT_ULTRA_BOLD,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			static_cast<float>(size),
			TEXT("ko-kr"),
			&TextObject::s_formats[to_string(size) + "_LEFT"]
		));
		DX::ThrowIfFailed(TextObject::s_formats[to_string(size) + "_LEFT"]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING));
		DX::ThrowIfFailed(TextObject::s_formats[to_string(size) + "_LEFT"]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR));
	}

	// 오른쪽 정렬 포멧
	for (int size : { 24, 36, 48 })
	{
		DX::ThrowIfFailed(dWriteFactory->CreateTextFormat(
			TEXT("나눔바른고딕OTF"), NULL,
			DWRITE_FONT_WEIGHT_ULTRA_BOLD,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			static_cast<float>(size),
			TEXT("ko-kr"),
			&TextObject::s_formats[to_string(size) + "_RIGHT"]
		));
		DX::ThrowIfFailed(TextObject::s_formats[to_string(size) + "_RIGHT"]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING));
		DX::ThrowIfFailed(TextObject::s_formats[to_string(size) + "_RIGHT"]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR));
	}
}
