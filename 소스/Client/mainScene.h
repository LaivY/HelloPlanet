#pragma once
#include "camera.h"
#include "object.h"
#include "scene.h"
#include "shadow.h"
#include "textObject.h"
#include "uiObject.h"
#include "windowObject.h"

class MainScene : public Scene
{
public:
	MainScene();
	~MainScene();

	virtual void OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
						const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature,
						const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory);
	virtual void OnResize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime);
	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnUpdate(FLOAT deltaTime);

	// 게임루프 함수
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void PreRender(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;
	virtual void Render2D(const ComPtr<ID2D1DeviceContext2>& device);

	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateTextObjects(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory);
	void CreateLights() const;

	void CreateSettingWindow();
	void CloseWindow();

	void Update(FLOAT deltaTime);
	void UpdateCameraPosition(FLOAT deltaTime);
	void UpdateShadowMatrix();

	void RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

private:
	ComPtr<ID3D12Resource>			m_cbGameScene;
	cbGameScene*					m_pcbGameScene;
	unique_ptr<cbGameScene>			m_cbGameSceneData;

	unique_ptr<ShadowMap>			m_shadowMap;
	unique_ptr<Skybox>				m_skybox;
	shared_ptr<Camera>				m_camera;
	shared_ptr<Camera>				m_uiCamera;
	vector<unique_ptr<Player>>		m_players;
	vector<unique_ptr<GameObject>>	m_gameObjects;
	vector<unique_ptr<UIObject>>	m_uiObjects;
	vector<unique_ptr<TextObject>>	m_textObjects;
	vector<unique_ptr<WindowObject>> m_windowObjects;
};