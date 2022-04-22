#pragma once
#include "camera.h"
#include "mesh.h"
#include "scene.h"
#include "shader.h"
#include "texture.h"
#include "uiObject.h"

class LoadingScene : public Scene
{
public:
	LoadingScene();
	~LoadingScene();

	// 이벤트 함수
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

	// 게임루프 함수
	virtual void Update(FLOAT deltaTime);
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void PreRender(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;
	virtual void Render2D(const ComPtr<ID2D1DeviceContext2>& device);

	// 서버 통신 함수
	virtual void ProcessClient();

	void CreateMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature);
	void CreateTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);

	void LoadResources(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature);
	void LoadMeshes(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void LoadShaders(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature);
	void LoadTextures(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);

private:
	thread								m_thread;
	ComPtr<ID3D12GraphicsCommandList>	m_commandList;
	ComPtr<ID3D12CommandAllocator>		m_commandAllocator;
	BOOL								m_isDone;

	shared_ptr<Camera>					m_camera;
	vector<unique_ptr<UIObject>>		m_uiObjects;
};