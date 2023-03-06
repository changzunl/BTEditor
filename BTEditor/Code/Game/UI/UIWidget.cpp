#include "Game/UI/UIWidget.hpp"

#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"

#include "Game/Framework/GameCommon.hpp"

#include <algorithm>

bool UI_RENDER_DEBUG_LABEL = false;


Vec2 UIWidget::GetMousePos()
{
    return g_theInput->GetMousePositionInWindow(g_theRenderer->GetViewport());
}

//========================================================================================
UIWidget::UIWidget(const char* name /*= ""*/)
    : m_name(name)
{

}


//========================================================================================
UIWidget::~UIWidget()
{
    for (auto* child : m_chidren)
        delete child;
}


//========================================================================================
void UIWidget::OnInit()
{

}


//========================================================================================
void UIWidget::OnUpdate()
{
    for (auto* child : m_chidren)
        child->OnUpdate();
}


//========================================================================================
void UIWidget::OnUpdateRender()
{
}


//========================================================================================
void UIWidget::OnDispose()
{

}


//========================================================================================
void UIWidget::OnChildAdded(UIWidget* child)
{
}


//========================================================================================
bool UIWidget::OnMouseClick(const Vec2& pos, char keycode)
{
    if (m_focused)
    {
        if (m_focusChild == m_hoverChild)
        {
            if (m_focusChild)
                return m_focusChild->OnMouseClick(pos, keycode);
            else
                return OnClick(pos, keycode);
        }
        else
        {
            if (m_focusChild)
                m_focusChild->OnLoseFocus();

            if (!m_hoverChild || !m_hoverChild->m_enabled)
                m_focusChild = nullptr;
            else
                m_focusChild = m_hoverChild;
            
            if (m_focusChild)
            {
                m_focusChild->OnGainFocus(pos);
                return m_focusChild->OnMouseClick(pos, keycode);
            }
            else
            {
                return OnClick(pos, keycode);
            }
        }
    }
    return false;
}


//========================================================================================
bool UIWidget::OnClick(const Vec2& pos, char keycode)
{
    return false;
}


//========================================================================================
void UIWidget::OnHoverBegin(const Vec2& pos)
{
    if (!m_hovered)
    {
       m_hovered = true;

        UIWidget* hoverChild = nullptr;
        for (auto* child : m_chidren)
        {
            if (child->m_viewBox.IsPointInside(pos))
            {
                if (!hoverChild || hoverChild->m_zDepth > child->m_zDepth)
                    hoverChild = child;
            }
        }

        m_hoverChild = hoverChild;
        if (m_hoverChild)
            m_hoverChild->OnHoverBegin(pos);
    }
}


//========================================================================================
void UIWidget::OnHoverUpdate(const Vec2& pos, bool recal /*= false*/)
{
    recal = true;
    if (recal)
    {
        UIWidget* hoverChild = nullptr;
        for (auto* child : m_chidren)
        {
            if (child->m_viewBox.IsPointInside(pos))
            {
                if (!hoverChild || hoverChild->m_zDepth > child->m_zDepth)
                    hoverChild = child;
            }
        }

        if (m_hoverChild != hoverChild)
        {
            if (m_hoverChild)
                m_hoverChild->OnHoverEnd(pos);
            m_hoverChild = hoverChild;
            if (m_hoverChild)
            {
                m_hoverChild->OnHoverBegin(pos);
                m_hoverChild->OnHoverUpdate(pos, true);
            }
        }
        else if (m_hoverChild)
        {
            m_hoverChild->OnHoverUpdate(pos, true);
        }
    }
    else
    {
        bool recalHover = false;

        if (m_hoverChild)
        {
            if (m_hoverChild->m_viewBox.IsPointInside(pos))
            {
                m_hoverChild->OnHoverUpdate(pos);
            }
            else
            {
                m_hoverChild->OnHoverEnd(pos);
                recalHover = true;
            }
        }
        else
        {
            recalHover = true;
        }

        if (recalHover)
        {
            UIWidget* hoverChild = nullptr;
            for (auto* child : m_chidren)
            {
                if (child->m_viewBox.IsPointInside(pos))
                {
                    if (!hoverChild || hoverChild->m_zDepth > child->m_zDepth)
                        hoverChild = child;
                }
            }

            m_hoverChild = hoverChild;
            if (m_hoverChild)
            {
                m_hoverChild->OnHoverBegin(pos);
                m_hoverChild->OnHoverUpdate(pos);
            }
        }
    }
}


//========================================================================================
void UIWidget::OnHoverEnd(const Vec2& pos)
{
    if (m_hovered)
    {
        m_hovered = false;

        if (m_hoverChild)
        {
            m_hoverChild->OnHoverEnd(pos);
            m_hoverChild = nullptr;
        }
    }
}


