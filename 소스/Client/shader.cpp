#include "stdafx.h"
#include "shader.h"
#include "mesh.h"

Shader::Shader()
{
	m_inputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "MATERIAL", 0, DXGI_FORMAT_R32_SINT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDEX", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 52, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

Shader::Shader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps) : Shader{}
{
	ComPtr<ID3DBlob> vertexShader, pixelShader, error;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, vs.c_str(), "vs_5_1", compileFlags, 0, &vertexShader, &error));
	(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, ps.c_str(), "ps_5_1", compileFlags, 0, &pixelShader, &error));
	if (error)
	{
		OutputDebugStringA(reinterpret_cast<char*>(error->GetBufferPointer()));
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

NoDepthShader::NoDepthShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps)
{
	ComPtr<ID3DBlob> vertexShader, pixelShader, error;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, vs.c_str(), "vs_5_1", compileFlags, 0, &vertexShader, &error));
	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, ps.c_str(), "ps_5_1", compileFlags, 0, &pixelShader, &error));

	CD3DX12_DEPTH_STENCIL_DESC depthStencilState{ D3D12_DEFAULT };
	depthStencilState.DepthEnable = FALSE;
	depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = depthStencilState;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

BlendingShader::BlendingShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps, bool alphaToCoverage)
{
	ComPtr<ID3DBlob> vertexShader, pixelShader, error;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, vs.c_str(), "vs_5_1", compileFlags, 0, &vertexShader, &error));
	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, ps.c_str(), "ps_5_1", compileFlags, 0, &pixelShader, &error));

	// 깊이 쓰기 OFF
	CD3DX12_DEPTH_STENCIL_DESC depthStencilState{ D3D12_DEFAULT };
	depthStencilState.DepthEnable = FALSE;
	depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;

	// 블렌딩 설정
	CD3DX12_BLEND_DESC blendState{ D3D12_DEFAULT };
	blendState.AlphaToCoverageEnable = alphaToCoverage;
	blendState.RenderTarget[0].BlendEnable = TRUE;
	blendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

	// PSO 생성
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = depthStencilState;
	psoDesc.BlendState = blendState;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

ShadowShader::ShadowShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& gs)
{
	ComPtr<ID3DBlob> vertexShader, geometryShader, error;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, vs.c_str(), "vs_5_1", compileFlags, 0, &vertexShader, &error));
	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, gs.c_str(), "gs_5_1", compileFlags, 0, &geometryShader, &error));

	// 깊이값 바이어스 설정
	CD3DX12_RASTERIZER_DESC rasterizerState{ D3D12_DEFAULT };
	rasterizerState.DepthBias = 1'000;
	rasterizerState.DepthBiasClamp = 0.0f;
	rasterizerState.SlopeScaledDepthBias = 1.0f;

	// 깊이값만 쓸 것이므로 렌더타겟을 설정하지 않는다.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.GS = CD3DX12_SHADER_BYTECODE(geometryShader.Get());
	psoDesc.RasterizerState = rasterizerState;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 0;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

StencilWriteShader::StencilWriteShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps)
{
	ComPtr<ID3DBlob> vertexShader, pixelShader, error;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, vs.c_str(), "vs_5_1", compileFlags, 0, &vertexShader, &error));
	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, ps.c_str(), "ps_5_1", compileFlags, 0, &pixelShader, &error));

	// 깊이 쓰기 OFF
	CD3DX12_DEPTH_STENCIL_DESC depthStencilState{ D3D12_DEFAULT };
	depthStencilState.StencilEnable = TRUE;
	depthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = depthStencilState;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

ComputeShader::ComputeShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& postRootSignature, const wstring& shaderFile, const string& cs)
{
	ComPtr<ID3DBlob> computeShader, error;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	auto hr = D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, cs.c_str(), "cs_5_1", compileFlags, 0, &computeShader, &error);
	if (error)
		OutputDebugStringA(reinterpret_cast<char*>(error->GetBufferPointer()));
	DX::ThrowIfFailed(hr);

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = postRootSignature.Get();
	psoDesc.CS = CD3DX12_SHADER_BYTECODE{ computeShader.Get() };
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

