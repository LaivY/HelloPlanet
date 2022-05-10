#include "outlineFilter.h"
#include "scene.h"

OutlineFilter::OutlineFilter(const ComPtr<ID3D12Device>& device)
{
	/*
	1. 렌더타겟을 m_buffer0에 복사한다.
	2. m_buffer0을 이용하여 소벨필터를 적용해서 외곽선을 검출하고 m_buffer1에 저장한다.
	3. m_buffer1을 렌더타겟으로 복사한다.
	*/

	// 텍스쳐 생성
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, g_width, g_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE),
		D3D12_RESOURCE_STATE_COMMON,
		NULL,
		IID_PPV_ARGS(&m_buffer0)
	));
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, g_width, g_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_COMMON,
		NULL,
		IID_PPV_ARGS(&m_buffer1)
	));

	// 서술자힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = 2;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DX::ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvUavHeap)));

	// 뷰 핸들 설정
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrvUavHeapStart{ m_srvUavHeap->GetCPUDescriptorHandleForHeapStart() };
	m_b0cpuSrvHandle = cpuSrvUavHeapStart;
	m_b1cpuUavHandle = cpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrvUavHeapStart{ m_srvUavHeap->GetGPUDescriptorHandleForHeapStart() };
	m_b0gpuSrvHandle = gpuSrvUavHeapStart;
	m_b1gpuUavHandle = gpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);

	// 뷰 생성
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

	device->CreateShaderResourceView(m_buffer0.Get(), &srvDesc, m_b0cpuSrvHandle);
	device->CreateUnorderedAccessView(m_buffer1.Get(), nullptr, &uavDesc, m_b1cpuUavHandle);

	// 계산셰이더 설정
	m_shader = Scene::s_shaders["OUTLINE"];
}

void OutlineFilter::Excute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& postRootSignature, const ComPtr<ID3D12Resource>& renderTarget)
{
	commandList->SetComputeRootSignature(postRootSignature.Get());
	commandList->SetPipelineState(m_shader->GetPipelineState().Get());

	// m_buffer0에 렌더타겟을 복사
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer0.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
	commandList->CopyResource(m_buffer0.Get(), renderTarget.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer0.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	// 서술자힙, 테이블 설정
	ID3D12DescriptorHeap* ppHeaps[] = { m_srvUavHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandList->SetComputeRootDescriptorTable(0, m_b0gpuSrvHandle);
	commandList->SetComputeRootDescriptorTable(1, m_b1gpuUavHandle);

	// m_buffer1을 셰이더에서 쓸 수 있도록 상태 변경
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer1.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	// 실행
	commandList->Dispatch(static_cast<UINT>(ceilf(g_width / 32.0f)), static_cast<UINT>(ceilf(g_height / 32.0f)), 1);

	// 렌더타겟에 m_buffer1을 복사
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer1.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	commandList->CopyResource(renderTarget.Get(), m_buffer1.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer1.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// m_buffer0 리소스 베리어 원상태로 변경
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer0.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COMMON));
}