//========================================================================================
bool UIWidget::OnMouseDragBegin(const Vec2& pos, char keycode)
{
    if (m_focusChild)
        return m_focusChild->OnMouseDragBegin(pos, keycode);
    return false; // not draggable
}


//========================================================================================
void UIWidget::OnMouseDragFinish(const Vec2& pos, char keycode)
{
    if (m_focusChild)
        m_focusChild->OnMouseDragFinish(pos, keycode);
}


//========================================================================================
bool UIWidget::AcceptDraggableWidget(UIWidget* source, const Vec2& pos, char keycode)
{
    if (m_hoverChild)
        return m_hoverChild->AcceptDraggableWidget(source, pos, keycode);
    return false;
}


//========================================================================================
void UIWidget::OnKeyPressed(char keycode)
{
    if (m_focusChild)
        m_focusChild->OnKeyPressed(keycode);
}


//========================================================================================
void UIWidget::OnCharInput(char charcode)
{
    if (m_focusChild)
        m_focusChild->OnCharInput(charcode);
}


//========================================================================================
void UIWidget::OnGainFocus(const Vec2& mousePos)
{
    if (!m_focused)
    {
        m_focused = true;

        if (!m_hoverChild || !m_hoverChild->m_enabled)
            m_focusChild = nullptr;
        else
            m_focusChild = m_hoverChild;

        if (m_focusChild)
            m_focusChild->OnGainFocus(mousePos);
    }
}


//========================================================================================
void UIWidget::OnLoseFocus()
{
    if (m_focused)
    {
        m_focused = false;
        if (m_focusChild)
        {
            m_focusChild->OnLoseFocus();
            m_focusChild = nullptr;
        }
    }
}


//========================================================================================
void UIWidget::OnQuickMenu(int, const std::string&)
{

}

//========================================================================================
void UIWidget::Render(RenderPass pass) const
{
    if (m_render)
    {
        if (pass == RenderPass::BG)
        {
            AABB2 box = m_viewBox;
            AddVertsForAABB2D(g_vertsBuffer, box, m_focused ? Rgba8::YELLOW : m_hovered ? Rgba8(0, 128, 255) : Rgba8::BLACK);
            box.SetDimensions(box.GetDimensions() - Vec2(5, 5));
            AddVertsForAABB2D(g_vertsBuffer, box, m_color);
        }
        else if (pass == RenderPass::SYMBOL && UI_RENDER_DEBUG_LABEL)
        {
            AABB2 box = m_viewBox;
            box.SetDimensions(box.GetDimensions() * Vec2(0.9f, 0.9f));
            auto color = m_enabled ? Rgba8::WHITE : Rgba8(160, 160, 160);
            color.a = 40;

            g_uiFont->AddVertsForTextInBox2D(g_vertsBuffer, box, 20.0f, m_name, color, 0.666f);
        }
    }

    RenderChildren(pass);
}


//========================================================================================
bool UIWidget::PostMessage(UIWidget* source, const std::string& message)
{
    if (m_owner)
        return m_owner->PostMessage(source, message);
    return false;
}


//========================================================================================
void UIWidget::RenderChildren(RenderPass pass) const
{
    for (auto* child : m_chidren)
        child->Render(pass);
}


//========================================================================================
void UIWidget::AddChild(UIWidget* child)
{
    m_chidren.push_back(child);
    child->m_owner = this;
    child->SetViewport(m_viewBox);

    auto* canvas = m_canvas;
    child->m_canvas = canvas;
    child->ForEachChild_DFS([canvas](auto child) { child->m_canvas = canvas; });

    OnChildAdded(child);
}


//========================================================================================
void UIWidget::AddChildComponent(UIWidget* child, const Vec2& relPos, const Vec2& relSize)
{
    child->m_rect.m_pos = relPos;
    child->m_rect.m_size = relSize;
    AddChild(child);
}


void UIWidget::AddChildComponent(UIWidget* child, const Vec2& relPos, const Vec2& relSize, const Vec2& align)
{
    child->m_rect.m_align = align;
    AddChildComponent(child, relPos, relSize);
}

