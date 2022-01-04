#include "blurFilter.h"

BlurFilter::BlurFilter(const ComPtr<ID3D12Device>& device)
{
	CreateBlurTexture(device);
	CreateSrvDsvDescriptorHeap(device);
	CreateSrvUav(device);
}

void BlurFilter::CreateBlurTexture(const ComPtr<ID3D12Device>& device)
{
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, SCREEN_WIDTH, SCREEN_HEIGHT, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_COMMON,
		NULL,
		IID_PPV_ARGS(&m_blurMap0)
	));

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, SCREEN_WIDTH, SCREEN_HEIGHT, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_COMMON,
		NULL,
		IID_PPV_ARGS(&m_blurMap1)
	));
}

void BlurFilter::CreateSrvDsvDescriptorHeap(const ComPtr<ID3D12Device>& device)
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DX::ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvUavHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrvUavHeapStart{ m_srvUavHeap->GetCPUDescriptorHandleForHeapStart() };
	m_blur0CpuSrvHandle = cpuSrvUavHeapStart;
	m_blur0CpuUavHandle = cpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);
	m_blur1CpuSrvHandle = cpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);
	m_blur1CpuUavHandle = cpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrvUavHeapStart{ m_srvUavHeap->GetGPUDescriptorHandleForHeapStart() };
	m_blur0GpuSrvHandle = gpuSrvUavHeapStart;
	m_blur0GpuUavHandle = gpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);
	m_blur1GpuSrvHandle = gpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);
	m_blur1GpuUavHandle = gpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);
}

void BlurFilter::CreateSrvUav(const ComPtr<ID3D12Device>& device)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	device->CreateShaderResourceView(m_blurMap0.Get(), &srvDesc, m_blur0CpuSrvHandle);
	device->CreateUnorderedAccessView(m_blurMap0.Get(), nullptr, &uavDesc, m_blur0CpuUavHandle);

	device->CreateShaderResourceView(m_blurMap1.Get(), &srvDesc, m_blur1CpuSrvHandle);
	device->CreateUnorderedAccessView(m_blurMap1.Get(), nullptr, &uavDesc, m_blur1CpuUavHandle);
}

void BlurFilter::Excute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& rootSignature,
						const shared_ptr<Shader>& horzBlurShader, const shared_ptr<Shader>& vertBlurShader, const ComPtr<ID3D12Resource>& input)
{
	// 루트시그시처(후처리) 설정
	commandList->SetComputeRootSignature(rootSignature.Get());

	// 렌더링 결과를 m_blurMap0에 복사, 계산 셰이더에서 읽을 수 있게 리소스 베리어 설정
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_blurMap0.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->CopyResource(m_blurMap0.Get(), input.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_blurMap0.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	// 계산 셰이더에서 읽을 수 있게 리소스 베리어 설정
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_blurMap1.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	// 서술자힙, 테이블 설정
	ID3D12DescriptorHeap* ppHeaps[] = { m_srvUavHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// 블러링
	int blurCount{ 1 };
	for (int i = 0; i < blurCount; ++i)
	{
		// 가로 블러링 시작
		// PSO 설정
		commandList->SetPipelineState(horzBlurShader->GetPipelineState().Get());

		// 서술자 테이블 설정
		commandList->SetComputeRootDescriptorTable(0, m_blur0GpuSrvHandle);
		commandList->SetComputeRootDescriptorTable(1, m_blur1GpuUavHandle);

		// 쓰레드 그룹 설정, 계산 셰이더 실행
		// 성능 상 한 그룹의 쓰레드를 32의 배수로 설정하는게 좋다. 나는 256개로 설정했다.
		UINT groupCountX{ (UINT)ceilf(static_cast<float>(SCREEN_WIDTH) / 256.0f) };
		commandList->Dispatch(groupCountX, SCREEN_HEIGHT, 1);

		// 지금 m_blurMap1에 가로 블러링된 결과가 저장되어있다.
		// 이제 m_blurMap1을 세로 블러링해서 m_blurMap0에 저장하기 위해 리소스 베리어를 설정해준다.
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_blurMap0.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_blurMap1.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

		// 세로 블러링 시작
		// PSO 설정
		commandList->SetPipelineState(vertBlurShader->GetPipelineState().Get());

		// 서술자 테이블 설정
		commandList->SetComputeRootDescriptorTable(0, m_blur1GpuSrvHandle);
		commandList->SetComputeRootDescriptorTable(1, m_blur0GpuUavHandle);

		// 쓰레드 그룹 설정, 계산 셰이더 실행
		UINT groupCountY{ (UINT)ceilf(static_cast<float>(SCREEN_HEIGHT) / 256.0f) };
		commandList->Dispatch(SCREEN_WIDTH, groupCountY, 1);

		// 리소스 베리어 원상태로 설정
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_blurMap0.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_blurMap1.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}
}