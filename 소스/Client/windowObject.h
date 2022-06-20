#pragma once
#include "uiObject.h"

class TextObject;
class MenuTextObject;

class WindowObject : public UIObject
{
public:
	WindowObject(FLOAT width, FLOAT height, BOOL hasOutline = TRUE);
	~WindowObject() = default;

	void OnResize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual void OnMouseEvent(HWND hWnd, FLOAT deltaTime);

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader = nullptr);
	virtual void Update(FLOAT deltaTime);

	void Render2D(const ComPtr<ID2D1DeviceContext2>& device);

	void Add(unique_ptr<UIObject>& uiObject);
	void Add(unique_ptr<MenuUIObject>& uiObject);
	void Add(unique_ptr<RewardUIObject>& uiObject);
	void Add(unique_ptr<TextObject>& textObject);
	void Add(unique_ptr<MenuTextObject>& textObject);

private:
	vector<unique_ptr<UIObject>>	m_uiObjects;
	vector<unique_ptr<TextObject>>	m_textObjects;
};