WireframeShader::WireframeShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& ps)
{
	ComPtr<ID3DBlob> vertexShader, pixelShader, error;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, vs.c_str(), "vs_5_1", compileFlags, 0, &vertexShader, &error));
	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, ps.c_str(), "ps_5_1", compileFlags, 0, &pixelShader, &error));

	CD3DX12_RASTERIZER_DESC rasterizerDesc{ D3D12_DEFAULT };
	rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), static_cast<UINT>(m_inputLayout.size()) };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.RasterizerState = rasterizerDesc;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

ComPtr<ID3D12PipelineState> ParticleShader::GetStreamPipelineState() const
{
	return m_streamPipelineState;
}

DustParticleShader::DustParticleShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& streamGs, const string& gs, const string& ps)
{
	ComPtr<ID3DBlob> vertexShader, streamGeometryShader, geometryShader, pixelShader;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, vs.c_str(), "vs_5_1", compileFlags, 0, &vertexShader, NULL));
	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, streamGs.c_str(), "gs_5_1", compileFlags, 0, &streamGeometryShader, NULL));
	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, gs.c_str(), "gs_5_1", compileFlags, 0, &geometryShader, NULL));
	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, ps.c_str(), "ps_5_1", compileFlags, 0, &pixelShader, NULL));

	// 입력 레이아웃 설정
	m_inputLayout.clear();
	m_inputLayout =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "DIRECTION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "SPEED",		0, DXGI_FORMAT_R32_FLOAT,		0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "LIFETIME",	0, DXGI_FORMAT_R32_FLOAT,		0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "AGE",		0, DXGI_FORMAT_R32_FLOAT,		0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// 스트림 출력 설정
	D3D12_SO_DECLARATION_ENTRY* SODeclaration{ new D3D12_SO_DECLARATION_ENTRY[5] };
	SODeclaration[0] = { 0, "POSITION",  0, 0, 3, 0 };
	SODeclaration[1] = { 0, "DIRECTION", 0, 0, 3, 0 };
	SODeclaration[2] = { 0, "SPEED",	 0, 0, 1, 0 };
	SODeclaration[3] = { 0, "LIFETIME",  0, 0, 1, 0 };
	SODeclaration[4] = { 0, "AGE",		 0, 0, 1, 0 };

	UINT* bufferStrides{ new UINT[1] };
	bufferStrides[0] = sizeof(DustParticleMesh::DustParticleVertex);

	D3D12_STREAM_OUTPUT_DESC streamOutput{};
	streamOutput.pSODeclaration = SODeclaration;
	streamOutput.NumEntries = 5;
	streamOutput.pBufferStrides = bufferStrides;
	streamOutput.NumStrides = 1;
	streamOutput.RasterizedStream = D3D12_SO_NO_RASTERIZED_STREAM;

	// 깊이 쓰기 OFF
	CD3DX12_DEPTH_STENCIL_DESC depthStencilState{ D3D12_DEFAULT };
	depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	// 알파 블렌딩
	CD3DX12_BLEND_DESC blendState{ D3D12_DEFAULT };
	blendState.RenderTarget[0].BlendEnable = TRUE;
	blendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

	// 스트림 출력용 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC streamPsoDesc{};
	streamPsoDesc.InputLayout = { m_inputLayout.data(), static_cast<UINT>(m_inputLayout.size()) };
	streamPsoDesc.pRootSignature = rootSignature.Get();
	streamPsoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	streamPsoDesc.GS = CD3DX12_SHADER_BYTECODE(streamGeometryShader.Get());
	streamPsoDesc.StreamOutput = streamOutput;
	streamPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	streamPsoDesc.DepthStencilState = depthStencilState;
	streamPsoDesc.BlendState = blendState;
	streamPsoDesc.SampleMask = UINT_MAX;
	streamPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	streamPsoDesc.NumRenderTargets = 0;
	streamPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	streamPsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	streamPsoDesc.SampleDesc.Count = 1;
	DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&streamPsoDesc, IID_PPV_ARGS(&m_streamPipelineState)));

	// 통상 렌더용 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), (UINT)m_inputLayout.size() };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
	psoDesc.GS = CD3DX12_SHADER_BYTECODE(geometryShader.Get());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = depthStencilState;
	psoDesc.BlendState = blendState;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

