#include "stdafx.h"
#include "framework.h"
#include "audioEngine.h"
#include "camera.h"
#include "filter.h"
#include "object.h"
#include "player.h"
#include "scene.h"
#include "gameScene.h"
#include "loadingScene.h"
#include "lobbyScene.h"
#include "mainScene.h"
#include "timer.h"
#include "texture.h"

GameFramework::GameFramework() 
	: m_hInstance{}, m_hWnd{}, m_isActive{ TRUE }, m_isFullScreen{ FALSE }, m_lastWindowRect{}, m_MSAA4xQualityLevel{},
	  m_frameIndex{ 0 }, m_fenceValues{}, m_fenceEvent{}, m_rtvDescriptorSize{ 0 }, m_pcbGameFramework{ nullptr }, m_nextScene{ eSceneType::NONE }, m_nextTempScene{ eSceneType::NONE }
{
	m_timer = make_unique<Timer>();

	HWND desktop{ GetDesktopWindow() };
	GetWindowRect(desktop, &m_fullScreenRect);
	g_maxWidth = m_fullScreenRect.right;
	g_maxHeight = m_fullScreenRect.bottom;
}

GameFramework::~GameFramework()
{

}

void GameFramework::GameLoop()
{
	m_timer->Tick();
	OnUpdate(m_timer->GetDeltaTime());
	OnRender();
}

void GameFramework::OnInit(HINSTANCE hInstance, HWND hWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hWnd;
	GetWindowRect(hWnd, &m_lastWindowRect);

	LoadPipeline();
	LoadAssets();
}

void GameFramework::OnResize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// GPU명령이 끝날때까지 대기
	WaitForGpu();

	// 스왑체인과 관련된 리소스 해제
	for (int i = 0; i < FrameCount; ++i)
	{
		m_renderTargets[i].Reset();
		m_wrappedBackBuffers[i].Reset();
		m_d2dRenderTargets[i].Reset();
		m_commandAllocators[i].Reset();
		m_fenceValues[i] = m_fenceValues[m_frameIndex];
	}
	m_d2dDeviceContext->SetTarget(nullptr);
	m_d2dDeviceContext->Flush();

	m_d3d11DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	m_d3d11DeviceContext->Flush();

	// 가로, 세로, 화면비 계산
	RECT clientRect{};
	GetWindowRect(hWnd, &clientRect);
	g_width = static_cast<UINT>(clientRect.right - clientRect.left);
	g_height = static_cast<UINT>(clientRect.bottom - clientRect.top);

	DXGI_SWAP_CHAIN_DESC desc{};
	m_swapChain->GetDesc(&desc);
	DX::ThrowIfFailed(m_swapChain->ResizeBuffers(FrameCount, g_width, g_height, desc.BufferDesc.Format, desc.Flags));

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	CreateRenderTargetView();
	CreateDepthStencilView();

	// 페이드 인/아웃 필터 생성
	// 윈도우 크기가 바뀔 때마다 그 크기에 맞게 텍스쳐를 다시 만들어야함
	m_fadeFilter = make_unique<FadeFilter>(m_device);

	if (m_scene) m_scene->OnResize(hWnd, message, wParam, lParam);
}

void GameFramework::OnMove(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (!m_isFullScreen)
		GetWindowRect(hWnd, &m_lastWindowRect);
}

void GameFramework::OnUpdate(FLOAT deltaTime)
{
	Update(deltaTime);
	if (m_scene)
	{
		m_scene->OnUpdate(deltaTime);
		m_scene->OnMouseEvent(m_hWnd, deltaTime);
		m_scene->OnKeyboardEvent(deltaTime);
	}
	g_audioEngine.Update(deltaTime);
}

void GameFramework::OnRender()
{
	// 3D 렌더링
	PopulateCommandList();
	ID3D12CommandList* ppCommandList[]{ m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

	// 2D 렌더링
	Render2D();

	// 출력
	DX::ThrowIfFailed(m_swapChain->Present(1, 0));
	WaitForPreviousFrame();
}

void GameFramework::OnDestroy()
{
	if (m_scene) m_scene->OnDestroy();
	WaitForPreviousFrame();
	CloseHandle(m_fenceEvent);
}

void GameFramework::OnMouseEvent()
{
	if (m_scene) m_scene->OnMouseEvent(m_hWnd, m_timer->GetDeltaTime());
}

void GameFramework::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (m_scene) m_scene->OnMouseEvent(hWnd, message, wParam, lParam);
}

