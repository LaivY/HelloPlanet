#include "stdafx.h"
#include "windowObject.h"
#include "scene.h"
#include "textObject.h"

WindowObject::WindowObject(FLOAT width, FLOAT height, BOOL hasOutline) : UIObject{ width, height }
{
	if (!hasOutline) return;
	constexpr float thickness{ 5.0f };

	// 좌측(왼쪽 위, 아래 모서리 포함)
	auto outline1{ make_unique<UIObject>(thickness, height + thickness * 2.0f) };
	outline1->SetTexture(Scene::s_textures["OUTLINE"]);
	outline1->SetPivot(ePivot::RIGHTCENTER);
	outline1->SetScreenPivot(ePivot::LEFTCENTER);
	outline1->SetPosition(XMFLOAT2{});
	Add(outline1);

	// 우측(오른쪽 위, 아래 모서리 포함)
	auto outline2{ make_unique<UIObject>(thickness, height + thickness * 2.0f) };
	outline2->SetTexture(Scene::s_textures["OUTLINE"]);
	outline2->SetPivot(ePivot::LEFTCENTER);
	outline2->SetScreenPivot(ePivot::RIGHTCENTER);
	outline2->SetPosition(XMFLOAT2{});
	Add(outline2);

	// 상단(왼쪽, 오른쪽 위 제외)
	auto outline3{ make_unique<UIObject>(width, thickness) };
	outline3->SetTexture(Scene::s_textures["OUTLINE"]);
	outline3->SetPivot(ePivot::CENTERBOT);
	outline3->SetScreenPivot(ePivot::CENTERTOP);
	outline3->SetPosition(XMFLOAT2{});
	Add(outline3);

	// 하단(왼쪽, 오른쪽 아래 제외)
	auto outline4{ make_unique<UIObject>(width, thickness) };
	outline4->SetTexture(Scene::s_textures["OUTLINE"]);
	outline4->SetPivot(ePivot::CENTERTOP);
	outline4->SetScreenPivot(ePivot::CENTERBOT);
	outline4->SetPosition(XMFLOAT2{});
	Add(outline4);
}

void WindowObject::OnResize(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	for (auto& ui : m_uiObjects)
		ui->SetPosition(ui->GetPivotPosition());
	for (auto& t : m_textObjects)
		t->SetPosition(t->GetPivotPosition());
}

void WindowObject::OnMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	for (auto& ui : m_uiObjects)
		ui->OnMouseEvent(hWnd, message, wParam, lParam);
	for (auto& t : m_textObjects)
		t->OnMouseEvent(hWnd, message, wParam, lParam);
}

void WindowObject::OnMouseEvent(HWND hWnd, FLOAT deltaTime)
{
	for (auto& ui : m_uiObjects)
		ui->OnMouseEvent(hWnd, deltaTime);
	for (auto& t : m_textObjects)
		t->OnMouseEvent(hWnd, deltaTime);
}

void WindowObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const shared_ptr<Shader>& shader)
{
	UIObject::Render(commandList, shader);
	for (const auto& ui : m_uiObjects)
		ui->Render(commandList, shader);
}

void WindowObject::Update(FLOAT deltaTime)
{
	for (auto& ui : m_uiObjects)
		ui->Update(deltaTime);
	for (auto& t : m_textObjects)
		t->Update(deltaTime);
}

void WindowObject::Render2D(const ComPtr<ID2D1DeviceContext2>& device)
{
	for (const auto& t : m_textObjects)
		t->Render(device);
}

void WindowObject::Add(unique_ptr<UIObject>& uiObject)
{
	XMFLOAT3 winPos{ GetPosition() };
	float hw{ m_width / 2.0f };
	float hh{ m_height / 2.0f };

	XMFLOAT2 uiPos{ uiObject->GetPivotPosition() };
	ePivot uiScreenPivot{ uiObject->GetScreenPivot() };
	uiObject->SetScreenPivot(m_screenPivot);
	switch (uiScreenPivot)
	{
	case ePivot::LEFTTOP:
		uiObject->SetPosition(XMFLOAT2{ winPos.x - hw + uiPos.x, winPos.y + hh + uiPos.y });
		break;
	case ePivot::CENTERTOP:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + uiPos.x, winPos.y + uiPos.y + hh });
		break;
	case ePivot::RIGHTTOP:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + hw + uiPos.x, winPos.y + uiPos.y + hh });
		break;
	case ePivot::LEFTCENTER:
		uiObject->SetPosition(XMFLOAT2{ winPos.x - hw + uiPos.x, winPos.y + uiPos.y });
		break;
	case ePivot::CENTER:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + uiPos.x, winPos.y + uiPos.y });
		break;
	case ePivot::RIGHTCENTER:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + hw + uiPos.x, winPos.y + uiPos.y });
		break;
	case ePivot::LEFTBOT:
		uiObject->SetPosition(XMFLOAT2{ winPos.x - hw + uiPos.x, winPos.y - hh + uiPos.y });
		break;
	case ePivot::CENTERBOT:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + uiPos.x, winPos.y - hh + uiPos.y });
		break;
	case ePivot::RIGHTBOT:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + hw + uiPos.x, winPos.y - hh + uiPos.y });
		break;
	}
	m_uiObjects.push_back(move(uiObject));
}