TrailParticleShader::TrailParticleShader(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12RootSignature>& rootSignature, const wstring& shaderFile, const string& vs, const string& streamGs, const string& gs, const string& ps)
{
	ComPtr<ID3DBlob> vertexShader, streamGeometryShader, geometryShader, pixelShader, error;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlags = 0;
#endif

	//DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, vs.c_str(), "vs_5_1", compileFlags, 0, &vertexShader, NULL));
	auto hr = D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, vs.c_str(), "vs_5_1", compileFlags, 0, &vertexShader, &error);
	if (FAILED(hr))
	{
		char* debugString{ reinterpret_cast<char*>(error->GetBufferPointer()) };
		OutputDebugStringA(debugString + '\n');
	}

	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, streamGs.c_str(), "gs_5_1", compileFlags, 0, &streamGeometryShader, NULL));
	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, gs.c_str(), "gs_5_1", compileFlags, 0, &geometryShader, NULL));
	DX::ThrowIfFailed(D3DCompileFromFile(shaderFile.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, ps.c_str(), "ps_5_1", compileFlags, 0, &pixelShader, NULL));

	// 입력 레이아웃 설정
	m_inputLayout.clear();
	m_inputLayout =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "DIRECTION",	0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "LIFETIME",	0, DXGI_FORMAT_R32_FLOAT,		0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TYPE",		0, DXGI_FORMAT_R32_SINT,		0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// 스트림 출력 설정
	D3D12_SO_DECLARATION_ENTRY* SODeclaration{ new D3D12_SO_DECLARATION_ENTRY[4] };
	SODeclaration[0] = { 0, "POSITION",  0, 0, 3, 0 };
	SODeclaration[1] = { 0, "DIRECTION", 0, 0, 3, 0 };
	SODeclaration[2] = { 0, "LIFETIME",  0, 0, 1, 0 };
	SODeclaration[3] = { 0, "TYPE",		 0, 0, 1, 0 };

	UINT* bufferStrides{ new UINT[1] };
	bufferStrides[0] = sizeof(TrailParticleMesh::TrailParticleVertex);

	D3D12_STREAM_OUTPUT_DESC streamOutput{};
	streamOutput.pSODeclaration = SODeclaration;
	streamOutput.NumEntries = 4;
	streamOutput.pBufferStrides = bufferStrides;
	streamOutput.NumStrides = 1;
	streamOutput.RasterizedStream = D3D12_SO_NO_RASTERIZED_STREAM;

	// 깊이 쓰기 OFF
	CD3DX12_DEPTH_STENCIL_DESC depthStencilState{ D3D12_DEFAULT };
	depthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	// 알파 블렌딩
	CD3DX12_BLEND_DESC blendState{ D3D12_DEFAULT };
	blendState.RenderTarget[0].BlendEnable = TRUE;
	blendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

	// 스트림 출력용 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC streamPsoDesc{};
	streamPsoDesc.InputLayout = { m_inputLayout.data(), static_cast<UINT>(m_inputLayout.size()) };
	streamPsoDesc.pRootSignature = rootSignature.Get();
	streamPsoDesc.VS = CD3DX12_SHADER_BYTECODE{ vertexShader.Get() };
	streamPsoDesc.GS = CD3DX12_SHADER_BYTECODE{ streamGeometryShader.Get() };
	streamPsoDesc.StreamOutput = streamOutput;
	streamPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC{ D3D12_DEFAULT };
	streamPsoDesc.DepthStencilState = depthStencilState;
	streamPsoDesc.BlendState = blendState;
	streamPsoDesc.SampleMask = UINT_MAX;
	streamPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	streamPsoDesc.NumRenderTargets = 0;
	streamPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	streamPsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	streamPsoDesc.SampleDesc.Count = 1;
	DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&streamPsoDesc, IID_PPV_ARGS(&m_streamPipelineState)));

	// 통상 렌더용 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.InputLayout = { m_inputLayout.data(), static_cast<UINT>(m_inputLayout.size()) };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE{ vertexShader.Get() };
	psoDesc.GS = CD3DX12_SHADER_BYTECODE{ geometryShader.Get() };
	psoDesc.PS = CD3DX12_SHADER_BYTECODE{ pixelShader.Get() };
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC{ D3D12_DEFAULT };
	psoDesc.DepthStencilState = depthStencilState;
	psoDesc.BlendState = blendState;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}