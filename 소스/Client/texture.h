#pragma once
#include "stdafx.h"
#include "DDSTextureLoader12.h"

class Texture
{
public:
	Texture() = default;
	~Texture() = default;

	void LoadTextureFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT rootParameterIndex, const wstring& fileName);
	void CreateTexture(const ComPtr<ID3D12Device>& device);
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList, int index = 0);
	void ReleaseUploadBuffer();

	size_t GetTextureCount() const { return m_textures.size(); }

protected:
	ComPtr<ID3D12DescriptorHeap>				m_srvHeap;
	vector<pair<UINT, ComPtr<ID3D12Resource>>>	m_textures;
	vector<ComPtr<ID3D12Resource>>				m_textureUploadBuffers;
};

class StencilTexture
{
public:
	StencilTexture(const ComPtr<ID3D12Device>& device);
	~StencilTexture() = default;

	void Copy(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& depthStencil);
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList);

private:
	ComPtr<ID3D12Resource>			m_buffer;
	ComPtr<ID3D12DescriptorHeap>	m_srvHeap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_srvHandle;
};