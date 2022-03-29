#include "mesh.h"
#include "object.h"
#define PLAYER_UPPER_JOINT_START 23

Mesh::Mesh() : m_nVertices{ 0 }, m_nIndices{ 0 }, m_vertexBufferView{}, m_indexBufferView{}, m_primitiveTopology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST }
{

}

void Mesh::LoadMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName)
{
	ifstream file{ fileName };
	string dumy{};

	// 변환 행렬
	file >> dumy;

	m_transformMatrix = make_unique<XMFLOAT4X4>();
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			file >> m_transformMatrix->m[i][j];

	int nVertices{ 0 };
	file >> dumy >> nVertices;
	vector<Vertex> vertices;
	vertices.reserve(nVertices);
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
	vector<Material> materials;
	materials.reserve(nMaterial);
	for (int i = 0; i < nMaterial; ++i)
	{
		Material m;
		file >> dumy >> dumy;
		file >> dumy >> m.color.x >> m.color.y >> m.color.z >> m.color.w;
		m_materials.push_back(move(m));
	}
	CreateVertexBuffer(device, commandList, vertices.data(), sizeof(Vertex), vertices.size());
}

void Mesh::LoadAnimation(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName, const string& animationName)
{
	ifstream file{ fileName };
	if (!file) return;

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
}

void Mesh::CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	//ComPtr<ID3D12Resource> dummy;
	//constexpr UINT cbMeshByteSize{ (sizeof(cbMesh) + 255) & ~255 };
	//m_cbMesh = CreateBufferResource(device, commandList, NULL, cbMeshByteSize, 1, D3D12_HEAP_TYPE_UPLOAD, {}, dummy);
	//m_cbMesh->Map(0, NULL, reinterpret_cast<void**>(&m_pcbMesh));
}

void Mesh::CreateVertexBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, void* data, UINT sizePerData, UINT dataCount)
{
	// 정점 버퍼 갯수 저장
	m_nVertices = dataCount;

	// 정점 버퍼 생성
	m_vertexBuffer = Utile::CreateBufferResource(device, commandList, data, sizePerData, dataCount, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_vertexUploadBuffer.Get());

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
	m_indexBuffer = Utile::CreateBufferResource(device, commandList, data, sizeof(UINT), dataCount, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, m_indexUploadBuffer.Get());

	// 인덱스 버퍼 뷰 설정
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = sizeof(UINT) * dataCount;
}

