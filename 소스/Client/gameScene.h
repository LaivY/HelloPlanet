#pragma once
#include "stdafx.h"
#include "camera.h"
#include "mesh.h"
#include "object.h"
#include "player.h"
#include "scene.h"
#include "shadow.h"
#include "texture.h"
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

struct cbGameScene
{
	ShadowLight shadowLight;
	Light		ligths[Setting::MAX_LIGHTS];
};

class GameScene : public Scene
{
public:
	GameScene();
	~GameScene();

	// 이벤트 함수
	virtual void OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
						const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature,
						const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory);
	virtual void OnResize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime);
	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnKeyboardEvent(FLOAT deltaTime);
	virtual void OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnUpdate(FLOAT deltaTime);

	// 게임루프 함수
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void PreRender(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;
	virtual void Render2D(const ComPtr<ID2D1DeviceContext2>& device);

	// 서버 통신 함수
	virtual void ProcessClient();

	void Update(FLOAT deltaTime);
	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateTextObjects(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory);
	void CreateLights() const;
	void LoadMapObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& mapFile);

	void RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	void PlayerCollisionCheck(FLOAT deltaTime);
	void UpdateShadowMatrix();

	void RecvPacket();
	void RecvLoginOk();
	void RecvUpdateClient();
	void RecvUpdateMonster();
	void RecvBulletFire();

private:
	ComPtr<ID3D12Resource>								m_cbGameScene;		// 상수 버퍼
	cbGameScene*										m_pcbGameScene;		// 상수 버퍼 포인터
	unique_ptr<cbGameScene>								m_cbGameSceneData;	// 상수 버퍼 데이터

	unique_ptr<ShadowMap>								m_shadowMap;		// 그림자맵
	unique_ptr<Skybox>									m_skybox;			// 스카이박스
	shared_ptr<Camera>									m_camera;			// 카메라
	shared_ptr<Player>									m_player;			// 플레이어
	array<unique_ptr<Player>, Setting::MAX_PLAYERS>		m_multiPlayers;		// 멀티플레이어
	unordered_map<INT, unique_ptr<Monster>>				m_monsters;			// 몬스터들
	vector<unique_ptr<GameObject>>						m_gameObjects;		// 게임오브젝트들

	unique_ptr<Camera>									m_uiCamera;			// UI 카메라
	vector<unique_ptr<UIObject>>						m_uiObjects;		// UI 오브젝트
	vector<unique_ptr<TextObject>>						m_textObjects;		// 텍스트 오브젝트
};