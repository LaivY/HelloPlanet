#pragma once
#include "stdafx.h"
#include "mesh.h"
#include "texture.h"

class Shader
{
public:
	Shader();
	Shader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~Shader() = default;

	ComPtr<ID3D12PipelineState> GetPipelineState() const { return m_pipelineState; }
	wstring PATH(const wstring& fileName) const;

protected:
	ComPtr<ID3D12PipelineState>			m_pipelineState;
	vector<D3D12_INPUT_ELEMENT_DESC>	m_inputLayout;
};

class ModelShader : public Shader
{
public:
	ModelShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~ModelShader() = default;
};

class AnimationShader : public Shader
{
public:
	AnimationShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~AnimationShader() = default;
};

class LinkShader : public Shader
{
public:
	LinkShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~LinkShader() = default;
};

class TextureShader : public Shader
{
public:
	TextureShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~TextureShader() = default;
};

class TerrainShader : public Shader
{
public:
	TerrainShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~TerrainShader() = default;
};

class TerrainTessShader : public Shader
{
public:
	TerrainTessShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~TerrainTessShader() = default;
};

class TerrainTessWireShader : public Shader
{
public:
	TerrainTessWireShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~TerrainTessWireShader() = default;
};

class SkyboxShader : public Shader
{
public:
	SkyboxShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~SkyboxShader() = default;
};

class BlendingShader : public Shader
{
public:
	BlendingShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~BlendingShader() = default;
};

class BlendingDepthShader : public Shader
{
public:
	BlendingDepthShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~BlendingDepthShader() = default;
};

class StencilShader : public Shader
{
public:
	StencilShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~StencilShader() = default;
};

class MirrorShader : public Shader
{
public:
	MirrorShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~MirrorShader() = default;
};

class MirrorTextureShader : public Shader
{
public:
	MirrorTextureShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~MirrorTextureShader() = default;
};

class ShadowShader : public Shader
{
public:
	ShadowShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~ShadowShader() = default;
};

class HorzBlurShader : public Shader
{
public:
	HorzBlurShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~HorzBlurShader() = default;
};

class VertBlurShader : public Shader
{
public:
	VertBlurShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature);
	virtual ~VertBlurShader() = default;
};