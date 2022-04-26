﻿#pragma once
#include "stdafx.h"
#include "object.h"
#include "camera.h"
#include "timer.h"
#include "scene.h"

enum class eScene
{
	NONE, LOADING, MAIN, GAME
};

struct cbGameFramework
{
	FLOAT deltaTime;
};

class GameFramework
{
public:
	GameFramework();
	~GameFramework();

	void GameLoop();
	void OnInit(HINSTANCE hInstance, HWND hWnd);
	void OnResize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnMove(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnUpdate(FLOAT deltaTime);
	void OnRender();
	void OnDestroy();
	void OnMouseEvent();
	void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void OnKeyboardEvent();
	void OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void LoadPipeline();
	void LoadAssets();
	void CreateFactory();
	void CreateDevice();
	void CreateCommandQueue();
	void CreateD3D11On12Device();
	void CreateD2DDevice();
	void CreateSwapChain();
	void CreateRtvDsvDescriptorHeap();
	void CreateRenderTargetView();
	void CreateDepthStencilView();
	void CreateRootSignature();
	void CreateShaderVariable();

	void Update(FLOAT deltaTime);
	void UpdateShaderVariable() const;
	void PopulateCommandList() const;
	void Render2D() const;
	void WaitForPreviousFrame();
	void WaitForGpu();
	BOOL ConnectServer();
	void ProcessClient();
	void ChangeScene();

	void SetIsActive(BOOL isActive);
	void SetIsFullScreen(BOOL isFullScreen);
	void SetNextScene(eScene sceneType);

	HWND GetWindow() const;
	BOOL isActive() const;
	BOOL isFullScreen() const;
	RECT GetLastWindowRect() const;
	RECT GetFullScreenRect() const;
	ComPtr<IDWriteFactory> GetDWriteFactory() const;
	ComPtr<ID3D12CommandQueue> GetCommandQueue() const;

private:
	static constexpr UINT				FrameCount = 3;

	// Window
	HINSTANCE							m_hInstance;
	HWND								m_hWnd;
	BOOL								m_isActive;
	BOOL								m_isFullScreen;
	RECT								m_lastWindowRect;
	RECT								m_fullScreenRect;

	// Pipeline
	ComPtr<IDXGIFactory4>				m_factory;
	ComPtr<IDXGISwapChain3>				m_swapChain;
	INT									m_MSAA4xQualityLevel;
	ComPtr<ID3D12Device>				m_device;
	ComPtr<ID3D12CommandAllocator>		m_commandAllocators[FrameCount];
	ComPtr<ID3D12CommandQueue>			m_commandQueue;
	ComPtr<ID3D12GraphicsCommandList>	m_commandList;
	ComPtr<ID3D12Resource>				m_renderTargets[FrameCount];
	ComPtr<ID3D12DescriptorHeap>		m_rtvHeap;
	UINT								m_rtvDescriptorSize;
	ComPtr<ID3D12Resource>				m_depthStencil;
	ComPtr<ID3D12DescriptorHeap>		m_dsvHeap;
	ComPtr<ID3D12RootSignature>			m_rootSignature;
	ComPtr<ID3D12RootSignature>			m_postProcessRootSignature;

	// Direct11, 2D
	ComPtr<ID3D11DeviceContext>			m_d3d11DeviceContext;
	ComPtr<ID3D11On12Device>			m_d3d11On12Device;
	ComPtr<IDWriteFactory>				m_dWriteFactory;
	ComPtr<ID2D1Factory3>				m_d2dFactory;
	ComPtr<ID2D1Device2>				m_d2dDevice;
	ComPtr<ID2D1DeviceContext2>			m_d2dDeviceContext;
	ComPtr<ID3D11Resource>				m_wrappedBackBuffers[FrameCount];
	ComPtr<ID2D1Bitmap1>				m_d2dRenderTargets[FrameCount];

	// Synchronization
	ComPtr<ID3D12Fence>					m_fence;
	UINT								m_frameIndex;
	HANDLE								m_fenceEvent;
	UINT64								m_fenceValues[FrameCount];

	// 상수 버퍼
	ComPtr<ID3D12Resource>				m_cbGameFramework;
	cbGameFramework*					m_pcbGameFramework;
	unique_ptr<cbGameFramework>			m_cbGameFrameworkData;

	// Timer
	Timer								m_timer;

	// Scene
	unique_ptr<Scene>					m_scene;
	eScene								m_nextScene;
};