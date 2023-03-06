#include "Game/UI/UICanvas.hpp"

#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Game/Framework/GameCommon.hpp"

#include "Engine/Renderer/DebugRender.hpp"


//========================================================================================
UICanvas::UICanvas(const char* name /*= "Canvas"*/, AABB2 box /*= AABB2::ZERO_TO_ONE*/) 
    : UIWidget(name)
    , m_quickMenu(new UIQuickMenu(this))
{
    m_canvas = this;

    SetBox(box);
}


//========================================================================================
UICanvas::~UICanvas()
{
    Deactive();

    delete m_quickMenu;
}


//========================================================================================
void UICanvas::Active()
{
    if (!m_active)
    {
        AddChild(m_quickMenu);

        m_handlers[0] = g_theEventSystem->Subscribe("Input:CharInput", [this](EventArgs& args) { return ProcessRawInput(args); });
        m_handlers[1] = g_theEventSystem->Subscribe("Input:KeyPressed", [this](EventArgs& args) { return ProcessRawButtonDown(args); });
        m_handlers[2] = g_theEventSystem->Subscribe("Input:KeyReleased", [this](EventArgs& args) { return ProcessRawButtonUp(args); });

        m_active = true;
        OnGainFocus(GetMousePos());
    }
}


//========================================================================================
void UICanvas::Deactive()
{
    if (m_active)
    {
        m_active = false;

        g_theEventSystem->Unsubscribe("Input:CharInput", m_handlers[0]);
        g_theEventSystem->Unsubscribe("Input:KeyPressed", m_handlers[1]);
        g_theEventSystem->Unsubscribe("Input:KeyReleased", m_handlers[2]);

        RemoveChild(m_quickMenu);
    }
}


//========================================================================================
void UICanvas::OnUpdate()
{
    Vec2 pos = GetMousePos();
    if (m_focusChild != m_quickMenu && m_quickMenu->m_opened)
    {
        if (m_quickMenu->m_viewBox.IsPointInside(pos))
        {
            m_mousePos = pos;
            OnHoverBegin(pos);
            OnHoverUpdate(pos, true);
        }
    }

    if (m_memberDirty || pos != m_mousePos)
    {
        m_mousePos = pos;
        OnHoverBegin(pos);
        OnHoverUpdate(pos);
    }

    if (!m_dragWatch[0].IsStopped() && (pos - m_dragPos[0]).GetLengthSquared() > 100) // replace stopwatch with distance detection to see if it feels better
    {
        OnHoverBegin(m_dragPos[0]);
        OnHoverUpdate(m_dragPos[0], true);
        if (OnMouseDragBegin(m_dragPos[0], KEYCODE_LEFT_MOUSE))
            m_dragging[0] = true;
        m_dragWatch[0].Stop();
    }

    if (!m_dragWatch[1].IsStopped() && (pos - m_dragPos[1]).GetLengthSquared() > 100) // replace stopwatch with distance detection to see if it feels better
    {
        OnHoverBegin(m_dragPos[1]);
        OnHoverUpdate(m_dragPos[1], true);
        if (OnMouseDragBegin(m_dragPos[1], KEYCODE_RIGHT_MOUSE))
            m_dragging[1] = true;
        m_dragWatch[1].Stop();
    }


    for (auto* child : m_chidren)
        child->OnUpdate();

    m_memberDirty = false;
}

extern Vec2 BTIMAP_FONT_OVERSIZE;

//========================================================================================
void UICanvas::Render() const
{
    BTIMAP_FONT_OVERSIZE = Vec2(2.f, 1.5f) * 0.75f;

    if (!g_uiFont)
        g_uiFont = g_theRenderer->CreateOrGetBitmapFont("Data/Fonts/SDF-ASCII");
    if (!g_uiFontShader)
        g_uiFontShader = g_theRenderer->CreateOrGetShader("FontSDF");

    g_vertsBuffer.reserve(32767);
    g_vertsBuffer.clear();

    m_quickMenu->m_render = false;

    g_theRenderer->BindTexture(nullptr);
    UIWidget::Render(RenderPass::BG);
    g_theRenderer->DrawVertexArray(g_vertsBuffer);
    g_vertsBuffer.clear();

    // g_theRenderer->SetSamplerMode(SamplerMode::POINTWRAP);
    g_theRenderer->SetSamplerMode(SamplerMode::BILINEARCLAMP);
    g_theRenderer->BindTexture(&g_uiFont->GetTexture());
    g_theRenderer->BindShader(g_uiFontShader);
    UIWidget::Render(RenderPass::SYMBOL);
    g_theRenderer->DrawVertexArray(g_vertsBuffer);
    g_theRenderer->BindShader(nullptr);
    g_vertsBuffer.clear();
    g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

    g_theRenderer->BindTexture(nullptr);
    UIWidget::Render(RenderPass::OVERLAY);
    g_theRenderer->DrawVertexArray(g_vertsBuffer);
    g_vertsBuffer.clear();

    m_quickMenu->m_render = true;

    g_theRenderer->BindTexture(nullptr);
    m_quickMenu->Render(RenderPass::BG);
    g_theRenderer->DrawVertexArray(g_vertsBuffer);
    g_vertsBuffer.clear();

    g_theRenderer->SetSamplerMode(SamplerMode::BILINEARCLAMP);
    g_theRenderer->BindTexture(&g_uiFont->GetTexture());
    g_theRenderer->BindShader(g_uiFontShader);
    m_quickMenu->Render(RenderPass::SYMBOL);
    g_theRenderer->DrawVertexArray(g_vertsBuffer);
    g_theRenderer->BindShader(nullptr);
    g_vertsBuffer.clear();
    g_theRenderer->SetSamplerMode(SamplerMode::BILINEARWRAP);

    g_theRenderer->BindTexture(nullptr);
    m_quickMenu->Render(RenderPass::OVERLAY);
    g_theRenderer->DrawVertexArray(g_vertsBuffer);
    g_vertsBuffer.clear();

    BTIMAP_FONT_OVERSIZE = Vec2(1.0f, 1.0f);
}


