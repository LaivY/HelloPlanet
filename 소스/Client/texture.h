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

	ComPtr<ID3D12DescriptorHeap> GetSrvHeap() const { return m_srvHeap; }
	size_t GetTextureCount() const { return m_textures.size(); }

protected:
	ComPtr<ID3D12DescriptorHeap>				m_srvHeap;
	vector<pair<UINT, ComPtr<ID3D12Resource>>>	m_textures;
	vector<ComPtr<ID3D12Resource>>				m_textureUploadBuffers;
};

class RenderTargetTexture : public Texture
{
public:
	RenderTargetTexture(const ComPtr<ID3D12Device>& device, UINT rootParameterIndex, UINT width, UINT height);
	~RenderTargetTexture() = default;

	ComPtr<ID3D12Resource> GetTexture() const { return m_textures.front().second; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle() const { return m_rtvHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuSrvHandle() const { return m_gpuSrvHandle; }

private:
	ComPtr<ID3D12DescriptorHeap>	m_rtvHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_rtvHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE		m_gpuSrvHandle;
};