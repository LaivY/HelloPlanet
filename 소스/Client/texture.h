#pragma once

class Texture
{
public:
	Texture() = default;
	~Texture() = default;

	// 빈 텍스쳐 생성
	void CreateTexture(const ComPtr<ID3D12Device>& device, DXGI_FORMAT textureFotmat, DXGI_FORMAT shaderResourceViewFormat, UINT width, UINT height, UINT rootParameterIndex);
	void Copy(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12Resource>& src, D3D12_RESOURCE_STATES currSrcResourceState);

	// DDS로딩
	void LoadTextureFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT rootParameterIndex, const wstring& fileName);
	void CreateTextureFromLoadedFiles(const ComPtr<ID3D12Device>& device);

	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList, int index = 0);
	void ReleaseUploadBuffer();

	size_t GetTextureCount() const { return m_textures.size(); }

protected:
	ComPtr<ID3D12DescriptorHeap>				m_srvHeap;
	vector<pair<UINT, ComPtr<ID3D12Resource>>>	m_textures;
	vector<ComPtr<ID3D12Resource>>				m_textureUploadBuffers;
};