void GameFramework::OnKeyboardEvent()
{
	if (m_scene) m_scene->OnKeyboardEvent(m_timer->GetDeltaTime());
}

void GameFramework::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (m_scene) m_scene->OnKeyboardEvent(hWnd, message, wParam, lParam);
}

void GameFramework::LoadPipeline()
{
	// 팩토리 생성
	CreateFactory();

	// 디바이스 생성(팩토리 필요)
	CreateDevice();

	// 명령큐 생성(디바이스 필요)
	CreateCommandQueue();

	// 11on12 디바이스 생성(명령큐 필요)
	CreateD3D11On12Device();

	// D2D, DWrite 생성(11on12 디바이스 필요)
	CreateD2DDevice();

	// 스왑체인 생성(팩토리, 명령큐 필요)
	CreateSwapChain();

	// 렌더타겟뷰, 깊이스텐실뷰의 서술자힙 생성(디바이스 필요)
	CreateRtvDsvDescriptorHeap();

	// 렌더타겟뷰 생성(디바이스 필요)
	CreateRenderTargetView();

	// 깊이스텐실뷰 생성(디바이스 필요)
	CreateDepthStencilView();

	// 루트시그니쳐 생성(디바이스 필요)
	CreateRootSignature();
	CreatePostRootSignature();

	// 명령리스트 생성
	DX::ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
	DX::ThrowIfFailed(m_commandList->Close());

	// 펜스 생성
	DX::ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	++m_fenceValues[m_frameIndex];

	// alt + enter 금지
	m_factory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
}

void GameFramework::LoadAssets()
{
	// 명령을 추가할 것이기 때문에 Reset
	m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), NULL);

	// 셰이더 변수 생성
	CreateShaderVariable();

	// 씬 생성, 초기화
	m_scene = make_unique<LoadingScene>();
	m_scene->OnInit(m_device, m_commandList, m_rootSignature, m_postRootSignature, m_d2dDeviceContext, m_dWriteFactory);

	// 명령 제출
	m_commandList->Close();
	ID3D12CommandList* ppCommandList[]{ m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

	// 명령들이 완료될 때까지 대기
	WaitForGpu();

	// 디폴트 버퍼로의 복사가 완료됐으므로 업로드 버퍼를 해제한다.
	m_scene->OnInitEnd();

	// 타이머 초기화
	m_timer->Tick();
}

void GameFramework::CreateFactory()
{
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif
	DX::ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_factory)));
}

void GameFramework::CreateDevice()
{
	ComPtr<IDXGIAdapter1> adapter;
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_factory->EnumAdapters1(i, &adapter); ++i)
	{
		DXGI_ADAPTER_DESC1 adapterDesc;
		adapter->GetDesc1(&adapterDesc);
		if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)))) break;
	}
	if (!m_device)
	{
		m_factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
		DX::ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));
	}

	// 서술자힙 크기
	g_cbvSrvDescriptorIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_dsvDescriptorIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	g_device = m_device;
}

void GameFramework::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	DX::ThrowIfFailed(m_device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_commandQueue)));
}

void GameFramework::CreateSwapChain()
{
	// 샘플링 수준 체크
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS multiSampleQualityLevels;
	multiSampleQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	multiSampleQualityLevels.SampleCount = 4;
	multiSampleQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	multiSampleQualityLevels.NumQualityLevels = 0;
	m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &multiSampleQualityLevels, sizeof(multiSampleQualityLevels));
	m_MSAA4xQualityLevel = multiSampleQualityLevels.NumQualityLevels;

	// 스왑체인 생성
	DXGI_SWAP_CHAIN_DESC swapChainDesc{};
	swapChainDesc.BufferDesc.Width = g_width;
	swapChainDesc.BufferDesc.Height = g_height;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.OutputWindow = m_hWnd;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = m_MSAA4xQualityLevel > 1 ? 4 : 1;
	swapChainDesc.SampleDesc.Quality = m_MSAA4xQualityLevel > 1 ? m_MSAA4xQualityLevel - 1 : 0;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // 전체화면으로 전환할 때 적합한 디스플레이 모드를 선택

	ComPtr<IDXGISwapChain> swapChain;
	DX::ThrowIfFailed(m_factory->CreateSwapChain(m_commandQueue.Get(), &swapChainDesc, &swapChain));
	DX::ThrowIfFailed(swapChain.As(&m_swapChain));
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void GameFramework::CreateRtvDsvDescriptorHeap()
{
	// 렌더타겟뷰 서술자힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.NumDescriptors = FrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = NULL;
	DX::ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 깊이스텐실 서술자힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = NULL;
	DX::ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
}