void WindowObject::Add(unique_ptr<MenuUIObject>& uiObject)
{
	XMFLOAT3 winPos{ GetPosition() };
	float hw{ m_width / 2.0f };
	float hh{ m_height / 2.0f };

	XMFLOAT2 uiPos{ uiObject->GetPivotPosition() };
	ePivot uiScreenPivot{ uiObject->GetScreenPivot() };
	uiObject->SetScreenPivot(m_screenPivot);
	switch (uiScreenPivot)
	{
	case ePivot::LEFTTOP:
		uiObject->SetPosition(XMFLOAT2{ winPos.x - hw + uiPos.x, winPos.y + hh + uiPos.y });
		break;
	case ePivot::CENTERTOP:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + uiPos.x, winPos.y + uiPos.y + hh });
		break;
	case ePivot::RIGHTTOP:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + hw + uiPos.x, winPos.y + uiPos.y + hh });
		break;
	case ePivot::LEFTCENTER:
		uiObject->SetPosition(XMFLOAT2{ winPos.x - hw + uiPos.x, winPos.y + uiPos.y });
		break;
	case ePivot::CENTER:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + uiPos.x, winPos.y + uiPos.y });
		break;
	case ePivot::RIGHTCENTER:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + hw + uiPos.x, winPos.y + uiPos.y });
		break;
	case ePivot::LEFTBOT:
		uiObject->SetPosition(XMFLOAT2{ winPos.x - hw + uiPos.x, winPos.y - hh + uiPos.y });
		break;
	case ePivot::CENTERBOT:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + uiPos.x, winPos.y - hh + uiPos.y });
		break;
	case ePivot::RIGHTBOT:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + hw + uiPos.x, winPos.y - hh + uiPos.y });
		break;
	}
	m_uiObjects.push_back(move(uiObject));
}

void WindowObject::Add(unique_ptr<RewardUIObject>& uiObject)
{
	XMFLOAT3 winPos{ GetPosition() };
	float hw{ m_width / 2.0f };
	float hh{ m_height / 2.0f };

	XMFLOAT2 uiPos{ uiObject->GetPivotPosition() };
	ePivot uiScreenPivot{ uiObject->GetScreenPivot() };
	uiObject->SetScreenPivot(m_screenPivot);
	switch (uiScreenPivot)
	{
	case ePivot::LEFTTOP:
		uiObject->SetPosition(XMFLOAT2{ winPos.x - hw + uiPos.x, winPos.y + hh + uiPos.y });
		break;
	case ePivot::CENTERTOP:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + uiPos.x, winPos.y + uiPos.y + hh });
		break;
	case ePivot::RIGHTTOP:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + hw + uiPos.x, winPos.y + uiPos.y + hh });
		break;
	case ePivot::LEFTCENTER:
		uiObject->SetPosition(XMFLOAT2{ winPos.x - hw + uiPos.x, winPos.y + uiPos.y });
		break;
	case ePivot::CENTER:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + uiPos.x, winPos.y + uiPos.y });
		break;
	case ePivot::RIGHTCENTER:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + hw + uiPos.x, winPos.y + uiPos.y });
		break;
	case ePivot::LEFTBOT:
		uiObject->SetPosition(XMFLOAT2{ winPos.x - hw + uiPos.x, winPos.y - hh + uiPos.y });
		break;
	case ePivot::CENTERBOT:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + uiPos.x, winPos.y - hh + uiPos.y });
		break;
	case ePivot::RIGHTBOT:
		uiObject->SetPosition(XMFLOAT2{ winPos.x + hw + uiPos.x, winPos.y - hh + uiPos.y });
		break;
	}
	m_uiObjects.push_back(move(uiObject));
}

