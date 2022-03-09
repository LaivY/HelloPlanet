﻿#pragma once
#include "stdafx.h"

class GameObject;

struct Vertex
{
	Vertex() : position{}, normal{}, color{}, uv{}, materialIndex{ -1 }, boneIndex{}, boneWeight{} { }

	XMFLOAT3	position;
	XMFLOAT3	normal;
	XMFLOAT4	color;
	XMFLOAT2	uv;
	INT			materialIndex;
	XMUINT4		boneIndex;
	XMFLOAT4	boneWeight;
};

struct Material
{
	Material() : color{} { }

	XMFLOAT4 color;
};

struct Joint
{
	Joint() : name{} { }

	string				name;
	vector<XMFLOAT4X4>	animationTransformMatrix;
};

struct Animation
{
	Animation() : length{}, interver{ 1.0f / 24.0f } { }

	UINT			length;
	FLOAT			interver;
	vector<Joint>	joints;
};

struct cbMesh
{
	XMFLOAT4X4						transformMatrix;
	array<Material, MAX_MATERIAL>	materials;
	array<XMFLOAT4X4, MAX_JOINT>	boneTransformMatrix;
};

struct cbMesh2
{
	XMFLOAT4X4						transformMatrix;
	array<Material, MAX_MATERIAL>	materials;
};

class Mesh
{
public:
	Mesh();
	virtual ~Mesh();

	void LoadMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName);
	void LoadAnimation(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName, const string& animationName);
	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateVertexBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, void* data, UINT sizePerData, UINT dataCount);
	void CreateIndexBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, void* data, UINT dataCount);
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList, GameObject* object);
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void ReleaseUploadBuffer();

	void Link(const shared_ptr<Mesh>& mesh) { m_linkMesh = mesh; }

	const Animation& GetAnimation(const string& animationName) const { return m_animations.at(animationName); }

protected:
	UINT												m_nVertices;
	ComPtr<ID3D12Resource>								m_vertexBuffer;
	ComPtr<ID3D12Resource>								m_vertexUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW							m_vertexBufferView;

	UINT												m_nIndices;
	ComPtr<ID3D12Resource>								m_indexBuffer;
	ComPtr<ID3D12Resource>								m_indexUploadBuffer;
	D3D12_INDEX_BUFFER_VIEW								m_indexBufferView;

	D3D_PRIMITIVE_TOPOLOGY								m_primitiveTopology;

	shared_ptr<Mesh>									m_linkMesh;			// 부모 메쉬

	XMFLOAT4X4											m_transformMatrix;	// 메쉬 변환 행렬
	vector<Material>									m_materials;		// 재질
	unordered_map<string, Animation>					m_animations;		// 애니메이션
	unordered_map<GameObject*, ComPtr<ID3D12Resource>>	m_cbMesh;			// 상수버퍼들
};

class RectMesh : public Mesh
{
public:
	RectMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, FLOAT width, FLOAT height, FLOAT length, XMFLOAT3 position = { 0.0f, 0.0f, 0.0f });
	virtual ~RectMesh() = default;
};

class BillboardMesh : public Mesh
{
public:
	BillboardMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const XMFLOAT3& position, FLOAT width, FLOAT height);
	virtual ~BillboardMesh() = default;
};