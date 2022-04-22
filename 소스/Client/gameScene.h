#pragma once
#include "stdafx.h"
#include "camera.h"
#include "object.h"
#include "player.h"
#include "scene.h"
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

	// �̺�Ʈ �Լ�
	virtual void OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
						const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature,
						const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory);
	virtual void OnInitEnd();
	virtual void OnResize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime);
	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnKeyboardEvent(FLOAT deltaTime);
	virtual void OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnUpdate(FLOAT deltaTime);

	// ���ӷ��� �Լ�
	virtual void Update(FLOAT deltaTime);
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void PreRender(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;
	virtual void Render2D(const ComPtr<ID2D1DeviceContext2>& device);

	// ���� ��� �Լ�
	virtual void ProcessClient();

	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature);
	void CreateTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
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
	ComPtr<ID3D12Resource>								m_cbGameScene;		// ��� ����
	cbGameScene*										m_pcbGameScene;		// ��� ���� ������
	unique_ptr<cbGameScene>								m_cbGameSceneData;	// ��� ���� ������

	unordered_map<string, shared_ptr<Mesh>>				m_meshes;			// �޽�
	unordered_map<string, shared_ptr<Shader>>			m_shaders;			// ���̴�
	unordered_map<string, shared_ptr<Texture>>			m_textures;			// �ؽ���
	unique_ptr<ShadowMap>								m_shadowMap;		// �׸��ڸ�

	unique_ptr<Skybox>									m_skybox;			// ��ī�̹ڽ�
	shared_ptr<Camera>									m_camera;			// ī�޶�
	shared_ptr<Player>									m_player;			// �÷��̾�
	array<unique_ptr<Player>, Setting::MAX_PLAYERS>		m_multiPlayers;		// ��Ƽ�÷��̾�
	unordered_map<INT, unique_ptr<Monster>>				m_monsters;			// ���͵�
	vector<unique_ptr<GameObject>>						m_gameObjects;		// ���ӿ�����Ʈ��

	unique_ptr<Camera>									m_uiCamera;			// UI ī�޶�
	vector<unique_ptr<UIObject>>						m_uiObjects;		// UI ������Ʈ

	vector<unique_ptr<TextObject>>						m_textObjects;		// �ؽ�Ʈ ������Ʈ
};