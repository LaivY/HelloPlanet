#pragma once

enum class eTextureType
{
	DEFAULT, SHADOW, DEPTH, STENCIL
};

class Texture
{
public:
	Texture();
	~Texture() = default;

	// 이 함수가 호출되면 현재 s_resources에 있는 리소스로 셰이더 리소스 뷰를 만듦
	static void CreateShaderResourceView(const ComPtr<ID3D12Device>& device);
	static void ReleaseUploadBuffer();

	void Create(const ComPtr<ID3D12Device>& device, DXGI_FORMAT textureFotmat, UINT width, UINT height, UINT rootParameterIndex, eTextureType type = eTextureType::DEFAULT);
	void Load(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT rootParameterIndex, const wstring& fileName);
	void Copy(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& src, D3D12_RESOURCE_STATES currSrcResourceState);

	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT index = 0);

	ComPtr<ID3D12Resource> GetBuffer(UINT index = 0) const;
	size_t GetCount() const;

	static ComPtr<ID3D12DescriptorHeap>	s_srvHeap;

protected:
	struct Resource
	{
		ComPtr<ID3D12Resource>	buffer;
		UINT					rootParameterIndex;
		eTextureType			type;
	};

	static vector<Resource>					s_resources;
	static vector<ComPtr<ID3D12Resource>>	s_uploadBuffers;

	size_t m_index; // s_resources에서 자신의 인덱스
	size_t m_count; // 자신의 텍스쳐 개수
};

class ShadowTexture : public Texture
{
public:
	ShadowTexture(const ComPtr<ID3D12Device>& device, UINT width, UINT height, INT count = Setting::SHADOWMAP_COUNT);
	~ShadowTexture() = default;

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsvCpuHandle() const;
	D3D12_VIEWPORT GetViewport() const;
	D3D12_RECT GetScissorRect() const;

private:
	ComPtr<ID3D12DescriptorHeap>	m_dsvHeap;
	CD3DX12_CPU_DESCRIPTOR_HANDLE	m_dsvCpuHandle;
	D3D12_VIEWPORT					m_viewport;
	D3D12_RECT						m_scissorRect;
};