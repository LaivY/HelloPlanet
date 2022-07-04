#include "stdafx.h"
#include "filter.h"
#include "scene.h"
#include "shader.h"

Filter::Filter(const ComPtr<ID3D12Device>& device)
{
	// 텍스쳐 생성
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, g_width, g_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE),
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

	// 셰이더 리소스 뷰 서술자힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
	srvHeapDesc.NumDescriptors = 2;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	DX::ThrowIfFailed(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvUavHeap)));

	// 뷰 핸들 설정
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuSrvUavHeapStart{ m_srvUavHeap->GetCPUDescriptorHandleForHeapStart() };
	m_b0cpuSrvHandle = cpuSrvUavHeapStart;
	m_b1cpuUavHandle = cpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuSrvUavHeapStart{ m_srvUavHeap->GetGPUDescriptorHandleForHeapStart() };
	m_b0gpuSrvHandle = gpuSrvUavHeapStart;
	m_b1gpuUavHandle = gpuSrvUavHeapStart.Offset(g_cbvSrvDescriptorIncrementSize);

	// 뷰 생성
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(m_buffer0.Get(), &srvDesc, m_b0cpuSrvHandle);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	device->CreateUnorderedAccessView(m_buffer1.Get(), nullptr, &uavDesc, m_b1cpuUavHandle);
}

FadeFilter::FadeFilter(const ComPtr<ID3D12Device>& device) : Filter{ device }
{

}

void FadeFilter::Excute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& postRootSignature, const ComPtr<ID3D12Resource>& renderTarget)
{
	/*
	1. 렌더타겟을 m_buffer0에 복사한다.
	2. 계산셰이더에서 m_buffer0을 이용해 m_buffer1에 페인드 인/아웃을 적용시킨다.
	3. m_buffer1을 렌더타겟에 복사한다.
	*/

	// 페이드 필터의 루트시그니쳐는 프레임워크 단에서 이미 설정되어있다.
	//commandList->SetComputeRootSignature(postRootSignature.Get());

	// 렌더타겟을 m_buffer에 복사한다.
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer0.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
	commandList->CopyResource(m_buffer0.Get(), renderTarget.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer0.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	// 서술자힙, 테이블 설정
	ID3D12DescriptorHeap* ppHeaps[]{ m_srvUavHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	commandList->SetComputeRootDescriptorTable(1, m_b0gpuSrvHandle);
	commandList->SetComputeRootDescriptorTable(2, m_b1gpuUavHandle);
	commandList->SetPipelineState(Scene::s_shaders["FADE"]->GetPipelineState().Get());

	// m_buffer1을 셰이더에서 쓸 수 있도록 상태 변경
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer1.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	// 실행
	commandList->Dispatch(static_cast<UINT>(ceilf(g_width / 32.0f)), static_cast<UINT>(ceilf(g_height / 32.0f)), 1);

	// 렌더타겟에 m_buffer1을 복사
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer1.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->CopyResource(renderTarget.Get(), m_buffer1.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer1.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// m_buffer0 리소스 베리어 원상태로 변경
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer0.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COMMON));
}

BlurFilter::BlurFilter(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList) : m_pConstantBuffer{ nullptr }
{
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

	// 셰이더 리소스 뷰 서술자힙 생성
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
	device->CreateShaderResourceView(m_buffer0.Get(), &srvDesc, m_b0cpuSrvHandle);
	device->CreateShaderResourceView(m_buffer1.Get(), &srvDesc, m_b1cpuSrvHandle);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	device->CreateUnorderedAccessView(m_buffer0.Get(), nullptr, &uavDesc, m_b0cpuUavHandle);
	device->CreateUnorderedAccessView(m_buffer1.Get(), nullptr, &uavDesc, m_b1cpuUavHandle);

	// 상수 버퍼 생성
	for (int i = 0; i < BLUR_COUNT; ++i)
	{
		m_constantBuffer[i] = Utile::CreateBufferResource(device, commandList, NULL, Utile::GetConstantBufferSize<ConstantBuffer>(), 1, D3D12_HEAP_TYPE_UPLOAD, {});
		m_constantBuffer[i]->Map(0, NULL, reinterpret_cast<void**>(&m_pConstantBuffer[i]));
		m_pConstantBuffer[i]->radius = static_cast<int>(g_height * (0.7f + 0.01f * i));
	}
}

	/*
	1. 렌더타겟을 m_buffer0에 복사한다.
	2. 계산셰이더에서 m_buffer0을 가로 블러링해서 m_buffer1에 저장한다.
	3. 계산셰이더에서 m_buffer1을 세로 블러링해서 m_buffer0에 저장한다.
	4. m_buffer0을 렌더타겟에 복사한다.
	*/
void BlurFilter::Excute(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& postRootSignature, const ComPtr<ID3D12Resource>& renderTarget)
{
	// 루트시그니쳐 설정
	commandList->SetComputeRootSignature(postRootSignature.Get());

	// 렌더타겟을 m_buffer0에 복사한다.
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer0.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
	commandList->CopyResource(m_buffer0.Get(), renderTarget.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer0.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	// m_buffer1를 셰이더에서 읽고 쓸 수 있도록 리소스 베리어 설정
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer1.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	// 서술자힙 설정
	ID3D12DescriptorHeap* ppHeaps[]{ m_srvUavHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// 블러링
	for (int i = 0; i < BLUR_COUNT; ++i)
	{
		commandList->SetComputeRootConstantBufferView(3, m_constantBuffer[i]->GetGPUVirtualAddress());

		// 가로 블러링
		commandList->SetComputeRootDescriptorTable(1, m_b0gpuSrvHandle);
		commandList->SetComputeRootDescriptorTable(2, m_b1gpuUavHandle);
		commandList->SetPipelineState(Scene::s_shaders["HORZ_BLUR"]->GetPipelineState().Get());
		commandList->Dispatch(static_cast<UINT>(ceilf(g_width / 256.0f)), g_height, 1);

		// m_buffer1을 쓰기로, m_buffer0을 읽기로 설정
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer0.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer1.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

		// 세로 블러링
		commandList->SetPipelineState(Scene::s_shaders["VERT_BLUR"]->GetPipelineState().Get());
		commandList->SetComputeRootDescriptorTable(1, m_b1gpuSrvHandle);
		commandList->SetComputeRootDescriptorTable(2, m_b0gpuUavHandle);
		commandList->Dispatch(g_width, static_cast<UINT>(ceilf(g_height / 256.0f)), 1);

		// m_buffer0을 쓰기로, m_buffer1을 읽기로 설정
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer0.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer1.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}

	// m_buffer0을 렌더타겟에 복사
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer0.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->CopyResource(renderTarget.Get(), m_buffer0.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer0.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTarget.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// m_buffer1 리소스 베리어 원상태로 변경
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_buffer1.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON));
}