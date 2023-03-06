#pragma once

#include "Game/UI/UICommons.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Rgba8.hpp"

#include <map>

extern bool UI_RENDER_DEBUG_LABEL;

class Texture;
class UICanvas;

class UIWidget
{
protected:
    using WidgetList = std::vector<UIWidget*>;

public:
    using MessageHandle = std::function<bool(UIWidget*, const std::string&)>;
    using Dispatcher = std::map<std::string, MessageHandle>;

public:
    enum class RenderPass
    {
        BG,
        SYMBOL,
        OVERLAY,
        SIZE,
    };

public:
    static Vec2 GetMousePos();

protected:
    UIWidget(const char* name = "Widget");
    virtual ~UIWidget();

public:
    inline UIWidget*                GetOwner() const { return m_owner; }
    inline const std::string        GetName() const { return m_name; }
    inline UIWidget*                GetFocusChild() const { return m_focusChild; }

    virtual void                    OnInit();
    virtual void                    OnUpdate();
    virtual void                    OnUpdateRender();
    virtual void                    OnDispose();
    virtual void                    OnChildAdded(UIWidget* child);

    virtual bool                    OnMouseClick(const Vec2& pos, char keycode); // return if click is handled
    virtual bool                    OnClick(const Vec2& pos, char keycode); // return if click is handled
    virtual void                    OnHoverBegin(const Vec2& pos);
    virtual void                    OnHoverUpdate(const Vec2& pos, bool recal = false);
    virtual void                    OnHoverEnd(const Vec2& pos);
    virtual bool                    OnMouseDragBegin(const Vec2& pos, char keycode); // return allow drag
    virtual void                    OnMouseDragFinish(const Vec2& pos, char keycode);
    virtual bool                    AcceptDraggableWidget(UIWidget* source, const Vec2& pos, char keycode); // return accept drag
    virtual void                    OnKeyPressed(char keycode);
    virtual void                    OnCharInput(char charcode);
    virtual void                    OnGainFocus(const Vec2& mousePos);
    virtual void                    OnLoseFocus();
    virtual void                    OnQuickMenu(int index, const std::string& option);

    virtual void                    Render(RenderPass pass) const;

    virtual bool                    PostMessage(UIWidget* source, const std::string& message);

    void                            RenderChildren(RenderPass pass) const;

    void                            AddChild(UIWidget* child);
    void                            AddChildComponent(UIWidget* child, const Vec2& relPos, const Vec2& relSize);
    void                            AddChildComponent(UIWidget* child, const Vec2& relPos, const Vec2& relSize, const Vec2& align);
    void                            AddChildLayout(UIWidget* child, ELayoutSide side, float size = -1.0f);
    void                            RemoveChild(UIWidget* child);
    void                            DeleteChild(UIWidget* child);
    void                            RemoveAllChildren();
    void                            ClearAllChildren(); // will delete them
    UIWidget*                       FindChild(const char* name) const;

    void                            ForEachChild_BFS(std::function<void(UIWidget*)> action);
    void                            ForEachChild_DFS(std::function<void(UIWidget*)> action);


    void                            SetViewport(const AABB2& viewport);
    AABB2                           GetViewBox(const AABB2& parentView) const;
    AABB2                           GetBox() const;
    void                            SetBox(const AABB2& box, Vec2 align = Vec2(0.5f, 0.5f));
    void                            RecalBox(bool recursive = true);

private:

protected:
    UIWidget*                       m_owner = nullptr;
    Rect                            m_rect;
    WidgetList                      m_chidren;
                                    
public:                             
    std::string                     m_name;
    float                           m_zDepth = 0;
    AABB2                           m_viewBox = AABB2::ZERO_TO_ONE;
    AABB2                           m_layoutBox = AABB2::ZERO_TO_ONE;

    UICanvas*                       m_canvas = nullptr;

    bool                            m_render = true;
    bool                            m_enabled = true;
    bool                            m_hovered = false;
    bool                            m_focused = false;

    UIWidget*                       m_hoverChild = nullptr;
    UIWidget*                       m_focusChild = nullptr;    

    Rgba8                           m_color = Rgba8(80,80,80);
};