void GameFramework::CreateRenderTargetView()
{
	// Query the desktop's dpi settings, which will be used to create D2D's render targets.
	float dpiX;
	float dpiY;
#pragma warning(push)
#pragma warning(disable : 4996) // GetDesktopDpi is deprecated.
	m_d2dFactory->GetDesktopDpi(&dpiX, &dpiY);
#pragma warning(pop)

	D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
		dpiX,
		dpiY
	);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{ m_rtvHeap->GetCPUDescriptorHandleForHeapStart() };
	for (UINT i = 0; i < FrameCount; ++i)
	{
		DX::ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
		m_device->CreateRenderTargetView(m_renderTargets[i].Get(), NULL, rtvHandle);

		// Create a wrapped 11On12 resource of this back buffer. Since we are 
		// rendering all D3D12 content first and then all D2D content, we specify 
		// the In resource state as RENDER_TARGET - because D3D12 will have last 
		// used it in this state - and the Out resource state as PRESENT. When 
		// ReleaseWrappedResources() is called on the 11On12 device, the resource 
		// will be transitioned to the PRESENT state.
		D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
		DX::ThrowIfFailed(m_d3d11On12Device->CreateWrappedResource(
			m_renderTargets[i].Get(),
			&d3d11Flags,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT,
			IID_PPV_ARGS(&m_wrappedBackBuffers[i])
		));

		// Create a render target for D2D to draw directly to this back buffer.
		ComPtr<IDXGISurface> surface;
		DX::ThrowIfFailed(m_wrappedBackBuffers[i].As(&surface));
		DX::ThrowIfFailed(m_d2dDeviceContext->CreateBitmapFromDxgiSurface(
			surface.Get(),
			&bitmapProperties,
			&m_d2dRenderTargets[i]
		));

		rtvHandle.Offset(m_rtvDescriptorSize);

		// 명령할당자 생성
		DX::ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i])));
	}
}

void GameFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = g_width;
	resourceDesc.Height = g_height;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = m_MSAA4xQualityLevel > 1 ? 4 : 1;
	resourceDesc.SampleDesc.Quality = m_MSAA4xQualityLevel > 1 ? m_MSAA4xQualityLevel - 1 : 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	DX::ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_PPV_ARGS(&m_depthStencil)
	));

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;
	m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilViewDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void GameFramework::CreateRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE ranges[4]{};
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
	ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

	// 자주 갱신하는 순서대로 해야 성능에 좋음
	CD3DX12_ROOT_PARAMETER rootParameter[9]{};
	rootParameter[0].InitAsConstants(16, 0, 0);												// cbGameObject : b0
	rootParameter[1].InitAsConstantBufferView(1);											// cbMesh : b1
	rootParameter[2].InitAsConstantBufferView(2);											// cbCamera : b2
	rootParameter[3].InitAsConstantBufferView(3);											// cbScene : b3
	rootParameter[4].InitAsConstantBufferView(4);											// cbGameFramework : b4
	rootParameter[5].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);	// Texture2D g_texture : t0
	rootParameter[6].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);	// Texture2DArray g_shadowMap : t1
	rootParameter[7].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);	// Texture2D<float> g_detph : t2
	rootParameter[8].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);	// Texture2D<uint2> g_stencil : t3

	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2]{};
	samplerDesc[0].Init(
		0,								 				// ShaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, 				// filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, 				// addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, 				// addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, 				// addressW
		0.0f,											// mipLODBias
		1,												// maxAnisotropy
		D3D12_COMPARISON_FUNC_ALWAYS,					// comparisonFunc
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,	// borderColor
		0.0f,											// minLOD
		D3D12_FLOAT32_MAX,								// maxLOD
		D3D12_SHADER_VISIBILITY_PIXEL,					// shaderVisibility
		0												// registerSpace
	);
	samplerDesc[1].Init(
		1,								 					// ShaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,	// filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER, 					// addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER, 					// addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER, 					// addressW
		0.0f,												// mipLODBias
		16,													// maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,					// comparisonFunc
		D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK			// borderColor
	);
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameter), rootParameter, _countof(samplerDesc), samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT);

	ComPtr<ID3DBlob> signature, error;
	DX::ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	DX::ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
}

