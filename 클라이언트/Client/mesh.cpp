#include "mesh.h"

Mesh::Mesh() : m_nVertices{ 0 }, m_nIndices{ 0 }, m_vertexBufferView{}, m_indexBufferView{},  m_primitiveTopology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST }
{

}

void Mesh::LoadMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName)
{
	ifstream file{ fileName };
	
	int nVertices{ 0 };
	file >> nVertices;
	
	vector<Vertex> vertices(nVertices);
	for (int i = 0; i < nVertices; ++i)
	{
		Vertex v;
		file >> v.position.x >> v.position.y >> v.position.z;
		file >> v.normal.x >> v.normal.y >> v.normal.z;
		file >> v.uv.x >> v.uv.y;
		file >> v.boneIndex.x >> v.boneIndex.y >> v.boneIndex.z >> v.boneIndex.w;
		file >> v.boneWeight.x >> v.boneWeight.y >> v.boneWeight.z >> v.boneWeight.w;
		vertices.push_back(v);
	}

	CreateVertexBuffer(device, commandList, vertices.data(), sizeof(Vertex), vertices.size());
}

void Mesh::LoadAnimation(const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName, const string& animationName)
{
	ifstream file{ fileName };

	int nJoints{ 0 };
	file >> nJoints;

	vector<Joint> joints(nJoints);
	for (int i = 0; i < nJoints; ++i)
	{
		Joint joint;
		int index{ 0 };
		int nameLength{ 0 };
		file >> index;
		file >> nameLength;
		file >> joint.name;
		file >> joint.parentIndex;

		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				file >> joint.idontknow.m[i][j];
	}

	int nFrame{ 0 };
	file >> nFrame;

	for (Joint& j : joints)
	{
		j.transformMatrix.reserve(nFrame);
		for (int i = 0; i < nFrame; ++i)
		{
			XMFLOAT4X4 matrix{};
			for (int y = 0; y < 4; ++y)
				for (int x = 0; x < 4; ++x)
					file >> matrix.m[y][x];
			j.transformMatrix.push_back(matrix);
		}
	}
}

void Mesh::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	commandList->IASetPrimitiveTopology(m_primitiveTopology);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	if (m_nIndices)
	{
		commandList->IASetIndexBuffer(&m_indexBufferView);
		commandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
	}
	else commandList->DrawInstanced(m_nVertices, 1, 0, 0);
}

void Mesh::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const D3D12_VERTEX_BUFFER_VIEW& instanceBufferView, UINT count) const
{
	commandList->IASetPrimitiveTopology(m_primitiveTopology);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->IASetVertexBuffers(1, 1, &instanceBufferView);
	if (m_nIndices)
	{
		commandList->IASetIndexBuffer(&m_indexBufferView);
		commandList->DrawIndexedInstanced(m_nIndices, count, 0, 0, 0);
	}
	else commandList->DrawInstanced(m_nVertices, count, 0, 0);
}

void Mesh::CreateVertexBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, void* data, UINT sizePerData, UINT dataCount)
{
	// 정점 버퍼 갯수 저장
	m_nVertices = dataCount;

	// 정점 버퍼 생성
	m_vertexBuffer = CreateBufferResource(device, commandList, data, sizePerData, dataCount, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_vertexUploadBuffer);

	// 정점 버퍼 뷰 설정
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = sizePerData * dataCount;
	m_vertexBufferView.StrideInBytes = sizePerData;
}

void Mesh::CreateIndexBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, void* data, UINT dataCount)
{
	// 인덱스 버퍼 갯수 저장
	m_nIndices = dataCount;

	// 인덱스 버퍼 생성
	m_indexBuffer = CreateBufferResource(device, commandList, data, sizeof(UINT), dataCount, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_indexUploadBuffer);

	// 인덱스 버퍼 뷰 설정
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = sizeof(UINT) * dataCount;
}

void Mesh::ReleaseUploadBuffer()
{
	if (m_vertexUploadBuffer) m_vertexUploadBuffer.Reset();
	if (m_indexUploadBuffer) m_indexUploadBuffer.Reset();
}

BillboardMesh::BillboardMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const XMFLOAT3& position, const XMFLOAT2& size)
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

	Vertex v;
	v.position = position;
	v.uv = size;
	CreateVertexBuffer(device, commandList, &v, sizeof(Vertex), 1);
}

ParticleMesh::ParticleMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const XMFLOAT2& size) : m_isFirst{ TRUE }
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

	vector<ParticleVertex> vertices;
	for (int i = 0; i < 81; ++i)
	{
		ParticleVertex v;
		v.position = XMFLOAT3{ i % 9 * 5.0f - 20.0f, 0.0f, i / 9 * 5.0f - 20.0f };
		v.size = size;
		v.speed = static_cast<float>(100 + rand() % 200);
		vertices.push_back(v);
	}

	CreateVertexBuffer(device, commandList, vertices.data(), sizeof(ParticleVertex), vertices.size());
	CreateStreamOutputBuffer(device, commandList);
}

