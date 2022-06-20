#pragma once
#include "scene.h"

class Camera;
class GameObject;
class Player;
class Monster;
class ShadowMap;
class Skybox;
class TextObject;
class UIObject;
class WindowObject;

class GameScene : public Scene
{
public:
	GameScene();
	~GameScene();

	virtual void OnInit(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList,
						const ComPtr<ID3D12RootSignature>& rootSignature, const ComPtr<ID3D12RootSignature>& postRootSignature,
						const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory);
	virtual void OnDestroy();
	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime);
	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnKeyboardEvent(FLOAT deltaTime);
	virtual void OnKeyboardEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnUpdate(FLOAT deltaTime);

	virtual void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void PreRender(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;
	virtual void Render2D(const ComPtr<ID2D1DeviceContext2>& device);
	virtual void PostProcessing(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& postRootSignature, const ComPtr<ID3D12Resource>& renderTarget);

	virtual void ProcessClient();

	virtual shared_ptr<Player> GetPlayer() const;

	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateTextObjects(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory);
	void CreateLights() const;
	void LoadMapObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& mapFile);

	void CreateExitWindow();
	void CloseWindow();

	void Update(FLOAT deltaTime);
	void PlayerCollisionCheck(FLOAT deltaTime);
	void UpdateShadowMatrix();

	void RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

	void RecvPacket();
	void RecvLoginOk();
	void RecvUpdateClient();
	void RecvUpdateMonster();
	void RecvBulletFire();
	void RecvBulletHit();
	void RecvMosterAttack();
	void RecvRoundResult();
	void RecvLogoutOkPacket();

	// 이전 씬에서 데이터를 가져옴
	void SetPlayer(unique_ptr<Player>& player);
	void SetMultiPlayers(array<unique_ptr<Player>, Setting::MAX_PLAYERS>& multiPlayers);

	static unordered_map<INT, unique_ptr<Monster>> s_monsters;

	// 총구 이펙트
	static vector<unique_ptr<GameObject>>			screenObjects;

private:
	ComPtr<ID3D12Resource>					m_cbGameScene;		// 상수 버퍼
	cbGameScene*							m_pcbGameScene;		// 상수 버퍼 포인터
	unique_ptr<cbGameScene>					m_cbGameSceneData;	// 상수 버퍼 데이터

	unique_ptr<GameObject>					m_fullScreenQuad;	// 화면을 가득 채우는 사각형
	unique_ptr<Skybox>						m_skybox;			// 스카이박스
	shared_ptr<Camera>						m_camera;			// 카메라
	shared_ptr<Camera>						m_screenCamera;		// 스크린카메라
	unique_ptr<Camera>						m_uiCamera;			// UI 카메라
	shared_ptr<Player>						m_player;			// 플레이어
	array<unique_ptr<Player>,
		  Setting::MAX_PLAYERS>				m_multiPlayers;		// 멀티플레이어
	vector<unique_ptr<GameObject>>			m_gameObjects;		// 게임오브젝트들
	vector<unique_ptr<UIObject>>			m_uiObjects;		// UI 오브젝트
	vector<unique_ptr<TextObject>>			m_textObjects;		// 텍스트 오브젝트
	vector<unique_ptr<WindowObject>>		m_windowObjects;	// 윈도우 오브젝트
};