void GameFramework::CreatePostRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE ranges[2]{};
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParameter[4]{};
	rootParameter[0].InitAsConstantBufferView(0);			// cbGameFramework : b0
	rootParameter[1].InitAsDescriptorTable(1, &ranges[0]);	// Texture2D g_input : t0;
	rootParameter[2].InitAsDescriptorTable(1, &ranges[1]);	// RWTexture2D<float4> g_output : u0;
	rootParameter[3].InitAsConstantBufferView(1);			// cbBlurFilter : b1

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameter), rootParameter, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature, error;
	DX::ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	DX::ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_postRootSignature)));
}

void GameFramework::CreateD3D11On12Device()
{
	// Create an 11 device wrapped around the 12 device and share 12's command queue.
	ComPtr<ID3D11Device> d3d11Device;
	DX::ThrowIfFailed(D3D11On12CreateDevice(
		m_device.Get(),
		D3D11_CREATE_DEVICE_BGRA_SUPPORT,
		nullptr,
		0,
		reinterpret_cast<IUnknown**>(m_commandQueue.GetAddressOf()),
		1,
		0,
		&d3d11Device,
		&m_d3d11DeviceContext,
		nullptr
	));

	// Query the 11On12 device from the 11 device.
	DX::ThrowIfFailed(d3d11Device.As(&m_d3d11On12Device));
}

void GameFramework::CreateD2DDevice()
{
	// Create D2D/DWrite components.
	D2D1_FACTORY_OPTIONS d2dFactoryOptions{};
	D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
	DX::ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, &m_d2dFactory));
	ComPtr<IDXGIDevice> dxgiDevice;
	DX::ThrowIfFailed(m_d3d11On12Device.As(&dxgiDevice));
	DX::ThrowIfFailed(m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice));
	DX::ThrowIfFailed(m_d2dDevice->CreateDeviceContext(deviceOptions, &m_d2dDeviceContext));
	DX::ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &m_dWriteFactory));
}

void GameFramework::CreateShaderVariable()
{
	m_cbGameFramework = Utile::CreateBufferResource(m_device, m_commandList, NULL, Utile::GetConstantBufferSize<cbGameFramework>(), 1, D3D12_HEAP_TYPE_UPLOAD, {});
	m_cbGameFramework->Map(0, NULL, reinterpret_cast<void**>(&m_pcbGameFramework));
	m_cbGameFrameworkData = make_unique<cbGameFramework>();

	m_cbPostGameFramework = Utile::CreateBufferResource(m_device, m_commandList, NULL, Utile::GetConstantBufferSize<cbPostGameFramework>(), 1, D3D12_HEAP_TYPE_UPLOAD, {});
	m_cbPostGameFramework->Map(0, NULL, reinterpret_cast<void**>(&m_pcbPostGameFramework));
	m_cbPostGameFrameworkData = make_unique<cbPostGameFramework>();
}

void GameFramework::Render2D() const
{
	// Acquire our wrapped render target resource for the current back buffer.
	m_d3d11On12Device->AcquireWrappedResources(m_wrappedBackBuffers[m_frameIndex].GetAddressOf(), 1);

	// Render text directly to the back buffer.
	m_d2dDeviceContext->SetTarget(m_d2dRenderTargets[m_frameIndex].Get());
	m_d2dDeviceContext->BeginDraw();

	if (m_scene) m_scene->Render2D(m_d2dDeviceContext);

	DX::ThrowIfFailed(m_d2dDeviceContext->EndDraw());

	// Release our wrapped render target resource. Releasing 
	// transitions the back buffer resource to the state specified
	// as the OutState when the wrapped resource was created.
	m_d3d11On12Device->ReleaseWrappedResources(m_wrappedBackBuffers[m_frameIndex].GetAddressOf(), 1);

	// Flush to submit the 11 command list to the shared command queue.
	m_d3d11DeviceContext->Flush();
}

