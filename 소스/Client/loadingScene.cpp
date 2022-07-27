#include "stdafx.h"
#include "loadingScene.h"
#include "audioEngine.h"
#include "camera.h"
#include "framework.h"
#include "mesh.h"
#include "shader.h"
#include "textObject.h"
#include "texture.h"
#include "uiObject.h"

LoadingScene::LoadingScene() : m_isDone{ FALSE }
{

}

LoadingScene::~LoadingScene()
{
	for (auto& [_, m] : s_meshes)
		m->ReleaseUploadBuffer();
	Texture::ReleaseUploadBuffer();
	Texture::CreateShaderResourceView(g_device);
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
	DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
	DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));

	// 로딩 쓰레드 시작
	m_thread = thread{ &LoadingScene::LoadResources, this, device, m_commandList, rootSignature, postProcessRootSignature, d2dDeivceContext, dWriteFactory };
}

void LoadingScene::OnUpdate(FLOAT deltaTime)
{
	Update(deltaTime);
}

void LoadingScene::Update(FLOAT deltaTime)
{
	constexpr size_t allResourceCount{ 32 + 20 + 26 + 4 + 10 + 11 };
	size_t currResourceCount{ 0 };
	currResourceCount += s_meshes.size();
	currResourceCount += s_shaders.size();
	currResourceCount += s_textures.size();
	currResourceCount += TextObject::s_brushes.size();
	currResourceCount += TextObject::s_formats.size();
	currResourceCount += g_audioEngine.GetAudioCount();

	// 로딩바 최신화
	if (m_loadingBarObject)
	{
		float width{ 200.0f * static_cast<float>(currResourceCount) / static_cast<float>(allResourceCount) };
		m_loadingBarObject->SetWidth(width);
	}

	// 로딩이 끝났으면 다음 씬으로
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
	s_shaders["FADE"] = make_shared<ComputeShader>(device, postProcessRootSignature, Utile::PATH(TEXT("Shader/fade.hlsl")), "CS");
}

void LoadingScene::CreateTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	s_textures["HPBARBASE"] = make_shared<Texture>();
	s_textures["HPBARBASE"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/HPBAR_BASE.dds")));

	s_textures["HPBAR"] = make_shared<Texture>();
	s_textures["HPBAR"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/HPBAR.dds")));
	Texture::CreateShaderResourceView(device);
}

void LoadingScene::CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// UI 카메라
	XMFLOAT4X4 projMatrix{};
	m_camera = make_unique<Camera>();
	m_camera->CreateShaderVariable(device, commandList);
	XMStoreFloat4x4(&projMatrix, XMMatrixOrthographicLH(static_cast<float>(g_width), static_cast<float>(g_height), 0.0f, 1.0f));
	m_camera->SetProjMatrix(projMatrix);

	// 로딩바 베이스
	auto loadingBarBase{ make_unique<UIObject>(200.0f, 10.0f) };
	loadingBarBase->SetTexture(s_textures["HPBARBASE"]);
	loadingBarBase->SetPivot(ePivot::LEFTCENTER);
	loadingBarBase->SetScreenPivot(ePivot::RIGHTBOT);
	loadingBarBase->SetPosition(XMFLOAT2{ -250.0f, 50.0f });
	m_uiObjects.push_back(move(loadingBarBase));

	// 로딩바
	m_loadingBarObject = make_unique<UIObject>(200.0f, 10.0f);
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
	LoadAudios();

	commandList->Close();
	ID3D12CommandList* ppCommandList[]{ commandList.Get() };
	g_gameFramework.GetCommandQueue()->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);
	g_gameFramework.WaitForPreviousFrame();
	m_isDone = TRUE;
}

