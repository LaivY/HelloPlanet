#pragma once
#include "stdafx.h"
#include "shader.h"

class OutlineFilter
{
public:
	OutlineFilter(const ComPtr<ID3D12Device>& device);
	~OutlineFilter() = default;

	void Excute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& postRootSignature, const ComPtr<ID3D12Resource>& depthStencil, const ComPtr<ID3D12Resource>& renderTarget);

private:
	ComPtr<ID3D12Resource>			m_buffer0; // 스텐실버퍼
	ComPtr<ID3D12Resource>			m_buffer1; // 렌더타겟

	ComPtr<ID3D12DescriptorHeap>	m_srvUavHeap;

	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_b0cpuSrvHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	m_b0gpuSrvHandle;

	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_b1cpuUavHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	m_b1gpuUavHandle;

	ComPtr<ID3D12DescriptorHeap>	m_dsvHeap;


	shared_ptr<Shader>				m_shader;
};