void Mesh::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList, GameObject* object)
{
	// 변환 행렬, 재질, 애니메이션 데이터가 없는 메쉬는 셰이더 변수를 최신화해줄 필요없다.
	if (!m_transformMatrix && m_materials.empty() && m_animations.empty())
		return;

	// 해당 게임오브젝트가 사용할 상수 버퍼가 없다면 생성
	if (!m_cbMesh[object])
	{
		UINT cbMeshByteSize{ 0 };
		if (m_animations.empty())
			cbMeshByteSize = Utile::GetConstantBufferSize<cbMesh2>();
		else
			cbMeshByteSize = Utile::GetConstantBufferSize<cbMesh>();
		m_cbMesh[object] = Utile::CreateBufferResource(g_device, commandList, NULL, cbMeshByteSize, 1, D3D12_HEAP_TYPE_UPLOAD, {});
	}

	// 상수 버퍼의 가상 메모리 주소를 받음
	cbMesh* pcbMesh{ nullptr };
	m_cbMesh[object]->Map(0, NULL, reinterpret_cast<void**>(&pcbMesh));

	// 변환 행렬
	if (m_transformMatrix)
	{
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				pcbMesh->transformMatrix.m[i][j] = m_transformMatrix->m[i][j];
	}

	// 재질
	for (int i = 0; i < m_materials.size(); ++i)
		pcbMesh->materials[i] = m_materials[i];

	// 애니메이션
	if (AnimationInfo* animationInfo{ object ? object->GetAnimationInfo() : nullptr })
	{
		// 이 블럭 안에서 선언할 수 있는 변수는 모두 선언한다.
		
		// 현재 애니메이션 구조체
		const Animation& currAnimation{ 
			m_linkMesh
			? m_linkMesh->GetAnimation(animationInfo->currAnimationName)
			: m_animations.at(animationInfo->currAnimationName)
		};

		// 현재 진행중인 애니메이션을 처리할 때 필요한 변수들
		constexpr float FPS{ 1.0f / 30.0f };																			// 애니메이션 1프레임 당 시간
		const float currFrame{ animationInfo->currTimer / FPS };														// 프레임(실수)
		const int nCurrFrame{ min(static_cast<int>(floorf(currFrame)), static_cast<int>(currAnimation.length - 1)) };	// 프레임(정수)
		const int nNextFrame{ min(static_cast<int>(ceilf(currFrame)), static_cast<int>(currAnimation.length - 1)) };	// 다음 프레임(정수)
		const float t{ currFrame - static_cast<int>(currFrame) };														// 프레임 진행 간 선형보간에 사용할 매개변수(실수 프레임의 소수부)

		// 블렌딩에 걸리는 시간
		constexpr float totalBlendingTime{ Setting::BLENDING_FRAMES * FPS };

		int start{ 0 }, end{ static_cast<int>(currAnimation.joints.size()) }; // 상하체 애니메이션 분리에 사용되는 인덱스 결정 변수

		// 상체 애니메이션
		if (AnimationInfo* upperAnimationInfo{ object->GetUpperAnimationInfo() })
		{
			// 이 블럭 안에서 상체 애니메이션과 관련된 선언할 수 있는 변수는 모두 선언한다.

			// 현재 상체 애니메이션 구조체
			const Animation& currUpperAnimation{
				m_linkMesh
				? m_linkMesh->GetAnimation(upperAnimationInfo->currAnimationName)
				: m_animations.at(upperAnimationInfo->currAnimationName)
			};

			// 현재 진행중인 상체 애니메이션을 처리할 때 필요한 변수들
			const float upperCurrFrame{ upperAnimationInfo->currTimer / FPS };
			const int nUpperCurrFrame{ min(static_cast<int>(floorf(upperCurrFrame)), static_cast<int>(currUpperAnimation.length - 1)) };
			const int nUpperNextFrame{ min(static_cast<int>(ceilf(upperCurrFrame)), static_cast<int>(currUpperAnimation.length - 1)) };
			const float upperT{ upperCurrFrame - static_cast<int>(upperCurrFrame) };

			// 상체 애니메이션이 있으면 기존 애니메이션은 하체만 애니메이션한다.
			// 상체 애니메이션은 0 ~ PLAYER_UPPER_JOINT_START 뼈만 애니메이션하는 것이다.
			// 총도 상체 애니메이션을 따라간다. (마지막 뼈는 총의 애니메이션 변환 행렬이다.)
			start = PLAYER_UPPER_JOINT_START; --end;

			if (upperAnimationInfo->state == eAnimationState::PLAY)
			{
				for (int i = 0; i < start; ++i)
				{
					pcbMesh->boneTransformMatrix[i] = Matrix::Interpolate(currUpperAnimation.joints[i].animationTransformMatrix[nUpperCurrFrame],
																		  currUpperAnimation.joints[i].animationTransformMatrix[nUpperNextFrame],
																		  upperT);
				}
				pcbMesh->boneTransformMatrix[currUpperAnimation.joints.size() - 1] = Matrix::Interpolate(currUpperAnimation.joints.back().animationTransformMatrix[nUpperCurrFrame],
																										 currUpperAnimation.joints.back().animationTransformMatrix[nUpperNextFrame],
																										 upperT);
				object->OnAnimation(upperCurrFrame, currUpperAnimation.length, TRUE);
			}
			else if (upperAnimationInfo->state == eAnimationState::BLENDING)
			{
				// 상체 애니메이션은 블렌딩할 때 하체 애니메이션의 타이밍과 맞춘다.
				const Animation& upperAfterAni{
					m_linkMesh
					? m_linkMesh->GetAnimation(upperAnimationInfo->afterAnimationName)
					: m_animations.at(upperAnimationInfo->afterAnimationName)
				};

				// 블렌딩 매개변수
				const float t2{ clamp(upperAnimationInfo->blendingTimer / totalBlendingTime, 0.0f, 1.0f) };

				for (int i = 0; i < start; ++i)
				{
					XMFLOAT4X4 before{ Matrix::Interpolate(currAnimation.joints[i].animationTransformMatrix[nUpperCurrFrame],
														   currAnimation.joints[i].animationTransformMatrix[nUpperNextFrame],
														   upperT) };
					XMFLOAT4X4 after{ upperAfterAni.joints[i].animationTransformMatrix.front() };
					pcbMesh->boneTransformMatrix[i] = Matrix::Interpolate(before, after, t2);
				}

				// 총 애니메이션
				XMFLOAT4X4 before{ Matrix::Interpolate(currAnimation.joints.back().animationTransformMatrix[nUpperCurrFrame],
													   currAnimation.joints.back().animationTransformMatrix[nUpperNextFrame],
													   upperT) };
				XMFLOAT4X4 after{ upperAfterAni.joints.back().animationTransformMatrix.front() };
				pcbMesh->boneTransformMatrix[currUpperAnimation.joints.size() - 1] = Matrix::Interpolate(before, after, t2);

				object->OnAnimation(upperAnimationInfo->blendingTimer / FPS, Setting::BLENDING_FRAMES, TRUE);
			}
			else if (upperAnimationInfo->state == eAnimationState::SYNC)
			{
				// 블렌딩 매개변수
				const float t2{ clamp(upperAnimationInfo->blendingTimer / totalBlendingTime, 0.0f, 1.0f) };

				for (int i = 0; i < start; ++i)
				{
					XMFLOAT4X4 before{ Matrix::Interpolate(currUpperAnimation.joints[i].animationTransformMatrix[nUpperCurrFrame],
														   currUpperAnimation.joints[i].animationTransformMatrix[nUpperNextFrame],
														   upperT) };
					XMFLOAT4X4 after{ Matrix::Interpolate(currAnimation.joints[i].animationTransformMatrix[nCurrFrame],
														  currAnimation.joints[i].animationTransformMatrix[nNextFrame],
														  t) };
					pcbMesh->boneTransformMatrix[i] = Matrix::Interpolate(before, after, t2);
				}

				// 총 애니메이션
				XMFLOAT4X4 before{ Matrix::Interpolate(currUpperAnimation.joints.back().animationTransformMatrix[nUpperCurrFrame],
													   currUpperAnimation.joints.back().animationTransformMatrix[nUpperNextFrame],
													   upperT) };
				XMFLOAT4X4 after{ Matrix::Interpolate(currAnimation.joints.back().animationTransformMatrix[nCurrFrame],
													  currAnimation.joints.back().animationTransformMatrix[nNextFrame],
													  t) };
				pcbMesh->boneTransformMatrix[currUpperAnimation.joints.size() - 1] = Matrix::Interpolate(before, after, t2);

				object->OnAnimation(upperAnimationInfo->blendingTimer / FPS, Setting::BLENDING_FRAMES, TRUE);
			}
		}

		if (animationInfo->state == eAnimationState::PLAY) // 프레임 진행
		{
			for (int i = start; i < end; ++i)
			{
				pcbMesh->boneTransformMatrix[i] = Matrix::Interpolate(currAnimation.joints[i].animationTransformMatrix[nCurrFrame],
																	  currAnimation.joints[i].animationTransformMatrix[nNextFrame],
																	  t);
			}
			object->OnAnimation(currFrame, currAnimation.length);
		}
		else if (animationInfo->state == eAnimationState::BLENDING) // 애니메이션 블렌딩
		{
			// curr -> after로 애니메이션 블렌딩 진행
			const Animation& afterAnimation{
				m_linkMesh
				? m_linkMesh->GetAnimation(animationInfo->afterAnimationName)
				: m_animations.at(animationInfo->afterAnimationName)
			};

			// 블렌딩 매개변수
			const float t2{ clamp(animationInfo->blendingTimer / totalBlendingTime, 0.0f, 1.0f) };

			for (int i = start; i < end; ++i)
			{
				const XMFLOAT4X4& before{ Matrix::Interpolate(currAnimation.joints[i].animationTransformMatrix[nCurrFrame],
															  currAnimation.joints[i].animationTransformMatrix[nNextFrame],
															  t) };
				const XMFLOAT4X4& after{ afterAnimation.joints[i].animationTransformMatrix.front() };
				pcbMesh->boneTransformMatrix[i] = Matrix::Interpolate(before, after, t2);
			}
			object->OnAnimation(animationInfo->blendingTimer / FPS, Setting::BLENDING_FRAMES);
		}
	}

	m_cbMesh[object]->Unmap(0, NULL);
	commandList->SetGraphicsRootConstantBufferView(1, m_cbMesh[object]->GetGPUVirtualAddress());
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

void Mesh::ReleaseUploadBuffer()
{
	if (m_vertexUploadBuffer) m_vertexUploadBuffer.Reset();
	if (m_indexUploadBuffer) m_indexUploadBuffer.Reset();
}

RectMesh::RectMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, FLOAT width, FLOAT height, FLOAT length, const XMFLOAT3& position)
{
	vector<Vertex> vertices;
	Vertex v; v.materialIndex = 0;
	FLOAT hx{ position.x + width / 2.0f }, hy{ position.y + height / 2.0f }, hz{ position.z + length / 2.0f };
	if (width == 0.0f) // YZ평면
	{
		if (position.x > 0.0f)
		{
			v.position = { +hx, +hy, +hz }; v.uv = { 0.0f, 0.0f }; vertices.push_back(v);
			v.position = { +hx, +hy, -hz }; v.uv = { 1.0f, 0.0f }; vertices.push_back(v);
			v.position = { +hx, -hy, -hz }; v.uv = { 1.0f, 1.0f }; vertices.push_back(v);

			v.position = { +hx, +hy, +hz }; v.uv = { 0.0f, 0.0f }; vertices.push_back(v);
			v.position = { +hx, -hy, -hz }; v.uv = { 1.0f, 1.0f }; vertices.push_back(v);
			v.position = { +hx, -hy, +hz }; v.uv = { 0.0f, 1.0f }; vertices.push_back(v);
		}
		else
		{
			v.position = { +hx, +hy, -hz }; v.uv = { 0.0f, 0.0f }; vertices.push_back(v);
			v.position = { +hx, +hy, +hz }; v.uv = { 1.0f, 0.0f }; vertices.push_back(v);
			v.position = { +hx, -hy, +hz }; v.uv = { 1.0f, 1.0f }; vertices.push_back(v);

			v.position = { +hx, +hy, -hz }; v.uv = { 0.0f, 0.0f }; vertices.push_back(v);
			v.position = { +hx, -hy, +hz }; v.uv = { 1.0f, 1.0f }; vertices.push_back(v);
			v.position = { +hx, -hy, -hz }; v.uv = { 0.0f, 1.0f }; vertices.push_back(v);
		}
	}
	else if (length == 0.0f) // XY평면
	{
		if (position.z > 0.0f)
		{
			v.position = { -hx, +hy, +hz }; v.uv = { 0.0f, 0.0f }; vertices.push_back(v);
			v.position = { +hx, +hy, +hz }; v.uv = { 1.0f, 0.0f }; vertices.push_back(v);
			v.position = { +hx, -hy, +hz }; v.uv = { 1.0f, 1.0f }; vertices.push_back(v);

			v.position = { -hx, +hy, +hz }; v.uv = { 0.0f, 0.0f }; vertices.push_back(v);
			v.position = { +hx, -hy, +hz }; v.uv = { 1.0f, 1.0f }; vertices.push_back(v);
			v.position = { -hx, -hy, +hz }; v.uv = { 0.0f, 1.0f }; vertices.push_back(v);
		}
		else
		{
			v.position = { +hx, +hy, +hz }; v.uv = { 0.0f, 0.0f }; vertices.push_back(v);
			v.position = { -hx, +hy, +hz }; v.uv = { 1.0f, 0.0f }; vertices.push_back(v);
			v.position = { -hx, -hy, +hz }; v.uv = { 1.0f, 1.0f }; vertices.push_back(v);

			v.position = { +hx, +hy, +hz }; v.uv = { 0.0f, 0.0f }; vertices.push_back(v);
			v.position = { -hx, -hy, +hz }; v.uv = { 1.0f, 1.0f }; vertices.push_back(v);
			v.position = { +hx, -hy, +hz }; v.uv = { 0.0f, 1.0f }; vertices.push_back(v);
		}
	}
	else if (height == 0.0f) // XZ평면
	{
		if (position.y > 0.0f)
		{
			v.position = { -hx, +hy, -hz }; v.uv = { 0.0f, 0.0f }; vertices.push_back(v);
			v.position = { +hx, +hy, -hz }; v.uv = { 1.0f, 0.0f }; vertices.push_back(v);
			v.position = { +hx, +hy, +hz }; v.uv = { 1.0f, 1.0f }; vertices.push_back(v);

			v.position = { -hx, +hy, -hz }; v.uv = { 0.0f, 0.0f }; vertices.push_back(v);
			v.position = { +hx, +hy, +hz }; v.uv = { 1.0f, 1.0f }; vertices.push_back(v);
			v.position = { -hx, +hy, +hz }; v.uv = { 0.0f, 1.0f }; vertices.push_back(v);
		}
		else
		{
			v.position = { +hx, +hy, -hz }; v.uv = { 1.0f, 1.0f }; vertices.push_back(v);
			v.position = { -hx, +hy, -hz }; v.uv = { 0.0f, 1.0f }; vertices.push_back(v);
			v.position = { -hx, +hy, +hz }; v.uv = { 0.0f, 0.0f }; vertices.push_back(v);

			v.position = { +hx, +hy, -hz }; v.uv = { 1.0f, 1.0f }; vertices.push_back(v);
			v.position = { -hx, +hy, +hz }; v.uv = { 0.0f, 0.0f }; vertices.push_back(v);
			v.position = { +hx, +hy, +hz }; v.uv = { 1.0f, 0.0f }; vertices.push_back(v);
		}
	}

	// 흰색 재질
	Material material;
	material.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	m_materials.push_back(move(material));

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

CubeMesh::CubeMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, FLOAT width, FLOAT height, FLOAT length, const XMFLOAT3& position, const XMFLOAT4& color)
{
	// 큐브 가로, 세로, 높이
	FLOAT sx{ width / 2.0f }, sy{ height }, sz{ length / 2.0f };

	vector<Vertex> vertices;
	vertices.reserve(36);

	Vertex v;
	v.materialIndex = 0;

	// 앞면
	v.position = { position.x - sx, position.y + sy, position.z - sz }; vertices.push_back(v);
	v.position = { position.x + sx, position.y + sy, position.z - sz }; vertices.push_back(v);
	v.position = { position.x + sx, position.y,		 position.z - sz };	vertices.push_back(v);

	v.position = { position.x - sx, position.y + sy, position.z - sz }; vertices.push_back(v);
	v.position = { position.x + sx, position.y,		 position.z - sz };	vertices.push_back(v);
	v.position = { position.x - sx, position.y,		 position.z - sz };	vertices.push_back(v);

	// 오른쪽면
	v.position = { position.x + sx, position.y + sy, position.z - sz }; vertices.push_back(v);
	v.position = { position.x + sx, position.y + sy, position.z + sz }; vertices.push_back(v);
	v.position = { position.x + sx, position.y,		 position.z + sz }; vertices.push_back(v);

	v.position = { position.x + sx, position.y + sy, position.z - sz }; vertices.push_back(v);
	v.position = { position.x + sx, position.y,		 position.z + sz }; vertices.push_back(v);
	v.position = { position.x + sx, position.y,		 position.z - sz }; vertices.push_back(v);

	// 왼쪽면
	v.position = { position.x - sx, position.y + sy, position.z + sz }; vertices.push_back(v);
	v.position = { position.x - sx, position.y + sy, position.z - sz }; vertices.push_back(v);
	v.position = { position.x - sx, position.y,		 position.z - sz }; vertices.push_back(v);

	v.position = { position.x - sx, position.y + sy, position.z + sz }; vertices.push_back(v);
	v.position = { position.x - sx, position.y,		 position.z - sz }; vertices.push_back(v);
	v.position = { position.x - sx, position.y,		 position.z + sz }; vertices.push_back(v);

	// 뒷면
	v.position = { position.x + sx, position.y + sy, position.z + sz }; vertices.push_back(v);
	v.position = { position.x - sx, position.y + sy, position.z + sz }; vertices.push_back(v);
	v.position = { position.x - sx, position.y,		 position.z + sz }; vertices.push_back(v);

	v.position = { position.x + sx, position.y + sy, position.z + sz }; vertices.push_back(v);
	v.position = { position.x - sx, position.y,		 position.z + sz }; vertices.push_back(v);
	v.position = { position.x + sx, position.y,		 position.z + sz }; vertices.push_back(v);

	// 윗면
	v.position = { position.x - sx, position.y + sy, position.z + sz }; vertices.push_back(v);
	v.position = { position.x + sx, position.y + sy, position.z + sz }; vertices.push_back(v);
	v.position = { position.x + sx, position.y + sy, position.z - sz }; vertices.push_back(v);

	v.position = { position.x - sx, position.y + sy, position.z + sz }; vertices.push_back(v);
	v.position = { position.x + sx, position.y + sy, position.z - sz }; vertices.push_back(v);
	v.position = { position.x - sx, position.y + sy, position.z - sz }; vertices.push_back(v);

	// 밑면
	v.position = { position.x + sx, position.y, position.z + sz }; vertices.push_back(v);
	v.position = { position.x - sx, position.y, position.z + sz }; vertices.push_back(v);
	v.position = { position.x - sx, position.y, position.z - sz }; vertices.push_back(v);

	v.position = { position.x + sx, position.y, position.z + sz }; vertices.push_back(v);
	v.position = { position.x - sx, position.y, position.z - sz }; vertices.push_back(v);
	v.position = { position.x + sx, position.y, position.z - sz }; vertices.push_back(v);

	// 회색 재질
	Material material;
	material.color = color;
	m_materials.push_back(move(material));

	CreateVertexBuffer(device, commandList, vertices.data(), sizeof(Vertex), vertices.size());

	UINT cbMeshByteSize{ 0 };
	cbMeshByteSize = Utile::GetConstantBufferSize<cbMesh2>();
	m_cbMesh[nullptr] = Utile::CreateBufferResource(g_device, commandList, NULL, cbMeshByteSize, 1, D3D12_HEAP_TYPE_UPLOAD, {});
}