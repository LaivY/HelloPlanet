﻿#pragma once
#include "stdafx.h"

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

class NoDepthShader : public Shader
{
public:
	NoDepthShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps);
	~NoDepthShader() = default;
};

class BlendingShader : public Shader
{
public:
	BlendingShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps, bool alphaToCoverage = false);
	~BlendingShader() = default;
};

class ShadowShader : public Shader
{
public:
	ShadowShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& gs);
	~ShadowShader() = default;
};

class WireframeShader : public Shader
{
public:
	WireframeShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps);
	~WireframeShader() = default;
};

class StencilShader : public Shader
{
public:
	StencilShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps);
	~StencilShader() = default;
};

class OutlineShader : public Shader
{
public:
	OutlineShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps);
	~OutlineShader() = default;
};

class OutlineShader2 : public Shader
{
public:
	OutlineShader2(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps);
	~OutlineShader2() = default;
};