#include "mesh.h"
#include "object.h"

Mesh::Mesh() : m_nVertices{ 0 }, m_nIndices{ 0 }, m_vertexBufferView{}, m_indexBufferView{}, m_primitiveTopology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST }
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

	// 변환 행렬
	file >> dumy;
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			file >> m_transformMatrix.m[i][j];
	m_transformMatrix = m_transformMatrix;

	int nVertices{ 0 };
	file >> dumy >> nVertices;
	vector<Vertex> vertices(nVertices);
	for (int i = 0; i < nVertices; ++i)
	{
		Vertex v;
		file >> dumy >> v.position.x >> v.position.y >> v.position.z;
		file >> dumy >> v.normal.x >> v.normal.y >> v.normal.z;
		file >> dumy >> v.uv.x >> v.uv.y;
		file >> dumy >> v.materialIndex;
		file >> dumy >> v.boneIndex.x >> v.boneIndex.y >> v.boneIndex.z >> v.boneIndex.w;
		file >> dumy >> v.boneWeight.x >> v.boneWeight.y >> v.boneWeight.z >> v.boneWeight.w;
		vertices.push_back(move(v));
	}

	int nMaterial{ 0 };
	file >> dumy >> nMaterial;
	vector<Material> materials(nMaterial);
	for (int i = 0; i < nMaterial; ++i)
	{
		Material m;
		file >> dumy >> dumy;
		file >> dumy >> m.color.x >> m.color.y >> m.color.z >> m.color.w;
		m_materials.push_back(move(m));
	}

	CreateVertexBuffer(device, commandList, vertices.data(), sizeof(Vertex), vertices.size());

	// 상수 버퍼가 없다면 생성
	if (!m_cbMesh) CreateShaderVariable(device, commandList);
}

void Mesh::LoadAnimation(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName, const string& animationName)
{
	ifstream file{ fileName };

	string dumy{};
	int nJoints{}, nFrame{};
	file >> dumy >> nJoints >> dumy >> nFrame;

	Animation animation{};
	animation.length = nFrame;
	animation.joints.reserve(nJoints);
	for (int i = 0; i < nJoints; ++i)
	{
		Joint joint;
		joint.animationTransformMatrix.reserve(nFrame);

		file >> joint.name;
		for (int i = 0; i < nFrame; ++i)
		{
			XMFLOAT4X4 matrix{};
			for (int y = 0; y < 4; ++y)
				for (int x = 0; x < 4; ++x)
					file >> matrix.m[y][x];
			joint.animationTransformMatrix.push_back(move(matrix));
		}
		animation.joints.push_back(move(joint));
	}
	m_animations[animationName] = move(animation);

	// 상수 버퍼가 없다면 생성
	if (!m_cbMesh) CreateShaderVariable(device, commandList);
}

void Mesh::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	ComPtr<ID3D12Resource> dummy;
	UINT cbSceneByteSize{ (sizeof(cbMesh) + 255) & ~255 };
	m_cbMesh = CreateBufferResource(device, commandList, NULL, cbSceneByteSize, 1, D3D12_HEAP_TYPE_UPLOAD, {}, dummy);
	m_cbMesh->Map(0, NULL, reinterpret_cast<void**>(&m_pcbMesh));
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

