﻿#include "stdafx.h"
#include "DDSTextureLoader12.h"
#include "texture.h"

void Texture::CreateTexture(const ComPtr<ID3D12Device>& device, DXGI_FORMAT textureFotmat, DXGI_FORMAT shaderResourceViewFormat, UINT width, UINT height, UINT rootParameterIndex)
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

	// 셰이더 리소스 뷰 서술자힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DX::ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));

	// 셰이더 리소스 뷰 생성
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = shaderResourceViewFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.PlaneSlice = (shaderResourceViewFormat == DXGI_FORMAT_X24_TYPELESS_G8_UINT) ? 1 : 0;
	device->CreateShaderResourceView(buffer.Get(), &srvDesc, m_srvHeap->GetCPUDescriptorHandleForHeapStart());

	m_textures.push_back(make_pair(rootParameterIndex, buffer));
}

void Texture::Copy(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& src, D3D12_RESOURCE_STATES currSrcResourceState)
{
	const auto& dst{ m_textures.front().second };
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dst.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(src.Get(), currSrcResourceState, D3D12_RESOURCE_STATE_COPY_SOURCE));
	commandList->CopyResource(dst.Get(), src.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dst.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(src.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, currSrcResourceState));
}

void Texture::LoadTextureFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT rootParameterIndex, const wstring& fileName)
{
	ComPtr<ID3D12Resource> textureBuffer, textureUploadBuffer;

	// DDS 텍스쳐 로딩
	unique_ptr<uint8_t[]> ddsData;
	vector<D3D12_SUBRESOURCE_DATA> subresources;
	DDS_ALPHA_MODE ddsAlphaMode{ DDS_ALPHA_MODE_UNKNOWN };
	DX::ThrowIfFailed(LoadDDSTextureFromFileEx(device.Get(), fileName.c_str(), 0, D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_DEFAULT, &textureBuffer, ddsData, subresources, &ddsAlphaMode));

	// 디폴트 힙으로 데이터 복사하기 위한 업로드 힙 생성
	UINT nSubresources{ (UINT)subresources.size() };
	UINT64 nBytes{ GetRequiredIntermediateSize(textureBuffer.Get(), 0, nSubresources) };
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(nBytes),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL,
		IID_PPV_ARGS(&textureUploadBuffer)
	));

	// subresources에 있는 데이터를 textureBuffer로 복사
	UpdateSubresources(commandList.Get(), textureBuffer.Get(), textureUploadBuffer.Get(), 0, 0, nSubresources, subresources.data());

	// 리소스 베리어 설정
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(textureBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	// 저장
	m_textures.push_back(make_pair(rootParameterIndex, textureBuffer));
	m_textureUploadBuffers.push_back(textureUploadBuffer);
}

void Texture::CreateTextureFromLoadedFiles(const ComPtr<ID3D12Device>& device)
{
	// 셰이더 리소스 뷰 서술자 생성
	if (m_srvHeap) m_srvHeap.Reset();
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = static_cast<UINT>(m_textures.size()); // SRV 개수
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));

	// 셰이더 리소스 뷰 생성
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvDescriptorHandle{ m_srvHeap->GetCPUDescriptorHandleForHeapStart() };
	for (const auto& [_, texture] : m_textures)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = texture->GetDesc().Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = -1;
		device->CreateShaderResourceView(texture.Get(), &srvDesc, srvDescriptorHandle);
		srvDescriptorHandle.Offset(g_cbvSrvDescriptorIncrementSize);
	}
}

void Texture::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList, int index)
{
	ID3D12DescriptorHeap* ppHeaps[]{ m_srvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvDescriptorHandle{ m_srvHeap->GetGPUDescriptorHandleForHeapStart() };

	if (index) // index번째의 텍스쳐를 업데이트함
	{
		srvDescriptorHandle.Offset(index, g_cbvSrvDescriptorIncrementSize);
		commandList->SetGraphicsRootDescriptorTable(m_textures[index].first, srvDescriptorHandle);
	}
	else // 모든 텍스쳐들을 업데이트함
	{
		for (const auto& [rootParameterIndex, _] : m_textures)
		{
			commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, srvDescriptorHandle);
			srvDescriptorHandle.Offset(g_cbvSrvDescriptorIncrementSize);
		}
	}
}

void Texture::ReleaseUploadBuffer()
{
	for (auto& textureUploadBuffer : m_textureUploadBuffers)
		textureUploadBuffer.Reset();
}