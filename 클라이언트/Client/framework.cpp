#include "framework.h"

GameFramework::GameFramework(UINT width, UINT height) :
	m_width{ width }, m_height{ height }, m_frameIndex{ 0 }, m_rtvDescriptorSize{ 0 }
{
	m_aspectRatio = static_cast<FLOAT>(width) / static_cast<FLOAT>(height);
}

GameFramework::~GameFramework()
{

}

void GameFramework::GameLoop()
{
	m_timer.Tick();
	if (m_isActive)
	{
		OnMouseEvent();
		OnKeyboardEvent();
	}
	OnUpdate(m_timer.GetDeltaTime());
	OnRender();
}

void GameFramework::OnInit(HINSTANCE hInstance, HWND hWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hWnd;

	LoadPipeline();
	LoadAssets();
}

void GameFramework::OnUpdate(FLOAT deltaTime)
{
	Update(deltaTime);
	if (m_scene) m_scene->OnUpdate(deltaTime);
}

void GameFramework::OnRender()
{
	PopulateCommandList();
	ID3D12CommandList* ppCommandList[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);
	DX::ThrowIfFailed(m_swapChain->Present(1, 0));
	WaitForPreviousFrame();
}

void GameFramework::OnDestroy()
{
	WaitForPreviousFrame();
	CloseHandle(m_fenceEvent);
}

void GameFramework::OnMouseEvent()
{
	if (m_scene) m_scene->OnMouseEvent(m_hWnd, m_width, m_height, m_timer.GetDeltaTime());
}

void GameFramework::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (m_scene) m_scene->OnMouseEvent(hWnd, message, wParam, lParam);
}

void GameFramework::OnKeyboardEvent()
{
	if (m_scene) m_scene->OnKeyboardEvent(m_timer.GetDeltaTime());
}

void GameFramework::OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (m_scene) m_scene->OnKeyboardEvent(hWnd, message, wParam, lParam);
}


void GameFramework::Update(FLOAT deltaTime)
{
	wstring title{ TEXT("DirectX12 (") + to_wstring(static_cast<int>(m_timer.GetFPS())) + TEXT("FPS)") };
	SetWindowText(m_hWnd, title.c_str());
}

void GameFramework::CreateDevice(const ComPtr<IDXGIFactory4>& factory)
{
	ComPtr<IDXGIAdapter1> adapter;
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter); ++i)
	{
		DXGI_ADAPTER_DESC1 adapterDesc;
		adapter->GetDesc1(&adapterDesc);
		if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)))) break;
	}
	if (!m_device)
	{
		factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
		DX::ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));
	}

	// 서술자힙 크기
	g_cbvSrvDescriptorIncrementSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void GameFramework::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	DX::ThrowIfFailed(m_device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_commandQueue)));
}

void GameFramework::CreateSwapChain(const ComPtr<IDXGIFactory4>& factory)
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
	swapChainDesc.BufferDesc.Width = m_width;
	swapChainDesc.BufferDesc.Height = m_height;
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
	DX::ThrowIfFailed(factory->CreateSwapChain(m_commandQueue.Get(), &swapChainDesc, &swapChain));
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
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{ m_rtvHeap->GetCPUDescriptorHandleForHeapStart() };
	for (UINT i = 0; i < FrameCount; ++i)
	{
		m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
		m_device->CreateRenderTargetView(m_renderTargets[i].Get(), NULL, rtvHandle);
		rtvHandle.Offset(m_rtvDescriptorSize);
	}
}

void GameFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = SCREEN_WIDTH;
	resourceDesc.Height = SCREEN_HEIGHT;
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
	CD3DX12_DESCRIPTOR_RANGE ranges[3];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // Texture2D g_texture		 : t0
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // Texture2D g_detailTexture : t1
	ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND); // Texture2D g_shadowMap	 : t2

	// 자주 갱신하는 순서대로 해야 성능에 좋음
	CD3DX12_ROOT_PARAMETER rootParameter[7];
	rootParameter[0].InitAsConstants(16, 0, 0);		// cbGameObject : b0
	rootParameter[1].InitAsConstantBufferView(1);	// cbCamera : b1
	rootParameter[2].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[3].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[4].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameter[5].InitAsConstantBufferView(2);	// cbScene : b2
	rootParameter[6].InitAsConstantBufferView(3);	// cbGameFramework : b3

	CD3DX12_STATIC_SAMPLER_DESC samplerDesc[2];
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

void GameFramework::CreatePostProcessRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE ranges[2];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // Texture2D g_input				: t0;
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // RWTexture2D<float4> g_output	: u0;
	
	CD3DX12_ROOT_PARAMETER rootParameter[2];
	rootParameter[0].InitAsDescriptorTable(1, &ranges[0]);
	rootParameter[1].InitAsDescriptorTable(1, &ranges[1]);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameter), rootParameter, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> signature, error;
	DX::ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	DX::ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_postProcessRootSignature)));
}

