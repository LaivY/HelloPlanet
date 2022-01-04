#pragma once
#include "stdafx.h"
#include "shader.h"

class BlurFilter
{
public:
	BlurFilter(const ComPtr<ID3D12Device>& device);
	~BlurFilter() = default;

	void CreateBlurTexture(const ComPtr<ID3D12Device>& device);
	void CreateSrvDsvDescriptorHeap(const ComPtr<ID3D12Device>& device);
	void CreateSrvUav(const ComPtr<ID3D12Device>& device);

	void Excute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& rootSignature,
				const shared_ptr<Shader>& horzBlurShader, const shared_ptr<Shader>& vertBlurShader, const ComPtr<ID3D12Resource>& input);

	ComPtr<ID3D12Resource> GetResult() const { return m_blurMap0; }

private:
	ComPtr<ID3D12Resource>			m_blurMap0; // 1. 렌더 타겟 복사본, 3. 최종 결과물
	ComPtr<ID3D12Resource>			m_blurMap1; // 2. 가로 블러 결과

	ComPtr<ID3D12DescriptorHeap>	m_srvUavHeap;

	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_blur0CpuSrvHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_blur0CpuUavHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	m_blur0GpuSrvHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	m_blur0GpuUavHandle;

	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_blur1CpuSrvHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_blur1CpuUavHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	m_blur1GpuSrvHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	m_blur1GpuUavHandle;
};