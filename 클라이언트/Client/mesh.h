#pragma once
#include "stdafx.h"

struct Vertex
{
	XMFLOAT3 position{};
	XMFLOAT3 normal{};
	XMFLOAT4 color{};
	XMFLOAT2 uv0{};
	XMFLOAT2 uv1{};
};

struct ParticleVertex
{
	XMFLOAT3 position{};
	XMFLOAT2 size{};
	FLOAT speed{};
};

class Mesh
{
public:
	Mesh();
	Mesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
		void* vertexData, UINT sizePerVertexData, UINT vertexDataCount, void* indexData, UINT indexDataCount, D3D_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	Mesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName, D3D_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	~Mesh() = default;

	void CreateVertexBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, void* data, UINT sizePerData, UINT dataCount);
	void CreateIndexBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, void* data, UINT dataCount);

	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const D3D12_VERTEX_BUFFER_VIEW& instanceBufferView, UINT count) const;

	void ReleaseUploadBuffer();

protected:
	UINT						m_nVertices;
	ComPtr<ID3D12Resource>		m_vertexBuffer;
	ComPtr<ID3D12Resource>		m_vertexUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW	m_vertexBufferView;

	UINT						m_nIndices;
	ComPtr<ID3D12Resource>		m_indexBuffer;
	ComPtr<ID3D12Resource>		m_indexUploadBuffer;
	D3D12_INDEX_BUFFER_VIEW		m_indexBufferView;

	D3D_PRIMITIVE_TOPOLOGY		m_primitiveTopology;
};

class CubeMesh : public Mesh
{
public:
	CubeMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, FLOAT width, FLOAT length, FLOAT height);
	~CubeMesh() = default;
};

class ReverseCubeMesh : public Mesh
{
public:
	ReverseCubeMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, FLOAT width, FLOAT length, FLOAT height);
	~ReverseCubeMesh() = default;
};

class TextureRectMesh : public Mesh
{
public:
	TextureRectMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, FLOAT width, FLOAT length, FLOAT height, XMFLOAT3 position);
	~TextureRectMesh() = default;
};

class BillboardMesh : public Mesh
{
public:
	BillboardMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const XMFLOAT3& position, const XMFLOAT2& size);
	~BillboardMesh() = default;
};

class ParticleMesh : public Mesh
{
public:
	ParticleMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const XMFLOAT2& size);
	~ParticleMesh() = default;

	void CreateStreamOutputBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);

	void RenderStreamOutput(const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList);

private:
	ComPtr<ID3D12Resource>			m_streamOutputBuffer;				// 스트림 출력 버퍼
	D3D12_STREAM_OUTPUT_BUFFER_VIEW m_streamOutputBufferView;			// 스트림 출력 버퍼 뷰
	
	ComPtr<ID3D12Resource>			m_streamFilledSizeBuffer;			// 스트림 버퍼에 쓰여진 데이터 크기를 받을 버퍼
	ComPtr<ID3D12Resource>			m_streamFilledSizeUploadBuffer;		// 위의 버퍼에 복사할 때 쓰일 업로드 버퍼
	UINT*							m_pFilledSize;						// 스트림 버퍼에 쓰여진 데이터 크기

	ComPtr<ID3D12Resource>			m_streamFilledSizeReadBackBuffer;	// 쓰여진 데이터 크기를 읽어올 때 쓰일 리드백 버퍼

	ComPtr<ID3D12Resource>			m_drawBuffer;						// 스트림 출력된 결과를 복사해서 출력할 때 쓰일 버퍼

	BOOL							m_isFirst;							// 처음 시작할때는 정점 버퍼와 바인딩, 그 이후엔 렌더링 결과와 바인딩
};