void GameFramework::CreateShaderVariable()
{
	ComPtr<ID3D12Resource> dummy;
	UINT cbByteSize{ (sizeof(cbGameFramework) + 255) & ~255 };
	m_cbGameFramework = CreateBufferResource(m_device, m_commandList, NULL, cbByteSize, 1, D3D12_HEAP_TYPE_UPLOAD, {}, dummy);
	m_cbGameFramework->Map(0, NULL, reinterpret_cast<void**>(&m_pcbGameFramework));
	m_cbGameFrameworkData = make_unique<cbGameFramework>();
}

void GameFramework::UpdateShaderVariable() const
{
	m_cbGameFrameworkData->deltaTime = m_timer.GetDeltaTime();
	memcpy(m_pcbGameFramework, m_cbGameFrameworkData.get(), sizeof(cbGameFramework));
	m_commandList->SetGraphicsRootConstantBufferView(6, m_cbGameFramework->GetGPUVirtualAddress());
}

void GameFramework::LoadPipeline()
{
	// 팩토리 생성
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif
	ComPtr<IDXGIFactory4> factory;
	DX::ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	// 디바이스 생성
	CreateDevice(factory);

	// 명령큐 생성
	CreateCommandQueue();

	// 스왑체인 생성
	CreateSwapChain(factory);

	// 렌더타겟뷰, 깊이스텐실뷰의 서술자힙 생성
	CreateRtvDsvDescriptorHeap();

	// 렌더타겟뷰 생성
	CreateRenderTargetView();

	// 깊이스텐실뷰 생성
	CreateDepthStencilView();

	// 루트시그니쳐 생성
	CreateRootSignature();

	// 블러를 위한 계산셰이더 루트시그니쳐 생성
	CreatePostProcessRootSignature();

	// 명령할당자 생성
	DX::ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

	// 명령리스트 생성
	DX::ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
	DX::ThrowIfFailed(m_commandList->Close());

	// 펜스 생성
	DX::ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_fenceValue = 1;

	// alt + enter 금지
	factory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
}

void GameFramework::LoadAssets()
{
	// 명령을 추가할 것이기 때문에 Reset
	m_commandList->Reset(m_commandAllocator.Get(), NULL);

	// 셰이더 변수 생성
	CreateShaderVariable();

	// 씬 생성, 초기화
	m_scene = make_unique<Scene>();
	m_scene->OnInit(m_device, m_commandList, m_rootSignature, m_postProcessRootSignature);

	// 명령 제출
	m_commandList->Close();
	ID3D12CommandList* ppCommandList[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandList), ppCommandList);

	// 명령들이 완료될 때까지 대기
	WaitForPreviousFrame();

	// 디폴트 버퍼로의 복사가 완료됐으므로 업로드 버퍼를 해제한다.
	m_scene->ReleaseUploadBuffer();

	// 타이머 초기화
	m_timer.Tick();
}

void GameFramework::PopulateCommandList() const
{
	DX::ThrowIfFailed(m_commandAllocator->Reset());
	DX::ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), NULL));

	// Set necessary state
	m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	// 프레임워크 셰이더 변수 최신화
	UpdateShaderVariable();

	// 씬 셰이더 변수 최신화
	if (m_scene) m_scene->UpdateShaderVariable(m_commandList);

	// 그림자맵에 깊이 버퍼 쓰기
	if (m_scene) m_scene->RenderToShadowMap(m_commandList);

	// Indicate that the back buffer will be used as a render target
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// 렌더타겟, 깊이스텐실 버퍼 바인딩
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{ m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(m_frameIndex), m_rtvDescriptorSize };
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle{ m_dsvHeap->GetCPUDescriptorHandleForHeapStart() };
	m_commandList->OMSetRenderTargets(1, &rtvHandle, TRUE, &dsvHandle);

	// 렌더타겟, 깊이스텐실 버퍼 지우기
	const FLOAT clearColor[]{ 1.0f, 1.0f, 1.0f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, NULL);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	// 씬 렌더링
	if (m_scene) m_scene->Render(m_commandList, rtvHandle, dsvHandle);

	// 후처리(블러링)
	if (m_scene && m_scene->doPostProcess())
	{
		// 블러링 진행
		m_scene->PostRenderProcess(m_commandList, m_postProcessRootSignature, m_renderTargets[m_frameIndex]);

		// 블러링한 결과를 렌더 타겟으로 복사하기 위한 리소스 베리어 설정
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST));

		// 복사
		m_commandList->CopyResource(m_renderTargets[m_frameIndex].Get(), m_scene->GetPostRenderProcessResult().Get());

		// 출력 상태로 리소스 베리어 설정
		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));
	}
	else m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	DX::ThrowIfFailed(m_commandList->Close());
}

void GameFramework::WaitForPreviousFrame()
{
	const UINT64 fence{ m_fenceValue };
	DX::ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
	++m_fenceValue;

	if (m_fence->GetCompletedValue() < fence)
	{
		DX::ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void GameFramework::SetIsActive(BOOL isActive)
{
	m_isActive = isActive;
	if (m_isActive)
		ShowCursor(FALSE);
	else
		ShowCursor(TRUE);
}