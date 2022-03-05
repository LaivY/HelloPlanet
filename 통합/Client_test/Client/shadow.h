#pragma once
#include "stdafx.h"

class ShadowMap
{
public:
	ShadowMap(const ComPtr<ID3D12Device>& device, UINT width, UINT height);
	~ShadowMap() = default;

	void CreateShadowMapTexture(const ComPtr<ID3D12Device>& device);
	void CreateSrvDsvDescriptorHeap(const ComPtr<ID3D12Device>& device);
	void CreateSrvDsv(const ComPtr<ID3D12Device>& device);

	D3D12_VIEWPORT GetViewport() const { return m_viewport; }
	D3D12_RECT GetScissorRect() const { return m_scissorRect; }

	ComPtr<ID3D12DescriptorHeap> GetSrvHeap() const { return m_srvHeap; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuSrvHandle() const { return m_cpuSrvHandle; }
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuSrvHandle() const { return m_gpuSrvHandle; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuDsvHandle() const { return m_cpuDsvHandle; }

	ComPtr<ID3D12Resource> GetShadowMap() const { return m_shadowMap; }

private:
	D3D12_VIEWPORT					m_viewport;
	D3D12_RECT						m_scissorRect;
	DXGI_FORMAT						m_format;
	UINT							m_width;
	UINT							m_height;

	ComPtr<ID3D12DescriptorHeap>	m_srvHeap;
	ComPtr<ID3D12DescriptorHeap>	m_dsvHeap;

	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_cpuSrvHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	m_gpuSrvHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_cpuDsvHandle;

	ComPtr<ID3D12Resource>			m_shadowMap;
};