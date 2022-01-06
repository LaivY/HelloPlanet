#pragma once
#include "stdafx.h"
#define MAX_JOINT 96

struct Vertex
{
	Vertex() : position{}, normal{}, color{}, uv{}, boneIndex{}, boneWeight{} { }

	XMFLOAT3	position;
	XMFLOAT3	normal;
	XMFLOAT4	color;
	XMFLOAT2	uv;
	XMUINT4		boneIndex;
	XMFLOAT4	boneWeight;
};

struct Joint
{
	Joint() : name{}, parentIndex{ -1 }, idontknow{} { }

	string name;
	INT parentIndex;
	XMFLOAT4X4 idontknow;
	vector<XMFLOAT4X4> transformMatrix;
};

struct Animation
{
	vector<Joint> joints;
};

struct cbMesh
{
	array<XMFLOAT4X4, MAX_JOINT> boneTransformMatrix;
};

class Mesh
{
public:
	Mesh();
	~Mesh();

	void LoadMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName);
	void LoadAnimation(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName, const string& animationName);
	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateVertexBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, void* data, UINT sizePerData, UINT dataCount);
	void CreateIndexBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, void* data, UINT dataCount);
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList, FLOAT frame) const;
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList);
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

	unordered_map<string, Animation>	m_animations;
	ComPtr<ID3D12Resource>				m_cbMesh;
	cbMesh*								m_pcbMesh;
	unique_ptr<cbMesh>					m_cbMeshData;
};

class BillboardMesh : public Mesh
{
public:
	BillboardMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const XMFLOAT3& position, const XMFLOAT2& size);
	~BillboardMesh() = default;
};