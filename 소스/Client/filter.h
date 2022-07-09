#pragma once

class Filter abstract
{
public:
	Filter() = default;
	Filter(const ComPtr<ID3D12Device>& device);
	virtual ~Filter() = default;

	virtual void Excute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& postRootSignature, const ComPtr<ID3D12Resource>& renderTarget) = 0;

protected:
	ComPtr<ID3D12Resource>			m_buffer0;
	ComPtr<ID3D12Resource>			m_buffer1;
	ComPtr<ID3D12DescriptorHeap>	m_srvUavHeap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_b0cpuSrvHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	m_b0gpuSrvHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_b1cpuUavHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	m_b1gpuUavHandle;
};

class FadeFilter : public Filter
{
public:
	FadeFilter(const ComPtr<ID3D12Device>& device);
	~FadeFilter() = default;

	virtual void Excute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& postRootSignature, const ComPtr<ID3D12Resource>& renderTarget);
};

class BlurFilter : public Filter
{
public:
	BlurFilter(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	~BlurFilter() = default;

	virtual void Excute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& postRootSignature, const ComPtr<ID3D12Resource>& renderTarget);

private:
	struct ConstantBuffer
	{
		INT radius;
	};

private:
	constexpr static int			BLUR_COUNT = 5;
	ComPtr<ID3D12Resource>			m_constantBuffer[BLUR_COUNT];
	ConstantBuffer*					m_pConstantBuffer[BLUR_COUNT];

	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_b0cpuUavHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	m_b0gpuUavHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_b1cpuSrvHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE	m_b1gpuSrvHandle;
};