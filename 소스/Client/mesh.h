#pragma once

class GameObject;

struct Vertex
{
	Vertex() : position{}, normal{}, uv{}, materialIndex{ -1 }, boneIndex{}, boneWeight{} { }

	XMFLOAT3	position;
	XMFLOAT3	normal;
	XMFLOAT2	uv;
	INT			materialIndex;
	XMUINT4		boneIndex;
	XMFLOAT4	boneWeight;
};

struct Material
{
	Material() : baseColor{}, reflection{}, roughness{} { }

	XMFLOAT4	baseColor;
	XMFLOAT3	reflection;
	FLOAT		roughness;
};

struct Joint
{
	vector<XMFLOAT4X4> animationTransformMatrix;
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
	XMFLOAT4X4								transformMatrix;
	array<Material, Setting::MAX_MATERIALS>	materials;
	array<XMFLOAT4X4, Setting::MAX_JOINTS>	boneTransformMatrix;
};

struct cbMesh2
{
	XMFLOAT4X4								transformMatrix;
	array<Material, Setting::MAX_MATERIALS>	materials;
};

class Mesh
{
public:
	Mesh();
	virtual ~Mesh() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList);

	void LoadMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName);
	void LoadMeshBinary(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName);
	void LoadAnimation(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName, const string& animationName);
	void LoadAnimationBinary(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& fileName, const string& animationName);
	void CreateVertexBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, void* data, UINT sizePerData, UINT dataCount);
	void CreateIndexBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, void* data, UINT dataCount);
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList, GameObject* object);
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

	unique_ptr<XMFLOAT4X4>								m_transformMatrix;	// 메쉬 변환 행렬
	vector<Material>									m_materials;		// 재질
	unordered_map<string, Animation>					m_animations;		// 애니메이션
	unordered_map<GameObject*, ComPtr<ID3D12Resource>>	m_cbMesh;			// 상수버퍼들
};

class RectMesh : public Mesh
{
public:
	RectMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, FLOAT width, FLOAT height, FLOAT length, const XMFLOAT3& position, const XMFLOAT4& color = { 0.0f, 0.0f, 0.0f, 0.0f });
	~RectMesh() = default;
};

class BillboardMesh : public Mesh
{
public:
	BillboardMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, FLOAT width, FLOAT height, const XMFLOAT3& position = { 0.0f, 0.0f, 0.0f });
	~BillboardMesh() = default;
};

class CubeMesh : public Mesh
{
public:
	CubeMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
			 FLOAT width, FLOAT height, FLOAT length, const XMFLOAT3& position = { 0.0f, 0.0f, 0.0f }, const XMFLOAT4 & color = { 1.0f, 1.0f, 1.0f, 1.0f });
	~CubeMesh() = default;
};

class FullScreenQuadMesh : public Mesh
{
public:
	FullScreenQuadMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const XMFLOAT4& color = { 1.0f, 1.0f, 1.0f, 1.0f });
	~FullScreenQuadMesh() = default;
};

class ParticleMesh abstract : public Mesh
{
public:
	ParticleMesh();
	virtual ~ParticleMesh() = default;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void RenderStreamOutput(const ComPtr<ID3D12GraphicsCommandList>& commandList);

	void CreateStreamOutputBuffer(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);

protected:
	BOOL							m_isFirstRender;					// 첫번째 렌더링인지 여부

	UINT							m_vertexSize;						// 정점 구조체 크기
	UINT							m_maxVertexCount;					// 최대 정점 개수

	ComPtr<ID3D12Resource>          m_streamOutputBuffer;               // 스트림 출력 버퍼
	D3D12_STREAM_OUTPUT_BUFFER_VIEW m_streamOutputBufferView;           // 스트림 출력 버퍼 뷰

	UINT*                           m_pFilledSize;						// 스트림 버퍼에 쓰여진 데이터 크기
	ComPtr<ID3D12Resource>			m_streamFilledSizeBuffer;			// 스트림 버퍼에 쓰여진 데이터 크기를 받을 버퍼
	ComPtr<ID3D12Resource>			m_streamFilledSizeUploadBuffer;		// 위의 버퍼에 복사할 때 쓰일 업로드 버퍼
	ComPtr<ID3D12Resource>			m_streamFilledSizeReadBackBuffer;	// 쓰여진 데이터 크기를 읽어올 때 쓰일 리드백 버퍼
	ComPtr<ID3D12Resource>			m_drawBuffer;						// 스트림 출력된 결과를 복사해서 출력할 때 쓰일 버퍼
};

class DustParticleMesh : public ParticleMesh
{
public:
	struct DustParticleVertex
	{
		XMFLOAT3	position;
		XMFLOAT3	direction;
		FLOAT		speed;
		FLOAT		lifeTime;
		FLOAT		age;
	};

public:
	DustParticleMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	~DustParticleMesh() = default;
};

class TrailParticleMesh : public ParticleMesh
{
public:
	struct TrailParticleVertex
	{
		XMFLOAT3	position;
		XMFLOAT3	direction;
		FLOAT		lifeTime;
		INT			type;
	};

public:
	TrailParticleMesh(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const XMFLOAT3& position, const XMFLOAT3& direction);
	~TrailParticleMesh() = default;
};