void WindowObject::Add(unique_ptr<TextObject>& textObject)
{
	XMFLOAT3 winPos{ GetPosition() };
	float hw{ m_width / 2.0f };
	float hh{ m_height / 2.0f };

	XMFLOAT2 textPos{ textObject->GetPivotPosition() };
	ePivot textScreenPivot{ textObject->GetScreenPivot() };
	textObject->SetScreenPivot(ePivot::CENTER);
	switch (textScreenPivot)
	{
	case ePivot::LEFTTOP:
		textObject->SetPosition(XMFLOAT2{ winPos.x - hw + textPos.x, winPos.y - hh - textPos.y });
		break;
	case ePivot::CENTERTOP:
		textObject->SetPosition(XMFLOAT2{ winPos.x + textPos.x, winPos.y - hh - textPos.y });
		break;
	case ePivot::RIGHTTOP:
		textObject->SetPosition(XMFLOAT2{ winPos.x + hw + textPos.x, winPos.y - hh - textPos.y });
		break;
	case ePivot::LEFTCENTER:
		textObject->SetPosition(XMFLOAT2{ winPos.x - hw + textPos.x, winPos.y - textPos.y });
		break;
	case ePivot::CENTER:
		textObject->SetPosition(XMFLOAT2{ winPos.x + textPos.x, winPos.y - textPos.y });
		break;
	case ePivot::RIGHTCENTER:
		textObject->SetPosition(XMFLOAT2{ winPos.x + hw + textPos.x, winPos.y - textPos.y });
		break;
	case ePivot::LEFTBOT:
		textObject->SetPosition(XMFLOAT2{ winPos.x - hw + textPos.x, winPos.y + hh - textPos.y });
		break;
	case ePivot::CENTERBOT:
		textObject->SetPosition(XMFLOAT2{ winPos.x + textPos.x, winPos.y + hh - textPos.y });
		break;
	case ePivot::RIGHTBOT:
		textObject->SetPosition(XMFLOAT2{ winPos.x + hw + textPos.x, winPos.y + hh - textPos.y });
		break;
	}
	m_textObjects.push_back(move(textObject));
}

void WindowObject::Add(unique_ptr<MenuTextObject>& textObject)
{
	XMFLOAT3 winPos{ GetPosition() };
	float hw{ m_width / 2.0f };
	float hh{ m_height / 2.0f };

	XMFLOAT2 textPos{ textObject->GetPivotPosition() };
	ePivot textScreenPivot{ textObject->GetScreenPivot() };
	textObject->SetScreenPivot(ePivot::CENTER);
	switch (textScreenPivot)
	{
	case ePivot::LEFTTOP:
		textObject->SetPosition(XMFLOAT2{ winPos.x - hw + textPos.x, winPos.y - hh - textPos.y });
		break;
	case ePivot::CENTERTOP:
		textObject->SetPosition(XMFLOAT2{ winPos.x + textPos.x, winPos.y - hh - textPos.y });
		break;
	case ePivot::RIGHTTOP:
		textObject->SetPosition(XMFLOAT2{ winPos.x + hw + textPos.x, winPos.y - hh - textPos.y });
		break;
	case ePivot::LEFTCENTER:
		textObject->SetPosition(XMFLOAT2{ winPos.x - hw + textPos.x, winPos.y - textPos.y });
		break;
	case ePivot::CENTER:
		textObject->SetPosition(XMFLOAT2{ winPos.x + textPos.x, winPos.y - textPos.y });
		break;
	case ePivot::RIGHTCENTER:
		textObject->SetPosition(XMFLOAT2{ winPos.x + hw + textPos.x, winPos.y - textPos.y });
		break;
	case ePivot::LEFTBOT:
		textObject->SetPosition(XMFLOAT2{ winPos.x - hw + textPos.x, winPos.y + hh - textPos.y - textObject->GetHeight() / 2.0f });
		break;
	case ePivot::CENTERBOT:
		textObject->SetPosition(XMFLOAT2{ winPos.x + textPos.x, winPos.y + hh - textPos.y - textObject->GetHeight() / 2.0f });
		break;
	case ePivot::RIGHTBOT:
		textObject->SetPosition(XMFLOAT2{ winPos.x + hw + textPos.x, winPos.y + hh - textPos.y - textObject->GetHeight() / 2.0f });
		break;
	}
	m_textObjects.push_back(move(textObject));
}
