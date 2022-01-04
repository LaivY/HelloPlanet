#pragma once
#include "stdafx.h"
#include "DDSTextureLoader12.h"

struct TextureInfo
{
	INT		frame;
	FLOAT	frameTimer;
	FLOAT	frameInterver;
	BOOL	isFrameRepeat;

	TextureInfo() : frame{ 0 }, frameTimer{ 0 }, frameInterver{ 1.0f / 60.0f }, isFrameRepeat{ TRUE }
	{

	}
};

class Texture
{
public:
	Texture() : m_textureInfo{ nullptr } { }
	~Texture() = default;

	void LoadTextureFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT rootParameterIndex, const wstring& fileName);
	void CreateSrvDescriptorHeap(const ComPtr<ID3D12Device>& device);
	void CreateShaderResourceView(const ComPtr<ID3D12Device>& device);
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void ReleaseUploadBuffer();

	void SetTextureInfo(TextureInfo* textureInfo) { m_textureInfo = textureInfo; }
	UINT GetTextureCount() const { return m_textures.size(); }

protected:
	ComPtr<ID3D12DescriptorHeap>				m_srvHeap;
	vector<pair<UINT, ComPtr<ID3D12Resource>>>	m_textures;
	vector<ComPtr<ID3D12Resource>>				m_textureUploadBuffers;
	TextureInfo*								m_textureInfo;
};