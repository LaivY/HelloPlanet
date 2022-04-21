#pragma once
#include "stdafx.h"
#include "camera.h"
#include "object.h"
#include "player.h"
#include "shadow.h"
#include "textObject.h"
#include "uiObject.h"

struct Light
{
	XMFLOAT4X4	lightViewMatrix;
	XMFLOAT4X4	lightProjMatrix;
	XMFLOAT3	color;
	FLOAT		padding1;
	XMFLOAT3	direction;
	FLOAT		padding2;
};

struct ShadowLight
{
	XMFLOAT4X4	lightViewMatrix[Setting::SHADOWMAP_COUNT];
	XMFLOAT4X4	lightProjMatrix[Setting::SHADOWMAP_COUNT];
	XMFLOAT3	color;
	FLOAT		padding1;
	XMFLOAT3	direction;
	FLOAT		padding2;
};

struct cbScene
{
	ShadowLight shadowLight;
	Light		ligths[Setting::MAX_LIGHTS];
};

class Scene
{
public:
	Scene();
	~Scene();

	// 이벤트 함수
	void OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, 
				const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature,
				const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory);
	void OnMouseEvent(HWND hWnd, FLOAT deltaTime);
	void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnKeyboardEvent(FLOAT deltaTime);
	void OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnUpdate(FLOAT deltaTime);

	// 초기화 함수
	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature);
	void CreateTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateTextObjects(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory);
	void CreateLights() const;
	void LoadMapObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& mapFile);

	// 초기화 후 호출되는 함수
	void ReleaseUploadBuffer();

	// 게임루프 함수
	void Update(FLOAT deltaTime);
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;
	void Render2D(const ComPtr<ID2D1DeviceContext2>& device);
	
	// 업데이트 함수
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	void RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	void PlayerCollisionCheck(FLOAT deltaTime);
	void UpdateShadowMatrix();

	// 서버 통신 함수
	/*virtual*/ void ProcessClient(LPVOID arg);
	void RecvPacket();
	void RecvLoginOk();
	void RecvUpdateClient();
	void RecvUpdateMonster();
	void RecvBulletFire();

	// 세터
	void SetViewport(const D3D12_VIEWPORT& viewport);
	void SetScissorRect(const D3D12_RECT& scissorRect);
	
private:
	D3D12_VIEWPORT										m_viewport;		// 뷰포트
	D3D12_RECT											m_scissorRect;	// 가위사각형

	ComPtr<ID3D12Resource>								m_cbScene;		// 씬 상수 버퍼
	cbScene*											m_pcbScene;		// 씬 상수 버퍼 포인터
	unique_ptr<cbScene>									m_cbSceneData;	// 씬 상수 버퍼 데이터

	unordered_map<string, shared_ptr<Mesh>>				m_meshes;		// 메쉬
	unordered_map<string, shared_ptr<Shader>>			m_shaders;		// 셰이더
	unordered_map<string, shared_ptr<Texture>>			m_textures;		// 텍스쳐
	unique_ptr<ShadowMap>								m_shadowMap;	// 그림자맵

	unique_ptr<Skybox>									m_skybox;		// 스카이박스
	shared_ptr<Camera>									m_camera;		// 카메라
	shared_ptr<Player>									m_player;		// 플레이어
	array<unique_ptr<Player>, Setting::MAX_PLAYERS>		m_multiPlayers;	// 멀티플레이어
	unordered_map<INT, unique_ptr<Monster>>				m_monsters;		// 몬스터들
	vector<unique_ptr<GameObject>>						m_gameObjects;	// 게임오브젝트들

	unique_ptr<Camera>									m_uiCamera;		// UI 카메라
	vector<unique_ptr<UIObject>>						m_uiObjects;	// UI 오브젝트

	vector<unique_ptr<TextObject>>						m_textObjects;	// 텍스트 오브젝트
};