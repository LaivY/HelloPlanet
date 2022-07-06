#include "stdafx.h"
#include "DDSTextureLoader12.h"
#include "texture.h"

ComPtr<ID3D12DescriptorHeap>	Texture::s_srvHeap;
vector<Texture::Resource>		Texture::s_resources;
vector<ComPtr<ID3D12Resource>>	Texture::s_uploadBuffers;

Texture::Texture() : m_count{ 0 }
{
	m_index = s_resources.size();
}

void Texture::CreateShaderResourceView(const ComPtr<ID3D12Device>& device)
{
	// 이미 셰이더 리소스 뷰가 있다면 해제
	if (s_srvHeap) s_srvHeap.Reset();

	// 셰이더 리소스 뷰 서술자 생성
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = static_cast<UINT>(s_resources.size());
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&s_srvHeap));

	// 셰이더 리소스 뷰 생성
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvDescriptorHandle{ s_srvHeap->GetCPUDescriptorHandleForHeapStart() };
	for (const auto& data : s_resources)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		switch (data.type)
		{
		case eTextureType::DEFAULT:
			srvDesc.Format = data.buffer->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = -1;
			break;
		case eTextureType::SHADOW:
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = Setting::SHADOWMAP_COUNT;
			srvDesc.Texture2DArray.PlaneSlice = 0;
			srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
			break;
		case eTextureType::DEPTH:
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.PlaneSlice = 0;
			break;
		case eTextureType::STENCIL:
			srvDesc.Format = DXGI_FORMAT_X24_TYPELESS_G8_UINT;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = 1;
			srvDesc.Texture2D.PlaneSlice = 1;
			break;
		}
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		device->CreateShaderResourceView(data.buffer.Get(), &srvDesc, srvDescriptorHandle);
		srvDescriptorHandle.Offset(g_cbvSrvDescriptorIncrementSize);
	}
}

void Texture::ReleaseUploadBuffer()
{
	s_uploadBuffers.clear();
}

void Texture::Create(const ComPtr<ID3D12Device>& device, DXGI_FORMAT textureFotmat, UINT width, UINT height, UINT rootParameterIndex, eTextureType type)
{
	// 버퍼 생성
	ComPtr<ID3D12Resource> buffer;
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(textureFotmat, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL,
		IID_PPV_ARGS(&buffer)
	));
	s_resources.emplace_back(buffer, rootParameterIndex, type);
	++m_count;
}

void Texture::Load(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT rootParameterIndex, const wstring& fileName)
{
	ComPtr<ID3D12Resource> buffer, uploadBuffer;

	// DDS 텍스쳐 로딩
	unique_ptr<uint8_t[]> ddsData;
	vector<D3D12_SUBRESOURCE_DATA> subresources;
	DDS_ALPHA_MODE ddsAlphaMode{ DDS_ALPHA_MODE_UNKNOWN };
	DX::ThrowIfFailed(LoadDDSTextureFromFileEx(device.Get(), fileName.c_str(), 0, D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_DEFAULT, &buffer, ddsData, subresources, &ddsAlphaMode));

	// 디폴트 힙으로 데이터 복사하기 위한 업로드 힙 생성
	UINT nSubresources{ static_cast<UINT>(subresources.size()) };
	UINT64 nBytes{ GetRequiredIntermediateSize(buffer.Get(), 0, nSubresources) };
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(nBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL,
		IID_PPV_ARGS(&uploadBuffer)
	));

	// subresources에 있는 데이터를 textureBuffer로 복사
	UpdateSubresources(commandList.Get(), buffer.Get(), uploadBuffer.Get(), 0, 0, nSubresources, subresources.data());

	// 리소스 베리어 설정
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	// 저장
	s_resources.emplace_back(buffer, rootParameterIndex, eTextureType::DEFAULT);
	s_uploadBuffers.push_back(move(uploadBuffer));
	++m_count;
}

void Texture::Copy(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& src, D3D12_RESOURCE_STATES currSrcResourceState)
{
	const auto& dst{ s_resources[m_index].buffer };
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dst.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(src.Get(), currSrcResourceState, D3D12_RESOURCE_STATE_COPY_SOURCE));
	commandList->CopyResource(dst.Get(), src.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dst.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(src.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, currSrcResourceState));
}

void Texture::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT index)
{
	int i{ static_cast<int>(m_index + min(index, m_count - 1)) };
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvDescriptorHandle{ s_srvHeap->GetGPUDescriptorHandleForHeapStart() };
	srvDescriptorHandle.Offset(i, g_cbvSrvDescriptorIncrementSize);
	commandList->SetGraphicsRootDescriptorTable(s_resources[i].rootParameterIndex, srvDescriptorHandle);
}

ComPtr<ID3D12Resource> Texture::GetBuffer(UINT index) const
{
	int i{ static_cast<int>(m_index + min(index, m_count - 1)) };
	return s_resources[i].buffer;
}

size_t Texture::GetCount() const
{
	return m_count;
}

ShadowTexture::ShadowTexture(const ComPtr<ID3D12Device>& device, UINT width, UINT height, INT count)
	: m_viewport{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f },
	  m_scissorRect{ 0, 0, static_cast<int>(width), static_cast<int>(height) }
{
	ComPtr<ID3D12Resource> buffer;
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R24G8_TYPELESS, width, height, count, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&CD3DX12_CLEAR_VALUE{ DXGI_FORMAT_D24_UNORM_S8_UINT, 1.0f, 0 },
		IID_PPV_ARGS(&buffer)
	));
	s_resources.emplace_back(buffer, 6, eTextureType::SHADOW);
	m_count = 1;

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	DX::ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));
	m_dsvCpuHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
	dsvDesc.Texture2DArray.MipSlice = 0;
	dsvDesc.Texture2DArray.FirstArraySlice = 0;
	dsvDesc.Texture2DArray.ArraySize = count;
	device->CreateDepthStencilView(buffer.Get(), &dsvDesc, m_dsvCpuHandle);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowTexture::GetDsvCpuHandle() const
{
	return m_dsvCpuHandle;
}

D3D12_VIEWPORT ShadowTexture::GetViewport() const
{
	return m_viewport;
}

D3D12_RECT ShadowTexture::GetScissorRect() const
{
	return m_scissorRect;
}
