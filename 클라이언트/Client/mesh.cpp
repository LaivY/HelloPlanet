#include "mesh.h"

Mesh::Mesh() : m_nVertices{ 0 }, m_nIndices{ 0 }, m_vertexBufferView{}, m_indexBufferView{}, m_primitiveTopology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST }, m_cbMeshData{ nullptr }
{

}

Mesh::~Mesh()
{
	if (m_cbMesh) m_cbMesh->Unmap(0, NULL);
}

void Mesh::LoadMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName)
{
	ifstream file{ fileName };

	string dumy{};
	int nVertices{ 0 };
	file >> dumy >> nVertices;
	
	vector<Vertex> vertices(nVertices);
	for (int i = 0; i < nVertices; ++i)
	{
		Vertex v;
		file >> dumy >> v.position.x >> v.position.y >> v.position.z;
		file >> dumy >> v.normal.x >> v.normal.y >> v.normal.z;
		file >> dumy >> v.uv.x >> v.uv.y;
		file >> dumy >> v.boneIndex.x >> v.boneIndex.y >> v.boneIndex.z >> v.boneIndex.w;
		file >> dumy >> v.boneWeight.x >> v.boneWeight.y >> v.boneWeight.z >> v.boneWeight.w;
		vertices.push_back(v);
	}

	CreateVertexBuffer(device, commandList, vertices.data(), sizeof(Vertex), vertices.size());
}

void Mesh::LoadAnimation(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName, const string& animationName)
{
	ifstream file{ fileName };

	string dumy{};
	int nJoints{}, nFrame{};
	file >> dumy >> nJoints >> dumy >> nFrame;

	vector<Joint> joints(nJoints);
	for (Joint& j : joints)
	{
		file >> dumy;
		for (int i = 0; i < nFrame; ++i)
		{
			XMFLOAT4X4 matrix{};
			for (int y = 0; y < 4; ++y)
				for (int x = 0; x < 4; ++x)
					file >> matrix.m[y][x];
			j.animationTransformMatrix.push_back(matrix);
		}
	}

	Animation animation{};
	animation.joints = move(joints);
	m_animations[animationName] = animation;

	// 애니메이션을 처음 불러온거라면 상수 버퍼를 만듦
	if (!m_cbMeshData) CreateShaderVariable(device, commandList);
}

void Mesh::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	ComPtr<ID3D12Resource> dummy;
	UINT cbSceneByteSize{ (sizeof(cbMesh) + 255) & ~255 };
	m_cbMesh = CreateBufferResource(device, commandList, NULL, cbSceneByteSize, 1, D3D12_HEAP_TYPE_UPLOAD, {}, dummy);
	m_cbMesh->Map(0, NULL, reinterpret_cast<void**>(&m_pcbMesh));
	m_cbMeshData = make_unique<cbMesh>();
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

void Mesh::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList, FLOAT frame) const
{
	if (!m_cbMeshData) return;

	// 아직 프레임을 보간하지 않음
	UINT f{ static_cast<UINT>(floorf(frame)) };

	const Animation& ani{ m_animations.at("RELOAD") };
	for (int i = 0; i < ani.joints.size(); ++i)
	{
		XMFLOAT4X4 m{ ani.joints[i].animationTransformMatrix[f] };
		m_cbMeshData->boneTransformMatrix[i] = Matrix::Transpose(m);
	}
	memcpy(m_pcbMesh, m_cbMeshData.get(), sizeof(cbMesh));
	commandList->SetGraphicsRootConstantBufferView(1, m_cbMesh->GetGPUVirtualAddress());
}

void Mesh::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	// 애니메이션 테스트용
	static float frame{ 0.0f };
	frame += 0.01f;
	if (frame > 80.0f) frame = 0.0f;
	UpdateShaderVariable(commandList, frame);

	commandList->IASetPrimitiveTopology(m_primitiveTopology);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	if (m_nIndices)
	{
		commandList->IASetIndexBuffer(&m_indexBufferView);
		commandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
	}
	else commandList->DrawInstanced(m_nVertices, 1, 0, 0);
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