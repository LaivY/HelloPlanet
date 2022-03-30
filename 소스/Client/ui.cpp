#include "ui.h"

void UI::Update(FLOAT deltaTime)
{
	for (auto& o : m_uiObjects)
		o->Update(deltaTime);
}

void UI::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{

}

UIObject::UIObject(const XMFLOAT2& size) : m_pivot{ eUIPivot::CENTER }, m_position{}, m_size{ size }
{
	m_mesh = make_unique<Mesh>();
}

void UIObject::Update(FLOAT deltaTime)
{
	
}

void UIObject::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{

}

void UIObject::SetPivot(eUIPivot pivot)
{
	m_pivot = pivot;
}

void UIObject::SetPosition(const XMFLOAT2& position)
{
	switch (m_pivot)
	{
	case eUIPivot::LEFTTOP:
		break;
	case eUIPivot::CENTERTOP:
		break;
	case eUIPivot::RIGHTTOP:
		break;
	case eUIPivot::LEFTCENTER:
		break;
	case eUIPivot::CENTER:
		break;
	case eUIPivot::RIGHTCENTER:
		break;
	case eUIPivot::LEFTBOT:
		break;
	case eUIPivot::CENTERBOT:
		break;
	case eUIPivot::RIGHTBOT:
		break;
	}
}

void UIObject::SetTexture(const shared_ptr<Texture>& texture)
{
	m_texture = texture;
}