//========================================================================================
UIWidget* UICanvas::GetFocusChild() const
{
    UIWidget* focus = m_focusChild;
    if (!focus)
        return nullptr;
    while (focus->GetFocusChild())
    {
        focus = focus->GetFocusChild();
    }
    return focus;
}


//========================================================================================
bool UICanvas::PostMessage(UIWidget* source, const std::string& message)
{
    auto ite = m_dispatcher.find(message);
    if (ite != m_dispatcher.end())
        return ite->second(source, message);
    return false;
}


//========================================================================================
bool UICanvas::OnMouseClick(const Vec2& pos, char keycode)
{
    if (m_focused && m_quickMenu->m_opened)
    {
        if (!m_quickMenu->m_viewBox.IsPointInside(m_mousePos))
        {
            m_quickMenu->SetContent(nullptr, Vec2::ZERO, Vec2::ZERO, StringList());
            m_quickMenu->m_opened = false;

            OnHoverBegin(m_mousePos);
            OnHoverUpdate(m_mousePos);
        }
    }

    return UIWidget::OnMouseClick(pos, keycode);
}


//========================================================================================
void UICanvas::AddHandle(const std::string& message, MessageHandle handle)
{
    m_dispatcher[message] = handle;
}


//========================================================================================
void UICanvas::OpenMenu(UIWidget* source, const Vec2& pos, const Vec2& size, const std::vector<std::string>& options)
{
    Vec2 compPos = m_viewBox.GetUVForPoint(pos);
    Vec2 compSize = size / m_viewBox.GetDimensions();

    m_quickMenu->SetContent(source, compPos, compSize, options);
    m_quickMenu->m_opened = true;

    OnHoverBegin(m_mousePos);
    OnHoverUpdate(m_mousePos, true);
}


//========================================================================================
void UICanvas::CloseMenu()
{
    m_quickMenu->SetContent(nullptr, Vec2::ZERO, Vec2::ZERO, StringList());
    m_quickMenu->m_opened = false;

    OnHoverUpdate(m_mousePos, true);
}


void UICanvas::SetFocus(UIWidget* widget)
{
    m_mousePos = widget->m_viewBox.GetCenter();

    OnHoverBegin(m_mousePos);
    OnHoverUpdate(m_mousePos);

    OnMouseClick(m_mousePos, KEYCODE_LEFT_MOUSE);
}


//========================================================================================
void UICanvas::SetInputEnabled(bool enabled)
{
    m_disableInput = !enabled;
}


//========================================================================================
bool UICanvas::IsInputEnabled() const
{
    return !m_disableInput;
}


//========================================================================================
bool UICanvas::ProcessRawButtonDown(EventArgs& args)
{
    if (m_active && !m_disableInput)
    {
        char charCode = args.GetValue("key", "")[0];

        if (charCode == KEYCODE_LEFT_MOUSE || charCode == KEYCODE_RIGHT_MOUSE || charCode == KEYCODE_MIDDLE_MOUSE)
        {
            DebugAddUIPoint(Vec3(GetMousePos().x, GetMousePos().y, -1), 20, 2, Rgba8(255, 255, 255, 60), Rgba8(255, 255, 255, 0), DebugRenderMode::ALWAYS);
            DebugAddUIPoint(Vec3(GetMousePos().x, GetMousePos().y, -1), 2, 1.7, Rgba8(255, 0, 0), Rgba8(255, 0, 0), DebugRenderMode::ALWAYS);

            m_dragWatch[charCode == KEYCODE_LEFT_MOUSE ? 0 : 1].Start(0.15);
            m_dragPos[charCode == KEYCODE_LEFT_MOUSE ? 0 : 1] = GetMousePos();

            OnMouseClick(GetMousePos(), KEYCODE_FOCUS);
            return true;
        }
        else
        {
            OnKeyPressed(charCode);
        }
    }
    return false;
}


