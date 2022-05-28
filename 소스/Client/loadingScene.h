#pragma once
#include "scene.h"

class Camera;
class UIObject;

class LoadingScene : public Scene
{
public:
	LoadingScene();
	~LoadingScene();

	// 이벤트 함수
	virtual void OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
						const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature,
						const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory);
	virtual void OnUpdate(FLOAT deltaTime);

	// 게임루프 함수
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;

	void Update(FLOAT deltaTime);
	void CreateMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature);
	void CreateTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);

	void LoadResources(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
					   const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature,
					   const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory);
	void LoadMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void LoadShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature);
	void LoadTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void LoadTextBurshes(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext);
	void LoadTextFormats(const ComPtr<IDWriteFactory>& dWriteFactory);
	void LoadAudios();

private:
	thread								m_thread;
	ComPtr<ID3D12GraphicsCommandList>	m_commandList;
	ComPtr<ID3D12CommandAllocator>		m_commandAllocator;
	BOOL								m_isDone;

	shared_ptr<Camera>					m_camera;
	vector<unique_ptr<UIObject>>		m_uiObjects;
	unique_ptr<UIObject>				m_loadingBarObject;
};