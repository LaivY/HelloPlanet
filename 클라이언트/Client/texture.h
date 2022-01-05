﻿#pragma once
#include "stdafx.h"
#include "DDSTextureLoader12.h"

struct TextureInfo
{
	TextureInfo() : frame{ 0 }, frameTimer{ 0 }, frameInterver{ 1.0f / 60.0f }, doRepeat{ TRUE } { }

	INT		frame;
	FLOAT	frameTimer;
	FLOAT	frameInterver;
	BOOL	doRepeat;
};

class Texture
{
public:
	Texture() = default;
	~Texture() = default;

	void LoadTextureFile(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, UINT rootParameterIndex, const wstring& fileName);
	void CreateTexture(const ComPtr<ID3D12Device>& device);
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList, int index = 0);
	void ReleaseUploadBuffer();

	UINT GetTextureCount() const { return m_textures.size(); }

protected:
	ComPtr<ID3D12DescriptorHeap>				m_srvHeap;
	vector<pair<UINT, ComPtr<ID3D12Resource>>>	m_textures;
	vector<ComPtr<ID3D12Resource>>				m_textureUploadBuffers;
};