//========================================================================================
void UIWidget::AddChildLayout(UIWidget* child, ELayoutSide side, float size)
{
    AABB2 layoutBox;

    if (size < 0)
    {
        layoutBox = m_layoutBox;
        switch (side)
        {
        case ELayoutSide::BOTTOM:            m_layoutBox.m_maxs.y = m_layoutBox.m_mins.y; break;
        case ELayoutSide::TOP:               m_layoutBox.m_mins.y = m_layoutBox.m_maxs.y; break;
        case ELayoutSide::LEFT:              m_layoutBox.m_maxs.x = m_layoutBox.m_mins.x; break;
        case ELayoutSide::RIGHT:             m_layoutBox.m_mins.x = m_layoutBox.m_maxs.x; break;
        default:                             throw "no corresponding enum case";
        }
    }
    else
    {
        switch (side)
        {
        case ELayoutSide::BOTTOM:            layoutBox = m_layoutBox.ChopOffBottom(size, false); break;
        case ELayoutSide::TOP:               layoutBox = m_layoutBox.ChopOffTop(size, false); break;
        case ELayoutSide::LEFT:              layoutBox = m_layoutBox.ChopOffLeft(size, false); break;
        case ELayoutSide::RIGHT:             layoutBox = m_layoutBox.ChopOffRight(size, false); break;
        default:                             throw "no corresponding enum case";
        }
    }

    child->SetBox(layoutBox);
    AddChild(child);
}


//========================================================================================
void UIWidget::RemoveChild(UIWidget* child)
{
    if (m_hoverChild == child)
    {
        m_hoverChild->OnHoverEnd(GetMousePos());
        m_hoverChild = nullptr;
    }
    if (m_focusChild == child)
    {
        m_focusChild->OnLoseFocus();
        m_focusChild = nullptr;
    }

    auto ite = std::find(m_chidren.begin(), m_chidren.end(), child);

    if (ite != m_chidren.end())
    {
        child->m_owner = nullptr;
        m_chidren.erase(ite);
    }
}


//========================================================================================
void UIWidget::DeleteChild(UIWidget* child)
{
    RemoveChild(child);
    delete child;
}


//========================================================================================
void UIWidget::RemoveAllChildren()
{
    if (m_hoverChild)
    {
        m_hoverChild->OnHoverEnd(GetMousePos());
        m_hoverChild = nullptr;
    }
    if (m_focusChild)
    {
        m_focusChild->OnLoseFocus();
        m_focusChild = nullptr;
    }

    m_chidren.clear();
}


//========================================================================================
void UIWidget::ClearAllChildren()
{
    if (m_hoverChild)
    {
        m_hoverChild->OnHoverEnd(GetMousePos());
        m_hoverChild = nullptr;
    }
    if (m_focusChild)
    {
        m_focusChild->OnLoseFocus();
        m_focusChild = nullptr;
    }

    auto children = std::move(m_chidren);
    for (auto* child : children)
        delete child;
}


//========================================================================================
UIWidget* UIWidget::FindChild(const char* name) const
{
    for (auto child : m_chidren)
    {
        if (child->m_name == name)
            return child;
    }
    for (auto child : m_chidren)
    {
        UIWidget* result = child->FindChild(name);
        if (result)
            return result;
    }
    return nullptr;
}


//========================================================================================
void UIWidget::ForEachChild_BFS(std::function<void(UIWidget*)> action)
{
    std::vector<UIWidget*> children1 = m_chidren;
    std::vector<UIWidget*> children2;
    action(this);

    while (!children1.empty())
    {
        for (auto* child : children1)
        {
            action(child);
            children2.insert(children2.end(), child->m_chidren.begin(), child->m_chidren.end());
        }
        children1.clear();
        children1.swap(children2);
    }
}


//========================================================================================
void UIWidget::ForEachChild_DFS(std::function<void(UIWidget*)> action)
{
    action(this);
    for (auto* child : m_chidren)
    {
        child->ForEachChild_DFS(action);
    }
}


//========================================================================================
void UIWidget::SetViewport(const AABB2& viewport)
{
    m_viewBox = GetViewBox(viewport);

    for (auto& child : m_chidren)
    {
        child->SetViewport(m_viewBox);
    }
}


//========================================================================================
AABB2 UIWidget::GetViewBox(const AABB2& parentView) const
{
    AABB2 box = GetBox();

    return parentView.GetSubBox(box);
}


//========================================================================================
AABB2 UIWidget::GetBox() const
{
    AABB2 box(m_rect.m_pos, m_rect.m_pos);
    box.m_mins -= m_rect.m_size * m_rect.m_align;
    box.m_maxs += m_rect.m_size * (1.0f - m_rect.m_align);

    return box;
}


//========================================================================================
void UIWidget::SetBox(const AABB2& box, Vec2 align /*= Vec2(0.5f, 0.5f)*/)
{
    m_rect.m_pos = box.GetCenter();
    m_rect.m_size = box.GetDimensions();
    m_rect.m_align = align;
}


//========================================================================================
void UIWidget::RecalBox(bool recursive /*= true*/)
{
    if (m_owner)
    {
        AABB2 box;
        box.m_mins = m_owner->m_viewBox.GetUVForPoint(m_viewBox.m_mins);
        box.m_maxs = m_owner->m_viewBox.GetUVForPoint(m_viewBox.m_maxs);

        SetBox(box, m_rect.m_align);
    }

    if (recursive)
        for (auto child : m_chidren)
            child->RecalBox();
}

