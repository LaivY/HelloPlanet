#include "stdafx.h"
#include "scene.h"
#include "framework.h"

unordered_map<string, shared_ptr<Mesh>>		Scene::s_meshes;
unordered_map<string, shared_ptr<Shader>>	Scene::s_shaders;
unordered_map<string, shared_ptr<Texture>>	Scene::s_textures;

Scene::Scene()
{
	m_viewport = D3D12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(g_width), static_cast<float>(g_height), 0.0f, 1.0f };
	m_scissorRect = D3D12_RECT{ 0, 0, static_cast<long>(g_width), static_cast<long>(g_height) };
}

void Scene::OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature, const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory) { }
void Scene::OnInitEnd() { }
void Scene::OnDestroy() { }
void Scene::OnResize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { }
void Scene::OnMouseEvent(HWND hWnd, FLOAT deltaTime) { }
void Scene::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { }
void Scene::OnKeyboardEvent(FLOAT deltaTime) { }
void Scene::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { }
void Scene::OnUpdate(FLOAT deltaTime) { }
void Scene::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const { }
void Scene::PreRender(const ComPtr<ID3D12GraphicsCommandList>& commandList) const { }
void Scene::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const { }
void Scene::Render2D(const ComPtr<ID2D1DeviceContext2>& device) const { }
void Scene::PostProcessing(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& postRootSignature, const ComPtr<ID3D12Resource>& renderTarget) const { }
void Scene::ProcessClient() { }

Player* Scene::GetPlayer() const { return nullptr; }
Camera* Scene::GetCamera() const { return nullptr; }