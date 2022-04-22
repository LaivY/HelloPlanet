#pragma once
#include "stdafx.h"
#include "mesh.h"
#include "shader.h"
#include "texture.h"

class Scene
{
public:
	Scene();
	virtual ~Scene() = default;

	// 이벤트 함수
	virtual void OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
						const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature,
						const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory) = NULL;
	virtual void OnInitEnd() = NULL;
	virtual void OnResize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) = NULL;
	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime) = NULL;
	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) = NULL;
	virtual void OnKeyboardEvent(FLOAT deltaTime) = NULL;
	virtual void OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) = NULL;
	virtual void OnUpdate(FLOAT deltaTime) = NULL;

	// 게임루프 함수
	virtual void Update(FLOAT deltaTime) = NULL;
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const = NULL;
	virtual void PreRender(const ComPtr<ID3D12GraphicsCommandList>& commandList) const = NULL;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const = NULL;
	virtual void Render2D(const ComPtr<ID2D1DeviceContext2>& device) = NULL;

	// 서버 통신 함수
	virtual void ProcessClient() = NULL;

protected:
	static unordered_map<string, shared_ptr<Mesh>>		s_meshes;
	static unordered_map<string, shared_ptr<Shader>>	s_shaders;
	static unordered_map<string, shared_ptr<Texture>>	s_textures;

	D3D12_VIEWPORT	m_viewport;
	D3D12_RECT		m_scissorRect;
};