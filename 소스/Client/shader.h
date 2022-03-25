#pragma once
#include "stdafx.h"
#include "mesh.h"
#include "texture.h"

class Shader
{
public:
	Shader();
	Shader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps);
	virtual ~Shader() = default;

	ComPtr<ID3D12PipelineState> GetPipelineState() const { return m_pipelineState; }

protected:
	ComPtr<ID3D12PipelineState>			m_pipelineState;
	vector<D3D12_INPUT_ELEMENT_DESC>	m_inputLayout;
};

class ModelShader : public Shader
{
public:
	ModelShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps);
	~ModelShader() = default;
};

class AnimationShader : public Shader
{
public:
	AnimationShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps);
	~AnimationShader() = default;
};

class LinkShader : public Shader
{
public:
	LinkShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps);
	~LinkShader() = default;
};

class SkyboxShader : public Shader
{
public:
	SkyboxShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps);
	~SkyboxShader() = default;
};

class BoundingBoxShader : public Shader
{
public:
	BoundingBoxShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps);
	~BoundingBoxShader() = default;
};