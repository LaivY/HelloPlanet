#pragma once

class Camera;
class Mesh;
class Player;
class Shader;
class Texture;

struct Light
{
	XMFLOAT4X4	lightViewMatrix;
	XMFLOAT4X4	lightProjMatrix;
	XMFLOAT3	color;
	FLOAT		padding1;
	XMFLOAT3	direction;
	FLOAT		padding2;
};

struct ShadowLight
{
	XMFLOAT4X4	lightViewMatrix[Setting::SHADOWMAP_COUNT];
	XMFLOAT4X4	lightProjMatrix[Setting::SHADOWMAP_COUNT];
	XMFLOAT3	color;
	FLOAT		padding1;
	XMFLOAT3	direction;
	FLOAT		padding2;
};

struct cbGameScene
{
	ShadowLight shadowLight;
	Light		ligths[Setting::MAX_LIGHTS];
};

class Scene abstract
{
public:
	Scene();
	virtual ~Scene() = default;

	// 이벤트 함수
	virtual void OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
						const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postProcessRootSignature,
						const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory);
	virtual void OnInitEnd();
	virtual void OnDestroy();
	virtual void OnResize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime);
	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnKeyboardEvent(FLOAT deltaTime);
	virtual void OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnUpdate(FLOAT deltaTime);

	// 게임루프 함수
	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void PreRender(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;
	virtual void Render2D(const ComPtr<ID2D1DeviceContext2>& device) const;
	virtual void PostProcessing(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& postRootSignature, const ComPtr<ID3D12Resource>& renderTarget) const;

	// 서버 통신 함수
	virtual void ProcessClient();

	// 게터
	virtual Player* GetPlayer() const;
	virtual Camera* GetCamera() const;

	// 리소스
	static unordered_map<string, shared_ptr<Mesh>>		s_meshes;
	static unordered_map<string, shared_ptr<Shader>>	s_shaders;
	static unordered_map<string, shared_ptr<Texture>>	s_textures;

protected:
	D3D12_VIEWPORT	m_viewport;
	D3D12_RECT		m_scissorRect;
};