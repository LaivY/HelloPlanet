﻿#pragma once
#include "stdafx.h"
#include "camera.h"
#include "object.h"
#include "player.h"
#include "shadow.h"
#include "skybox.h"

struct Light
{
	XMFLOAT4X4	lightViewMatrix;
	XMFLOAT4X4	lightProjMatrix;
	XMFLOAT3	position;
	FLOAT		padding1;
	XMFLOAT3	direction;
	FLOAT		padding2;
};

struct cbScene
{
	Light		ligths[MAX_LIGHT];
};

class Scene
{
public:
	Scene();
	~Scene();

	// 이벤트 함수
	void OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, 
				const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature);
	void OnMouseEvent(HWND hWnd, UINT width, UINT height, FLOAT deltaTime);
	void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnKeyboardEvent(FLOAT deltaTime);
	void OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnUpdate(FLOAT deltaTime);

	// 초기화 함수
	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature);
	void CreateTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateLights();
	void CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);

	// 초기화 후 호출되는 함수
	void ReleaseUploadBuffer();

	// 게임루프 함수
	void Update(FLOAT deltaTime);
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;
	
	// 위의 함수를 구현하기 위한 함수
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	void UpdateLights(FLOAT deltaTime);
	void RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

	// 세터
	void SetSkybox(unique_ptr<Skybox>& skybox);
	void SetPlayer(const shared_ptr<Player>& player);
	void SetCamera(const shared_ptr<Camera>& camera);

	// 게터
	Skybox* GetSkybox() const { return m_skybox.get(); }
	shared_ptr<Player> GetPlayer() const { return m_player; }
	shared_ptr<Camera> GetCamera() const { return m_camera; }
	
private:
	D3D12_VIEWPORT								m_viewport;		// 뷰포트
	D3D12_RECT									m_scissorRect;	// 가위사각형

	ComPtr<ID3D12Resource>						m_cbScene;		// 씬 상수 버퍼
	cbScene*									m_pcbScene;		// 씬 상수 버퍼 포인터
	unique_ptr<cbScene>							m_cbSceneData;	// 씬 상수 버퍼 데이터

	unordered_map<string, shared_ptr<Mesh>>		m_meshes;		// 메쉬
	unordered_map<string, shared_ptr<Shader>>	m_shaders;		// 셰이더
	unordered_map<string, shared_ptr<Texture>>	m_textures;		// 텍스쳐
	unique_ptr<ShadowMap>						m_shadowMap;	// 그림자맵
	unique_ptr<Skybox>							m_skybox;		// 스카이박스
	shared_ptr<Camera>							m_camera;		// 카메라
	shared_ptr<Player>							m_player;		// 플레이어
	vector<unique_ptr<GameObject>>				m_gameObjects;	// 게임오브젝트들
};