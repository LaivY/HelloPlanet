#include "outlineFilter.h"

OutlineFilter::OutlineFilter(const ComPtr<ID3D12Device>& device)
{
	/*
	1. m_buffer0에 렌더타겟을 복사한다.
	2. m_buffer0을 g_input으로, m_buffer1을 g_output으로 설정하고 가로 필터링을 해서 저장한다.
	3. m_buffer1을 g_input으로, m_buffer0을 g_output으로 설정하고 세로 필터링을 해서 저장한다.
	4. m_buffer0을 렌더타겟으로 복사한다.
	*/

	// 텍스쳐 생성
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, g_width, g_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
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
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DX::ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvUavHeap)));

	// 뷰 핸들 설정
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrvUavHeapStart{ m_srvUavHeap->GetCPUDescriptorHandleForHeapStart() };
	m_b0cpuSrvHandle = cpuSrvUavHeapStart;
	m_b0cpuUavHandle = cpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);
	m_b1cpuSrvHandle = cpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);
	m_b1cpuUavHandle = cpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrvUavHeapStart{ m_srvUavHeap->GetGPUDescriptorHandleForHeapStart() };
	m_b0gpuSrvHandle = gpuSrvUavHeapStart;
	m_b0gpuUavHandle = gpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);
	m_b1gpuSrvHandle = gpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);
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
	device->CreateUnorderedAccessView(m_buffer0.Get(), nullptr, &uavDesc, m_b0cpuUavHandle);
	device->CreateShaderResourceView(m_buffer1.Get(), &srvDesc, m_b0cpuSrvHandle);
	device->CreateUnorderedAccessView(m_buffer1.Get(), nullptr, &uavDesc, m_b1cpuUavHandle);
}

void OutlineFilter::Excute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& postRootSignature, const ComPtr<ID3D12Resource>& renderTarget)
{

}

ComPtr<ID3D12Resource> OutlineFilter::GetResult() const
{
	return m_buffer0;
}