void GameFramework::Update(FLOAT deltaTime)
{
	if (m_nextScene != eSceneType::NONE)
		ChangeToNextScene();

	wstring title{ TEXT("Hello, Planet! (") + to_wstring(static_cast<int>(m_timer->GetFPS())) + TEXT("FPS)") };
	SetWindowText(m_hWnd, title.c_str());

	// 상수버퍼
	m_cbGameFrameworkData->deltaTime = deltaTime;
	m_cbGameFrameworkData->screenWidth = g_width;
	m_cbGameFrameworkData->screenWidth = g_height;

	// 후처리 상수버퍼
	if (m_cbPostGameFrameworkData->fadeType == -1 &&
		m_cbPostGameFrameworkData->fadeTimer >= m_cbPostGameFrameworkData->fadeTime)
	{
		m_nextScene = m_nextTempScene;
		m_nextTempScene = eSceneType::NONE;
	}
	else if (m_cbPostGameFrameworkData->fadeType == 1 &&
			 m_cbPostGameFrameworkData->fadeTimer >= m_cbPostGameFrameworkData->fadeTime)
	{
		m_cbPostGameFrameworkData->fadeType = 0;
	}
	m_cbPostGameFrameworkData->fadeTimer += deltaTime;
}

void GameFramework::UpdateShaderVariable() const
{
	memcpy(m_pcbGameFramework, m_cbGameFrameworkData.get(), sizeof(cbGameFramework));
	m_commandList->SetGraphicsRootConstantBufferView(4, m_cbGameFramework->GetGPUVirtualAddress());

	ID3D12DescriptorHeap* ppHeaps[]{ Texture::s_srvHeap.Get() };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
}

void GameFramework::UpdatePostShaderVariable() const
{
	memcpy(m_pcbPostGameFramework, m_cbPostGameFrameworkData.get(), sizeof(cbPostGameFramework));
	m_commandList->SetComputeRootConstantBufferView(0, m_cbPostGameFramework->GetGPUVirtualAddress());
}

void GameFramework::PopulateCommandList() const
{
	DX::ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());
	DX::ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), NULL));

	// Set necessary state
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	// Indicate that the back buffer will be used as a render target
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// 렌더타겟, 깊이스텐실 버퍼 바인딩
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{ m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(m_frameIndex), m_rtvDescriptorSize };
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle{ m_dsvHeap->GetCPUDescriptorHandleForHeapStart() };
	m_commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

	// 렌더타겟, 깊이스텐실 버퍼 지우기
	constexpr FLOAT clearColor[]{ 0.15625f, 0.171875f, 0.203125f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, NULL);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	// 렌더링
	if (m_scene)
	{
		UpdateShaderVariable();
		m_scene->UpdateShaderVariable(m_commandList);
		m_scene->PreRender(m_commandList);
		m_scene->Render(m_commandList, rtvHandle, dsvHandle);
		m_scene->PostProcessing(m_commandList, m_postRootSignature, m_renderTargets[m_frameIndex]);
	}

	// 페이드 인/아웃
	if (m_cbPostGameFrameworkData->fadeType != 0)
	{
		m_commandList->SetComputeRootSignature(m_postRootSignature.Get());
		UpdatePostShaderVariable();
		m_fadeFilter->Excute(m_commandList, m_postRootSignature, m_renderTargets[m_frameIndex]);
	}

	//m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	DX::ThrowIfFailed(m_commandList->Close());
}

void GameFramework::WaitForPreviousFrame()
{
	// Schedule a Signal command in the queue.
	const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
	DX::ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
	{
		DX::ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	// Update the frame index.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Set the fence value for the next frame.
	m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}

void GameFramework::WaitForGpu()
{
	// Schedule a Signal command in the queue.
	DX::ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));

	// Wait until the fence has been processed.
	DX::ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

	// Increment the fence value for the current frame.
	m_fenceValues[m_frameIndex]++;
}

