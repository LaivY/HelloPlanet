#pragma once
#include "stdafx.h"
#include "mesh.h"
#include "texture.h"

class Shader
{
public:
	Shader();
	Shader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~Shader() = default;

	ComPtr<ID3D12PipelineState> GetPipelineState() const { return m_pipelineState; }
	wstring PATH(const wstring& fileName) const;

protected:
	ComPtr<ID3D12PipelineState>			m_pipelineState;
	vector<D3D12_INPUT_ELEMENT_DESC>	m_inputLayout;
};

class AnimationShader : public Shader
{
public:
	AnimationShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~AnimationShader() = default;
};

class TextureShader : public Shader
{
public:
	TextureShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~TextureShader() = default;
};

class TerrainShader : public Shader
{
public:
	TerrainShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~TerrainShader() = default;
};

class TerrainTessShader : public Shader
{
public:
	TerrainTessShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~TerrainTessShader() = default;
};

class TerrainTessWireShader : public Shader
{
public:
	TerrainTessWireShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~TerrainTessWireShader() = default;
};

class SkyboxShader : public Shader
{
public:
	SkyboxShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~SkyboxShader() = default;
};

class BlendingShader : public Shader
{
public:
	BlendingShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~BlendingShader() = default;
};

class BlendingDepthShader : public Shader
{
public:
	BlendingDepthShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~BlendingDepthShader() = default;
};

class StencilShader : public Shader
{
public:
	StencilShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~StencilShader() = default;
};

class MirrorShader : public Shader
{
public:
	MirrorShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~MirrorShader() = default;
};

class MirrorTextureShader : public Shader
{
public:
	MirrorTextureShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~MirrorTextureShader() = default;
};

class ModelShader : public Shader
{
public:
	ModelShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~ModelShader() = default;
};

class ShadowShader : public Shader
{
public:
	ShadowShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~ShadowShader() = default;
};

class HorzBlurShader : public Shader
{
public:
	HorzBlurShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~HorzBlurShader() = default;
};

class VertBlurShader : public Shader
{
public:
	VertBlurShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	~VertBlurShader() = default;
};