//========================================================================================
bool UICanvas::ProcessRawButtonUp(EventArgs& args)
{
    if (m_active && !m_disableInput)
    {
        char charCode = args.GetValue("key", "")[0];

        if (charCode == KEYCODE_LEFT_MOUSE || charCode == KEYCODE_RIGHT_MOUSE || charCode == KEYCODE_MIDDLE_MOUSE)
        {
            int index = charCode == KEYCODE_LEFT_MOUSE ? 0 : 1;
            m_dragWatch[index].Stop();
            if (m_dragging[index])
            {
                m_dragWidget->OnMouseDragFinish(GetMousePos(), charCode);
                if (m_hoverChild)
                    m_hoverChild->AcceptDraggableWidget(m_dragWidget, GetMousePos(), charCode);
                m_dragging[index] = false;
                m_dragWidget = nullptr;
            }
            else
            {
                OnMouseClick(GetMousePos(), charCode);
            }
            return true;
        }
    }
    return false;
}


//========================================================================================
bool UICanvas::ProcessRawInput(EventArgs& args)
{
    if (m_active && !m_disableInput)
    {
        char charCode = args.GetValue("char", "")[0];
        OnCharInput(charCode);

        return true;
    }

    return false;
}


//========================================================================================
void UICanvas::MarkDirty()
{
    m_memberDirty = true;
}


//========================================================================================
UIQuickMenu::UIQuickMenu(UICanvas* canvas) : UIWidget("QuickMenu")
{
    m_canvas = canvas;
    m_zDepth = -1.0f;

    m_rect.m_align = Vec2(0, 1);
}


//========================================================================================
void UIQuickMenu::Render(RenderPass pass) const
{
    for (auto* child : m_chidren)
        child->Render(pass);
}


//========================================================================================
void UIQuickMenu::SetContent(UIWidget* source, const Vec2& pos, const Vec2& size, std::vector<std::string> options)
{
    ClearAllChildren();

    int optionSize = static_cast<int>(options.size());

    m_source = source;

    AABB2 box = AABB2(pos, pos);
    box.SetDimensions(size * Vec2(1.0f, (float)optionSize));
    SetBox(box, m_rect.m_align);
    SetViewport(m_canvas->m_viewBox);

    m_layoutBox = AABB2::ZERO_TO_ONE;

    float sizeLayout = 1.0f / optionSize;

    for (int i = 0; i < optionSize; i++)
    {
        AddChildLayout(new UIMenuOption(source, i, options[i]), ELayoutSide::TOP, sizeLayout);
    }

    m_canvas->MarkDirty();
}


//========================================================================================
UIMenuOption::UIMenuOption(UIWidget* source, int index, const std::string& option)
    : UIButton(option.c_str())
    , m_source(source)
    , m_index(index)
    , m_option(option)
{
    m_zDepth = -1;
}


//========================================================================================
void UIMenuOption::Render(RenderPass pass) const
{
    if (pass == RenderPass::BG)
    {
        AABB2 box = m_viewBox;
        AddVertsForAABB2D(g_vertsBuffer, box, m_focused ? Rgba8::YELLOW : m_hovered ? Rgba8(0, 128, 255) : Rgba8::BLACK);
        box.SetDimensions(box.GetDimensions() - Vec2(5, 5));
        AddVertsForAABB2D(g_vertsBuffer, box, Rgba8(80, 80, 80));
    }
    else if (pass == RenderPass::SYMBOL)
    {
        AABB2 box = m_viewBox;
        box.SetDimensions(box.GetDimensions() * Vec2(0.9f, 0.9f));
        g_uiFont->AddVertsForTextInBox2D(g_vertsBuffer, box, 15.0f, m_name, Rgba8::WHITE, 0.666f, Vec2(0.f, 0.5f));
    }

    for (auto* child : m_chidren)
        child->Render(pass);
}


//========================================================================================
void UIMenuOption::OnClick()
{
    g_theConsole->AddLine(DevConsole::LOG_WARN, std::string("Menu clicked: ") + m_option);

    // canvas close will delete this!!!
    int index = m_index;
    std::string option = m_option;
    UIWidget* source = m_source;
    
    m_canvas->CloseMenu();
    source->OnQuickMenu(index, option);
}


//========================================================================================
UIMenuButton::UIMenuButton(const char* name)
    : UIButton(name)
{
}


//========================================================================================
void UIMenuButton::OnClick()
{
    float maxWidth = m_viewBox.GetDimensions().x;

    for (int i = 0; i < m_options.size(); i++)
    {
        float width = g_uiFont->GetTextWidth(20.0f, m_options[i], 0.666f) * 1.1f;
        if (width > maxWidth)
            maxWidth = width;
    }

    AABB2 box = m_viewBox;
    m_canvas->OpenMenu(m_owner, box.GetPointAtUV(Vec2(0, 0)), Vec2(maxWidth, box.GetDimensions().y), m_options);
}


//========================================================================================
void UIMenuButton::OnLoseFocus()
{
    UIWidget::OnLoseFocus();
}