void LoadingScene::LoadMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 플레이어 관련 메쉬 로딩
	vector<pair<string, string>> animations
	{
		{ "idle", "IDLE" }, { "walking", "WALKING" }, { "walkLeft", "WALKLEFT" }, { "walkRight", "WALKRIGHT" }, { "walkBack", "WALKBACK" },
		{ "running", "RUNNING" }, { "firing", "FIRING" }, { "reload", "RELOAD" }, { "hit", "HIT" }, { "die", "DIE" }
	};
	s_meshes["PLAYER"] = make_shared<Mesh>();
	s_meshes["PLAYER"]->LoadMeshBinary(device, commandList, Utile::PATH("Player/player.bin"));
	for (const string& weaponName : { "AR", "SG", "MG" })
		for (const auto& [fileName, animationName] : animations)
			s_meshes["PLAYER"]->LoadAnimationBinary(device, commandList, Utile::PATH("Player/" + weaponName + "/" + fileName + ".bin"), weaponName + "/" + animationName);

	s_meshes["ARM"] = make_shared<Mesh>();
	s_meshes["ARM"]->LoadMeshBinary(device, commandList, Utile::PATH("Player/arm.bin"));
	s_meshes["ARM"]->Link(s_meshes["PLAYER"]);
	s_meshes["AR"] = make_shared<Mesh>();
	s_meshes["AR"]->LoadMeshBinary(device, commandList, Utile::PATH("Player/AR/AR.bin"));
	s_meshes["AR"]->Link(s_meshes["PLAYER"]);
	s_meshes["SG"] = make_shared<Mesh>();
	s_meshes["SG"]->LoadMeshBinary(device, commandList, Utile::PATH("Player/SG/SG.bin"));
	s_meshes["SG"]->Link(s_meshes["PLAYER"]);
	s_meshes["MG"] = make_shared<Mesh>();
	s_meshes["MG"]->LoadMeshBinary(device, commandList, Utile::PATH("Player/MG/MG.bin"));
	s_meshes["MG"]->Link(s_meshes["PLAYER"]);

	// 몬스터 관련 로딩
	s_meshes["GAROO"] = make_shared<Mesh>();
	s_meshes["GAROO"]->LoadMesh(device, commandList, Utile::PATH("Mob/Garoo/AlienGaroo.txt"));
	s_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/Garoo/attack.txt"), "ATTACK");
	s_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/Garoo/die.txt"), "DIE");
	s_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/Garoo/hit.txt"), "HIT");
	s_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/Garoo/idle.txt"), "IDLE");
	s_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/Garoo/running.txt"), "RUNNING");
	s_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/Garoo/walkBack.txt"), "WALKBACK");
	s_meshes["GAROO"]->LoadAnimation(device, commandList, Utile::PATH("Mob/Garoo/walking.txt"), "WALKING");

	s_meshes["SERPENT"] = make_shared<Mesh>();
	s_meshes["SERPENT"]->LoadMeshBinary(device, commandList, Utile::PATH("Mob/Serpent/AlienSerpent.bin"));
	s_meshes["SERPENT"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Serpent/attack.bin"), "ATTACK");
	s_meshes["SERPENT"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Serpent/die.bin"), "DIE");
	s_meshes["SERPENT"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Serpent/hit.bin"), "HIT");
	s_meshes["SERPENT"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Serpent/idle.bin"), "IDLE");
	s_meshes["SERPENT"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Serpent/walking.bin"), "WALKING");

	s_meshes["HORROR"] = make_shared<Mesh>();
	s_meshes["HORROR"]->LoadMeshBinary(device, commandList, Utile::PATH("Mob/Horror/AlienHorror.bin"));
	s_meshes["HORROR"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Horror/attack.bin"), "ATTACK");
	s_meshes["HORROR"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Horror/die.bin"), "DIE");
	s_meshes["HORROR"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Horror/hit.bin"), "HIT");
	s_meshes["HORROR"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Horror/idle.bin"), "IDLE");
	s_meshes["HORROR"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Horror/walking.bin"), "WALKING");

	s_meshes["ULIFO"] = make_shared<Mesh>();
	s_meshes["ULIFO"]->LoadMeshBinary(device, commandList, Utile::PATH("Mob/Ulifo/AlienUlifo.bin"));
	s_meshes["ULIFO"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Ulifo/die.bin"), "DIE");
	s_meshes["ULIFO"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Ulifo/down.bin"), "DOWN");
	s_meshes["ULIFO"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Ulifo/hit.bin"), "HIT");
	s_meshes["ULIFO"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Ulifo/idle.bin"), "IDLE");
	s_meshes["ULIFO"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Ulifo/jumpAttack.bin"), "JUMPATK");
	s_meshes["ULIFO"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Ulifo/legAttack.bin"), "LEGATK");
	s_meshes["ULIFO"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Ulifo/rest.bin"), "REST");
	s_meshes["ULIFO"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Ulifo/roar.bin"), "ROAR");
	s_meshes["ULIFO"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Ulifo/standup.bin"), "STANDUP");
	s_meshes["ULIFO"]->LoadAnimationBinary(device, commandList, Utile::PATH("Mob/Ulifo/walking.bin"), "WALKING");

	// 게임오브젝트 관련 로딩
	s_meshes["FLOOR"] = make_shared<RectMesh>(device, commandList, 2000.0f, 0.0f, 2000.0f, XMFLOAT3{}, XMFLOAT4{ 217.0f / 255.0f, 112.0f / 255.0f, 61.0f / 255.0f, 1.0f });
	s_meshes["BULLET"] = make_shared<CubeMesh>(device, commandList, 0.1f, 0.1f, 10.0f, XMFLOAT3{ 0.0f, 0.0f, 5.0f }, XMFLOAT4{ 39.0f / 255.0f, 151.0f / 255.0f, 255.0f / 255.0f, 1.0f });
	s_meshes["BULLET2"] = make_shared<CubeMesh>(device, commandList, 0.2f, 0.2f, 10.0f, XMFLOAT3{ 0.0f, 0.0f, 5.0f }, XMFLOAT4{ 39.0f / 255.0f, 151.0f / 255.0f, 255.0f / 255.0f, 1.0f });

	// 맵 오브젝트 관련 로딩
	for (int i = 0; i <= 11; ++i)
	{
		string n{ to_string(i) };
		s_meshes["OBJECT" + n] = make_shared<Mesh>();
		s_meshes["OBJECT" + n]->LoadMeshBinary(device, commandList, Utile::PATH("Object/object" + n + ".bin"));
	}

	// 게임 시스템 관련 로딩
	s_meshes["SKYBOX"] = make_shared<Mesh>();
	s_meshes["SKYBOX"]->LoadMesh(device, commandList, Utile::PATH("Object/Skybox.txt"));

	// 테두리
	s_meshes["FULLSCREEN"] = make_shared<FullScreenQuadMesh>(device, commandList);

	// 파티클
	s_meshes["DUST"] = make_shared<DustParticleMesh>(device, commandList);
	s_meshes["TRAIL"] = make_shared<TrailParticleMesh>(device, commandList, XMFLOAT3{ 0.0f, 20.0f, 0.0f }, XMFLOAT3{ 0.0f, 0.0f, 1.0f });

	// 총구 이펙트
	s_meshes["MUZZLE_FRONT"] = make_shared<RectMesh>(device, commandList, 30.0f, 30.0f, 0.0f, XMFLOAT3{ 0.0f, 0.0f, 0.01f });
	s_meshes["MUZZLE_SIDE"] = make_shared<RectMesh>(device, commandList, 50.0f, 30.0f, 0.0f, XMFLOAT3{ 0.0f, 0.0f, 0.01f });

	// 히트박스 메쉬
	s_meshes["CUBE"] = make_shared<CubeMesh>(device, commandList, 1.0f, 1.0f, 1.0f, XMFLOAT3{ 0.0f, 0.0f, 0.0f }, XMFLOAT4{ 0.8f, 0.0f, 0.0f, 1.0f });
}

void LoadingScene::LoadShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postRootSignature)
{
	s_shaders["DEFAULT"] = make_shared<StencilWriteShader>(device, rootSignature, Utile::PATH(TEXT("Shader/default.hlsl")), "VS", "PS");
	s_shaders["BLENDING"] = make_shared<BlendingShader>(device, rootSignature, Utile::PATH(TEXT("Shader/default.hlsl")), "VS", "PS");
	s_shaders["SKYBOX"] = make_shared<NoDepthShader>(device, rootSignature, Utile::PATH(TEXT("Shader/model.hlsl")), "VS", "PSSkybox");
	s_shaders["MODEL"] = make_shared<StencilWriteShader>(device, rootSignature, Utile::PATH(TEXT("Shader/model.hlsl")), "VS", "PS");
	s_shaders["ANIMATION"] = make_shared<StencilWriteShader>(device, rootSignature, Utile::PATH(TEXT("Shader/animation.hlsl")), "VS", "PS");
	s_shaders["LINK"] = make_shared<StencilWriteShader>(device, rootSignature, Utile::PATH(TEXT("Shader/link.hlsl")), "VS", "PS");
	s_shaders["UI"] = make_shared<BlendingShader>(device, rootSignature, Utile::PATH(TEXT("Shader/ui.hlsl")), "VS", "PS");
	s_shaders["UI_ATC"] = make_shared<BlendingShader>(device, rootSignature, Utile::PATH(TEXT("Shader/ui.hlsl")), "VS", "PS", true); // 알파 투 커버리지
	s_shaders["UI_SKILL_GAGE"] = make_shared<BlendingShader>(device, rootSignature, Utile::PATH(TEXT("Shader/ui.hlsl")), "VS", "PS_SKILL_GAGE");
	s_shaders["UI_WARNING"] = make_shared<BlendingShader>(device, rootSignature, Utile::PATH(TEXT("Shader/ui.hlsl")), "VS", "PS_WARNING");

	// 그림자 셰이더
	s_shaders["SHADOW_MODEL"] = make_shared<ShadowShader>(device, rootSignature, Utile::PATH(TEXT("Shader/shadow.hlsl")), "VS_MODEL", "GS");
	s_shaders["SHADOW_ANIMATION"] = make_shared<ShadowShader>(device, rootSignature, Utile::PATH(TEXT("Shader/shadow.hlsl")), "VS_ANIMATION", "GS");
	s_shaders["SHADOW_LINK"] = make_shared<ShadowShader>(device, rootSignature, Utile::PATH(TEXT("Shader/shadow.hlsl")), "VS_LINK", "GS");

	// 외곽선
	s_shaders["FULLSCREEN"] = make_shared<BlendingShader>(device, rootSignature, Utile::PATH(TEXT("Shader/outline.hlsl")), "VS", "PS");

	// 파티클
	s_shaders["DUST"] = make_shared<DustParticleShader>(device, rootSignature, Utile::PATH(TEXT("Shader/dust.hlsl")), "VS", "GS_STREAM", "GS", "PS");
	s_shaders["TRAIL"] = make_shared<TrailParticleShader>(device, rootSignature, Utile::PATH(TEXT("Shader/trail.hlsl")), "VS", "GS_STREAM", "GS", "PS");

	// 블러
	s_shaders["HORZ_BLUR"] = make_shared<ComputeShader>(device, postRootSignature, Utile::PATH(TEXT("Shader/blur.hlsl")), "HorzBlurCS");
	s_shaders["VERT_BLUR"] = make_shared<ComputeShader>(device, postRootSignature, Utile::PATH(TEXT("Shader/blur.hlsl")), "VertBlurCS");

	// 디버그
	s_shaders["WIREFRAME"] = make_shared<WireframeShader>(device, rootSignature, Utile::PATH(TEXT("Shader/default.hlsl")), "VS", "PS");
}

void LoadingScene::LoadTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	s_textures["SHADOW"] = make_shared<ShadowTexture>(device, 1 << 12, 1 << 12);

	s_textures["SKYBOX"] = make_shared<Texture>();
	s_textures["SKYBOX"]->Load(device, commandList, 5, Utile::PATH(TEXT("Object/skybox.dds")));

	s_textures["GAROO"] = make_shared<Texture>();
	s_textures["GAROO"]->Load(device, commandList, 5, Utile::PATH(TEXT("Mob/Garoo/texture.dds")));

	s_textures["SERPENT"] = make_shared<Texture>();
	s_textures["SERPENT"]->Load(device, commandList, 5, Utile::PATH(TEXT("Mob/Serpent/texture.dds")));

	s_textures["HORROR"] = make_shared<Texture>();
	s_textures["HORROR"]->Load(device, commandList, 5, Utile::PATH(TEXT("Mob/Horror/texture.dds")));

	s_textures["ULIFO"] = make_shared<Texture>();
	s_textures["ULIFO"]->Load(device, commandList, 5, Utile::PATH(TEXT("Mob/Ulifo/texture.dds")));

	s_textures["OBJECT0"] = make_shared<Texture>();
	s_textures["OBJECT0"]->Load(device, commandList, 5, Utile::PATH(TEXT("Object/texture0.dds")));

	s_textures["OBJECT1"] = make_shared<Texture>();
	s_textures["OBJECT1"]->Load(device, commandList, 5, Utile::PATH(TEXT("Object/texture1.dds")));

	s_textures["OBJECT2"] = make_shared<Texture>();
	s_textures["OBJECT2"]->Load(device, commandList, 5, Utile::PATH(TEXT("Object/texture2.dds")));

	s_textures["WHITE"] = make_shared<Texture>();
	s_textures["WHITE"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/WHITE.dds")));

	s_textures["TITLE"] = make_shared<Texture>();
	s_textures["TITLE"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/TITLE.dds")));

	s_textures["HIT"] = make_shared<Texture>();
	s_textures["HIT"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/UI_CROSSHAIR_HIT1.dds")));
	s_textures["HIT"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/UI_CROSSHAIR_HIT2.dds")));
	s_textures["HIT"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/UI_CROSSHAIR_HIT3.dds")));
	s_textures["HIT"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/UI_CROSSHAIR_HIT4.dds")));

	s_textures["ARROW"] = make_shared<Texture>();
	s_textures["ARROW"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/HIT_NOTICE_ARROW.dds")));

	// UI 외곽선
	s_textures["OUTLINE"] = make_shared<Texture>();
	s_textures["OUTLINE"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/UI_OUTLINE.dds")));

	// 보상 아이콘
	s_textures["REWARD_BULLET"] = make_shared<Texture>();
	s_textures["REWARD_BULLET"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/REWARD_BULLET.dds")));
	s_textures["REWARD_DAMAGE"] = make_shared<Texture>();
	s_textures["REWARD_DAMAGE"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/REWARD_DAMAGE.dds")));
	s_textures["REWARD_HP"] = make_shared<Texture>();
	s_textures["REWARD_HP"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/REWARD_HP.dds")));
	s_textures["REWARD_SPEED"] = make_shared<Texture>();
	s_textures["REWARD_SPEED"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/REWARD_SPEED.dds")));
	s_textures["REWARD_AR"] = make_shared<Texture>();
	s_textures["REWARD_AR"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/REWARD_AR.dds")));
	s_textures["REWARD_SG"] = make_shared<Texture>();
	s_textures["REWARD_SG"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/REWARD_SG.dds")));
	s_textures["REWARD_MG"] = make_shared<Texture>();
	s_textures["REWARD_MG"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/REWARD_MG.dds")));

	s_textures["WARNING"] = make_shared<Texture>();
	s_textures["WARNING"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/WARNING.dds")));

	s_textures["MANUAL"] = make_shared<Texture>();
	s_textures["MANUAL"]->Load(device, commandList, 5, Utile::PATH(TEXT("UI/MANUAL.dds")));

	// 총구 이펙트
	s_textures["MUZZLE_FRONT"] = make_shared<Texture>();
	s_textures["MUZZLE_FRONT"]->Load(device, commandList, 5, Utile::PATH(TEXT("Effect/muzzle_front_1.dds")));
	s_textures["MUZZLE_FRONT"]->Load(device, commandList, 5, Utile::PATH(TEXT("Effect/muzzle_front_2.dds")));
	s_textures["MUZZLE_FRONT"]->Load(device, commandList, 5, Utile::PATH(TEXT("Effect/muzzle_front_3.dds")));

	s_textures["TARGET"] = make_shared<Texture>();
	s_textures["TARGET"]->Load(device, commandList, 5, Utile::PATH(TEXT("Effect/AUTO_TARGET.dds")));
}

void LoadingScene::LoadTextBurshes(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext)
{
	DX::ThrowIfFailed(d2dDeivceContext->CreateSolidColorBrush(D2D1::ColorF{ 0x1e1e1e }, &TextObject::s_brushes["BLACK"]));
	DX::ThrowIfFailed(d2dDeivceContext->CreateSolidColorBrush(D2D1::ColorF{ D2D1::ColorF::Red }, &TextObject::s_brushes["RED"]));
	DX::ThrowIfFailed(d2dDeivceContext->CreateSolidColorBrush(D2D1::ColorF{ D2D1::ColorF::DeepSkyBlue }, &TextObject::s_brushes["BLUE"]));
	DX::ThrowIfFailed(d2dDeivceContext->CreateSolidColorBrush(D2D1::ColorF{ D2D1::ColorF::White }, &TextObject::s_brushes["WHITE"]));
}

void LoadingScene::LoadTextFormats(const ComPtr<IDWriteFactory>& dWriteFactory)
{
	for (int size : { 24, 28, 32, 36, 48 })
	{
		// 왼쪽 정렬
		DX::ThrowIfFailed(dWriteFactory->CreateTextFormat(
			TEXT("나눔바른고딕OTF"), NULL,
			DWRITE_FONT_WEIGHT_ULTRA_BOLD,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			static_cast<float>(size),
			TEXT("ko-kr"),
			&TextObject::s_formats[to_string(size) + "L"]
		));
		DX::ThrowIfFailed(TextObject::s_formats[to_string(size) + "L"]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING));
		DX::ThrowIfFailed(TextObject::s_formats[to_string(size) + "L"]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR));

		// 오른쪽 정렬
		DX::ThrowIfFailed(dWriteFactory->CreateTextFormat(
			TEXT("나눔바른고딕OTF"), NULL,
			DWRITE_FONT_WEIGHT_ULTRA_BOLD,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			static_cast<float>(size),
			TEXT("ko-kr"),
			&TextObject::s_formats[to_string(size) + "R"]
		));
		DX::ThrowIfFailed(TextObject::s_formats[to_string(size) + "R"]->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING));
		DX::ThrowIfFailed(TextObject::s_formats[to_string(size) + "R"]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR));
	}
}

void LoadingScene::LoadAudios()
{
	g_audioEngine.Load(Utile::PATH("Sound/BGM_INGAME.wav"), "INGAME", eAudioType::MUSIC);
	g_audioEngine.Load(Utile::PATH("Sound/BGM_LOBBY.wav"), "LOBBY", eAudioType::MUSIC);
	g_audioEngine.Load(Utile::PATH("Sound/GAME_HEARTBEAT.wav"), "HEARTBEAT", eAudioType::MUSIC);
	g_audioEngine.Load(Utile::PATH("Sound/GAME_DEATH.wav"), "DEATH", eAudioType::SOUND);
	g_audioEngine.Load(Utile::PATH("Sound/GAME_FOOTSTEP.wav"), "FOOTSTEP", eAudioType::SOUND);
	g_audioEngine.Load(Utile::PATH("Sound/GAME_HIT.wav"), "HIT", eAudioType::SOUND);
	g_audioEngine.Load(Utile::PATH("Sound/GAME_ROAR.wav"), "ROAR", eAudioType::SOUND);
	g_audioEngine.Load(Utile::PATH("Sound/GAME_SHOT.wav"), "SHOT0", eAudioType::SOUND);
	g_audioEngine.Load(Utile::PATH("Sound/GAME_SHOT.wav"), "SHOT1", eAudioType::SOUND);
	g_audioEngine.Load(Utile::PATH("Sound/UI_CLICK.wav"), "CLICK", eAudioType::SOUND);
	g_audioEngine.Load(Utile::PATH("Sound/UI_HOVER.wav"), "HOVER", eAudioType::SOUND);
}