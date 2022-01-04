#include "stdafx.h"

UINT g_cbvSrvDescriptorIncrementSize{ 0 };

ComPtr<ID3D12Resource> CreateBufferResource(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const void* data, UINT sizePerData, UINT dataCount,
	D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES resourceState, ComPtr<ID3D12Resource>& uploadBuffer)
{
	ComPtr<ID3D12Resource> buffer;
	const UINT bufferSize{ sizePerData * dataCount };

	// 디폴트 버퍼에 데이터를 넣을 경우 업로드 버퍼가 필요함
	if (heapType == D3D12_HEAP_TYPE_DEFAULT)
	{
		// 디폴트 버퍼 생성
		DX::ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			NULL,
			IID_PPV_ARGS(&buffer)));

		// 데이터 복사
		if (data)
		{
			// 업로드 버퍼 생성
			DX::ThrowIfFailed(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				NULL,
				IID_PPV_ARGS(&uploadBuffer)));

			// 업로드 버퍼에서 디폴트 버퍼로 복사
			D3D12_SUBRESOURCE_DATA bufferData{};
			bufferData.pData = data;
			bufferData.RowPitch = bufferSize;
			bufferData.SlicePitch = bufferData.RowPitch;
			UpdateSubresources<1>(commandList.Get(), buffer.Get(), uploadBuffer.Get(), 0, 0, 1, &bufferData);
		}

		// 버퍼 리소스 베리어 설정
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, resourceState));
		return buffer;
	}

	// 업로드 버퍼에 데이터를 넣을 경우 바로 복사함
	if (heapType == D3D12_HEAP_TYPE_UPLOAD)
	{
		// 업로드 버퍼 생성
		DX::ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			NULL,
			IID_PPV_ARGS(&buffer)));

		// 데이터 복사
		if (data)
		{
			UINT8* pBufferDataBegin{ NULL };
			CD3DX12_RANGE readRange{ 0, 0 };
			DX::ThrowIfFailed(buffer->Map(0, &readRange, reinterpret_cast<void**>(&pBufferDataBegin)));
			memcpy(pBufferDataBegin, data, bufferSize);
			buffer->Unmap(0, NULL);
		}

		return buffer;
	}

	if (heapType == D3D12_HEAP_TYPE_READBACK)
	{
		// 리드백 버퍼 생성
		DX::ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			NULL,
			IID_PPV_ARGS(&buffer)));

		// 데이터 복사
		if (data)
		{
			UINT8* pBufferDataBegin{ NULL };
			CD3DX12_RANGE readRange{ 0, 0 };
			DX::ThrowIfFailed(buffer->Map(0, &readRange, reinterpret_cast<void**>(&pBufferDataBegin)));
			memcpy(pBufferDataBegin, data, bufferSize);
			buffer->Unmap(0, NULL);
		}

		return buffer;
	}

	return NULL;
}

string sPATH(const string& filePath)
{
	return "../Resource/" + filePath;
}

wstring wPATH(const string& filePath)
{
	wstring wStr{ filePath.begin(), filePath.end() };
	return TEXT("../Resource/") + wStr;
}