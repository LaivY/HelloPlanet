#pragma once

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

class StencilWriteShader : public Shader
{
public:
	StencilWriteShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps);
	~StencilWriteShader() = default;
};

class ComputeShader : public Shader
{
public:
	ComputeShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& postRootSignature, const wstring& shaderFile, const string& cs);
	~ComputeShader() = default;
};

class WireframeShader : public Shader
{
public:
	WireframeShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps);
	~WireframeShader() = default;
};

class ParticleShader abstract : public Shader
{
public:
	virtual ~ParticleShader() = default;

	ComPtr<ID3D12PipelineState> GetStreamPipelineState() const;

protected:
	ComPtr<ID3D12PipelineState> m_streamPipelineState;
};

class DustParticleShader : public ParticleShader
{
public:
	DustParticleShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& streamGs, const string& gs, const string& ps);
	~DustParticleShader() = default;
};

class TrailParticleShader : public ParticleShader
{
public:
	TrailParticleShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& streamGs, const string& gs, const string& ps);
	~TrailParticleShader() = default;
};