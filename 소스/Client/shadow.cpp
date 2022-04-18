#include "shadow.h"

ShadowMap::ShadowMap(const ComPtr<ID3D12Device>& device, UINT width, UINT height, INT count)
	: m_viewport{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f }, 
	  m_scissorRect{ 0, 0, static_cast<int>(width), static_cast<int>(height) }, 
	  m_format{ DXGI_FORMAT_R24G8_TYPELESS }, m_width{ width }, m_height{ height }, m_count{ count }
{
	CreateShadowMapTexture(device);
	CreateSrvDsvDescriptorHeap(device);
	CreateSrvDsv(device);
}

void ShadowMap::CreateShadowMapTexture(const ComPtr<ID3D12Device>& device)
{
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(m_format, m_width, m_height, m_count, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&CD3DX12_CLEAR_VALUE{ DXGI_FORMAT_D24_UNORM_S8_UINT, 1.0f, 0 },
		IID_PPV_ARGS(&m_shadowMap)
	));
}

void ShadowMap::CreateSrvDsvDescriptorHeap(const ComPtr<ID3D12Device>& device)
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DX::ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	DX::ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

	m_cpuSrvHandle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
	m_gpuSrvHandle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
	m_cpuDsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void ShadowMap::CreateSrvDsv(const ComPtr<ID3D12Device>& device)
{
	// Create SRV to resource so we can sample the shadow map in a shader program.
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvDescriptorHandle{ m_cpuSrvHandle };
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	//srvDesc.Texture2D.MostDetailedMip = 0;
	//srvDesc.Texture2D.MipLevels = 1;
	//srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	//srvDesc.Texture2D.PlaneSlice = 0;

	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = 1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = m_count;
	srvDesc.Texture2DArray.PlaneSlice = 0;
	srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;

	device->CreateShaderResourceView(m_shadowMap.Get(), &srvDesc, srvDescriptorHandle);

	// Create DSV to resource so we can render to the shadow map.
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvDescriptorHandle{ m_cpuDsvHandle };
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
	dsvDesc.Texture2DArray.MipSlice = 0;
	dsvDesc.Texture2DArray.FirstArraySlice = 0;
	dsvDesc.Texture2DArray.ArraySize = m_count;
	device->CreateDepthStencilView(m_shadowMap.Get(), &dsvDesc, dsvDescriptorHandle);
}

ComPtr<ID3D12DescriptorHeap> ShadowMap::GetSrvHeap() const
{
	return m_srvHeap;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::GetCpuSrvHandle(INT index) const
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrvHandle{ m_cpuSrvHandle };
	return cpuSrvHandle.Offset(g_cbvSrvDescriptorIncrementSize * index);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShadowMap::GetGpuSrvHandle(INT index) const
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrvHandle{ m_gpuSrvHandle };
	return gpuSrvHandle.Offset(g_cbvSrvDescriptorIncrementSize * index);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::GetCpuDsvHandle() const
{
	return m_cpuDsvHandle;
}

ComPtr<ID3D12Resource> ShadowMap::GetShadowMap() const
{
	return m_shadowMap;
}

//ComPtr<ID3D12Resource> ShadowMap::GetShadowMap(INT index) const
//{
//	return m_shadowMaps[index];
//}