void Mesh::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList, GameObject* object) const
{
	if (!m_cbMesh) return;

	// 변환 행렬
	m_pcbMesh->transformMatrix = m_transformMatrix;

	// 재질
	for (int i = 0; i < m_materials.size(); ++i)
		m_pcbMesh->materials[i] = m_materials[i];

	// 애니메이션
	if (AnimationInfo* aniInfo{ object->GetAnimationInfo() }; aniInfo)
	{
		if (aniInfo->beforeAnimationName.empty()) // 프레임 진행
		{
			const Animation& ani{ m_animations.at(aniInfo->animationName) };
			float frame{ aniInfo->timer / (1.0f / 24.0f) };
			UINT currFrame{ min(static_cast<UINT>(floorf(frame)), m_animations.at(aniInfo->animationName).length - 1) };
			UINT nextFrame{ min(static_cast<UINT>(ceilf(frame)), m_animations.at(aniInfo->animationName).length - 1) };
			float t{ frame - static_cast<int>(frame) };

			for (int i = 0; i < ani.joints.size(); ++i)
			{
				XMFLOAT4X4 m{ Matrix::Interpolate(ani.joints[i].animationTransformMatrix[currFrame],
												  ani.joints[i].animationTransformMatrix[nextFrame],
												  t) };
				m_pcbMesh->boneTransformMatrix[i] = Matrix::Transpose(m);
			}
			object->OnAnimation(aniInfo->animationName, frame, m_animations.at(aniInfo->animationName).length);
		}
		else // 애니메이션 블렌딩
		{
			// before -> after로 애니메이션 블렌딩 진행
			const Animation& beforeAni{ m_animations.at(aniInfo->beforeAnimationName) };
			const Animation& afterAni{ m_animations.at(aniInfo->animationName) };

			float frame{ aniInfo->timer / (1.0f / 24.0f) };
			UINT currFrame{ min(static_cast<UINT>(floorf(frame)), beforeAni.length - 1) };
			UINT nextFrame{ min(static_cast<UINT>(ceilf(frame)), beforeAni.length - 1) };
			float t1{ frame - static_cast<int>(frame) };

			// 애니메이션 블렌딩은 n프레임에 걸쳐되도록 설정
			UINT iFrame{ 10 };
			float blendingFrame{ iFrame * (1.0f / 24.0f) };
			float t2{ clamp(aniInfo->blendingTimer / blendingFrame, 0.0f, 1.0f) };

			for (int i = 0; i < beforeAni.joints.size(); ++i)
			{
				XMFLOAT4X4 before{ Matrix::Interpolate(beforeAni.joints[i].animationTransformMatrix.at(currFrame),
													   beforeAni.joints[i].animationTransformMatrix.at(nextFrame),
													   t1) };
				XMFLOAT4X4 after{ afterAni.joints[i].animationTransformMatrix[0] };
				XMFLOAT4X4 m{ Matrix::Interpolate(before, after, t2) };
				m_pcbMesh->boneTransformMatrix[i] = Matrix::Transpose(m);
			}
			object->OnAnimation("BLENDING", aniInfo->blendingTimer / (1.0f / 24.0f), iFrame);
		}
	}

	commandList->SetGraphicsRootConstantBufferView(1, m_cbMesh->GetGPUVirtualAddress());
}

void Mesh::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, GameObject* object) const
{
	// 애니메이션
	UpdateShaderVariable(commandList, object);

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

RectMesh::RectMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, FLOAT width, FLOAT height)
{
	vector<Vertex> vertices(6);
	float hx{ width / 2.0f }, hy{ height / 2.0f };

	Vertex v;
	v.position = { -hx, +hy, 0.0f }; v.uv = { 0.0f, 0.0f }; vertices.push_back(v);
	v.position = { +hx, +hy, 0.0f }; v.uv = { 1.0f, 0.0f }; vertices.push_back(v);
	v.position = { +hx, -hy, 0.0f }; v.uv = { 1.0f, 1.0f }; vertices.push_back(v);

	v.position = { -hx, +hy, 0.0f }; v.uv = { 0.0f, 0.0f }; vertices.push_back(v);
	v.position = { +hx, -hy, 0.0f }; v.uv = { 1.0f, 1.0f }; vertices.push_back(v);
	v.position = { -hx, -hy, 0.0f }; v.uv = { 0.0f, 1.0f }; vertices.push_back(v);

	CreateVertexBuffer(device, commandList, vertices.data(), sizeof(Vertex), vertices.size());
}

BillboardMesh::BillboardMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const XMFLOAT3& position, FLOAT width, FLOAT height)
{
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

	Vertex v;
	v.position = position;
	v.uv = XMFLOAT2{ width, height };
	CreateVertexBuffer(device, commandList, &v, sizeof(Vertex), 1);
}