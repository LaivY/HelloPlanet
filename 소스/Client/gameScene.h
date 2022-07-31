#pragma once
#include "scene.h"

class BlurFilter;
class Camera;
class ThirdPersonCamera;
class GameObject;
class OutlineObject;
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
	virtual void Render2D(const ComPtr<ID2D1DeviceContext2>& device) const;
	virtual void PostProcessing(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12RootSignature>& postRootSignature, const ComPtr<ID3D12Resource>& renderTarget) const;

	virtual void ProcessClient();

	virtual Player* GetPlayer() const;
	virtual Camera* GetCamera() const;

	void OnPlayerHit(Monster* monster);
	void OnPlayerDie();
	void OnPlayerRevive();

	// 이전 씬에서 데이터를 가져옴
	void SetPlayer(unique_ptr<Player>& player);
	void SetMultiPlayers(array<shared_ptr<Player>, Setting::MAX_PLAYERS>& multiPlayers);

	// 일부 게임오브젝트들
	static vector<unique_ptr<Monster>>		s_monsters;
	static vector<unique_ptr<GameObject>>	s_screenObjects;

private:
	void CreateShaderVariable(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateGameObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateUIObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList);
	void CreateTextObjects(const ComPtr<ID2D1DeviceContext2>& d2dDeivceContext, const ComPtr<IDWriteFactory>& dWriteFactory);
	void CreateLights() const;
	void LoadMapObjects(const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12GraphicsCommandList>& commandList, const string& mapFile);

	void RenderGameObjects(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;
	void RenderMultiPlayers(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;
	void RenderMonsters(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;
	void RenderScreenObjects(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;
	void RenderPlayer(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;
	void RenderUIObjects(const ComPtr<ID3D12GraphicsCommandList>& commandList, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle) const;

	void RenderToShadowMap(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;
	void RenderOutline(const ComPtr<ID3D12GraphicsCommandList>& commandList, const unique_ptr<OutlineObject>& outliner) const;
	void Update(FLOAT deltaTime);
	void PlayerCollisionCheck(FLOAT deltaTime);
	void UpdateShadowMatrix();
	void CreateExitWindow();

	void RecvPacket();
	void RecvUpdateClient();
	void RecvUpdateMonster();
	void RecvBulletFire();
	void RecvBulletHit();
	void RecvMosterAttack();
	void RecvRoundResult();
	void RecvLogoutOkPacket();
	void RecvRoundClear();

public:
	ComPtr<ID3D12Resource>				m_cbGameScene;		// 상수 버퍼
	cbGameScene*						m_pcbGameScene;		// 상수 버퍼 포인터
	unique_ptr<cbGameScene>				m_cbGameSceneData;	// 상수 버퍼 데이터

	unique_ptr<BlurFilter>				m_blurFilter;		// 블러필터
	unique_ptr<OutlineObject>			m_redOutliner;		// 빨강 외곽선
	unique_ptr<OutlineObject>			m_greenOutliner;	// 초록 외곽선
	unique_ptr<OutlineObject>			m_blackOutliner;	// 검정 외곽선

	shared_ptr<Camera>					m_camera;			// 메인 카메라
	shared_ptr<Camera>					m_showCamera;		// 연출 카메라
	shared_ptr<Camera>					m_observeCamera;	// 관전 카메라
	unique_ptr<Camera>					m_screenCamera;		// 스크린 카메라
	unique_ptr<Camera>					m_uiCamera;			// UI 카메라

	unique_ptr<Skybox>					m_skybox;			// 스카이박스
	shared_ptr<Player>					m_player;			// 플레이어
	array<shared_ptr<Player>,
		  Setting::MAX_PLAYERS>			m_multiPlayers;		// 멀티플레이어
	vector<unique_ptr<GameObject>>		m_gameObjects;		// 게임오브젝트들
	vector<unique_ptr<UIObject>>		m_uiObjects;		// UI 오브젝트
	vector<unique_ptr<TextObject>>		m_textObjects;		// 텍스트 오브젝트
	vector<unique_ptr<WindowObject>>	m_windowObjects;	// 윈도우 오브젝트

	BOOL								m_isShowing;		// 연출 중
};