void GameFramework::ChangeToNextScene()
{
	WaitForPreviousFrame();

	switch (m_nextScene)
	{
	case eSceneType::LOADING:
		m_scene = make_unique<LoadingScene>();
		break;
	case eSceneType::MAIN:
		m_scene = make_unique<MainScene>();
		g_audioEngine.ChangeMusic("LOBBY");
		break;
	case eSceneType::LOBBY:
		m_scene = make_unique<LobbyScene>();
		break;
	case eSceneType::GAME:
	{
		// 로비 씬에서 만든 플레이어와 멀티플레이어를 게임씬으로 옮김
		auto lobbyScene{ reinterpret_cast<LobbyScene*>(m_scene.get()) };
		auto gameScene{ make_unique<GameScene>() };
		auto& player{ lobbyScene->GetPlayer() };
		player->SetIsMultiplayer(FALSE);
		gameScene->SetPlayer(player);
		gameScene->SetMultiPlayers(lobbyScene->GetMultiPlayers());
		m_scene = move(gameScene);
		g_audioEngine.ChangeMusic("INGAME");
		break;
	}
	}
	m_nextScene = eSceneType::NONE;

	DX::ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());
	DX::ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), NULL));

	m_scene->OnInit(m_device, m_commandList, m_rootSignature, m_postRootSignature, m_d2dDeviceContext, m_dWriteFactory);

	m_commandList->Close();
	ID3D12CommandList* ppCommandList[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

	WaitForGpu();

	m_scene->OnInitEnd();

	// 페이드 인
	SetFadeIn(0.3f);
}

BOOL GameFramework::ConnectServer()
{
	wcout.imbue(locale("korean"));
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return FALSE;

	// socket 생성
	g_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (g_socket == INVALID_SOCKET)
		return FALSE;

	// connect
	SOCKADDR_IN server_address{};
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, g_serverIP.c_str(), &(server_address.sin_addr.s_addr));

	if (connect(g_socket, reinterpret_cast<SOCKADDR*>(&server_address), sizeof(server_address)) == SOCKET_ERROR)
		return FALSE;

	g_isConnected = TRUE;
	return TRUE;
}

void GameFramework::ProcessClient()
{
	if (m_scene) m_scene->ProcessClient();
}

void GameFramework::SetIsActive(BOOL isActive)
{
	m_isActive = isActive;
}

void GameFramework::SetIsFullScreen(BOOL isFullScreen)
{
	m_isFullScreen = isFullScreen;
}

void GameFramework::SetNextScene(eSceneType scene)
{
	SetFadeOut(0.3f);
	m_nextTempScene = scene;
}

void GameFramework::SetFadeIn(FLOAT time)
{
	m_cbPostGameFrameworkData->fadeType = 1;
	m_cbPostGameFrameworkData->fadeTimer = 0.0f;
	m_cbPostGameFrameworkData->fadeTime = time;
}

void GameFramework::SetFadeOut(FLOAT time)
{
	m_cbPostGameFrameworkData->fadeType = -1;
	m_cbPostGameFrameworkData->fadeTimer = 0.0f;
	m_cbPostGameFrameworkData->fadeTime = time;
}

ComPtr<IDWriteFactory> GameFramework::GetDWriteFactory() const
{
	return m_dWriteFactory;
}

ComPtr<ID3D12CommandQueue> GameFramework::GetCommandQueue() const
{
	return m_commandQueue;
}

Scene* GameFramework::GetScene() const
{
	return m_scene.get();
}

HWND GameFramework::GetWindow() const
{
	return m_hWnd;
}

BOOL GameFramework::isActive() const
{
	return m_isActive;
}

BOOL GameFramework::isFullScreen() const
{
	return m_isFullScreen;
}

RECT GameFramework::GetLastWindowRect() const
{
	return m_lastWindowRect;
}

RECT GameFramework::GetFullScreenRect() const
{
	return m_fullScreenRect;
}

ComPtr<ID3D12Resource> GameFramework::GetDepthStencil() const
{
	return m_depthStencil;
}