void ParticleMesh::CreateStreamOutputBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	ComPtr<ID3D12Resource> dummy;

	// 최대 파티클 분열 개수
	// 나의 경우 파티클이 파티클을 만들지 않음
	const UINT nMaxParticles{ 81 };

	// 스트림출력 버퍼 생성
	m_streamOutputBuffer = CreateBufferResource(device, commandList, NULL, sizeof(ParticleVertex) * nMaxParticles, 1, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_STREAM_OUT, dummy);

	// 스트림출력 버퍼에 얼만큼 쓸건지를 저장하는 버퍼 생성
	m_streamFilledSizeBuffer = CreateBufferResource(device, commandList, NULL, sizeof(UINT64), 1, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_STREAM_OUT, dummy);

	// 스트림출력 버퍼에 데이터를 복사하기 위한 업로드 버퍼 생성
	m_streamFilledSizeUploadBuffer = CreateBufferResource(device, commandList, NULL, sizeof(UINT64), 1, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, dummy);
	m_streamFilledSizeUploadBuffer->Map(0, NULL, reinterpret_cast<void**>(&m_pFilledSize));

	// 스트림출력 버퍼 뷰 생성
	m_streamOutputBufferView.BufferLocation = m_streamOutputBuffer->GetGPUVirtualAddress();
	m_streamOutputBufferView.SizeInBytes = sizeof(ParticleVertex) * nMaxParticles;
	m_streamOutputBufferView.BufferFilledSizeLocation = m_streamFilledSizeBuffer->GetGPUVirtualAddress();

	// 스트림출력 버퍼에 쓰여진 데이터 크기를 CPU에서 읽기 위한 리드백 버퍼 생성
	m_streamFilledSizeReadBackBuffer = CreateBufferResource(device, commandList, NULL, sizeof(UINT64), 1, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST, dummy);

	// 통상적인 렌더링에 사용되는 버퍼 생성
	m_drawBuffer = CreateBufferResource(device, commandList, NULL, sizeof(ParticleVertex) * nMaxParticles, 1, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, dummy);
}

void ParticleMesh::RenderStreamOutput(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 맨 처음 한번만 정점 버퍼와 바인딩한다.
	// 그 이후로는 렌더링 출력 결과와 바인딩한다.
	if (m_isFirst)
	{
		m_isFirst = FALSE;
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(ParticleVertex);
		m_vertexBufferView.SizeInBytes = sizeof(ParticleVertex) * m_nVertices;
	}

	// 스트림 출력 패스 시작
	// m_pFilledSize를 m_streamFilledSizeBuffer에 복사
	*m_pFilledSize = 0;
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_streamFilledSizeBuffer.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->CopyResource(m_streamFilledSizeBuffer.Get(), m_streamFilledSizeUploadBuffer.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_streamFilledSizeBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT));

	// 스트림 출력
	D3D12_STREAM_OUTPUT_BUFFER_VIEW streamOutputBufferViews[]{ m_streamOutputBufferView };
	commandList->SOSetTargets(0, _countof(streamOutputBufferViews), streamOutputBufferViews);
	Mesh::Render(commandList);

	// 리드백 버퍼로 스트림 출력으로 얼마만큼 데이터를 썼는지 받아옴
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_streamFilledSizeBuffer.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE));
	commandList->CopyResource(m_streamFilledSizeReadBackBuffer.Get(), m_streamFilledSizeBuffer.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_streamFilledSizeBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT));

	// 리드백 버퍼에 받아온 데이터 크기로 정점 개수 재설정
	UINT64* pFilledSize = NULL;
	m_streamFilledSizeReadBackBuffer->Map(0, NULL, reinterpret_cast<void**>(&pFilledSize));
	m_nVertices = UINT(*pFilledSize) / sizeof(ParticleVertex);
	m_streamFilledSizeReadBackBuffer->Unmap(0, NULL);

	// 스트림 출력 결과를 m_drawBuffer로 복사
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_drawBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_streamOutputBuffer.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE));
	commandList->CopyResource(m_drawBuffer.Get(), m_streamOutputBuffer.Get());
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_drawBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_streamOutputBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT));
}

void ParticleMesh::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 렌더링 패스 시작
	m_vertexBufferView.BufferLocation = m_drawBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(ParticleVertex);
	m_vertexBufferView.SizeInBytes = sizeof(ParticleVertex) * m_nVertices;

	// 통상적인 렌더링
	commandList->SOSetTargets(0, 1, NULL);
	Mesh::Render(commandList);
}