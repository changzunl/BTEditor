#pragma once

#include "Game/UI/UIWidget.hpp"
#include "Game/UI/UIComponents.hpp"

#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Stopwatch.hpp"


class UIQuickMenu;
class UICanvas;

class UIMenuOption : public UIButton
{
    friend class UIQuickMenu;

private:
    UIMenuOption(UIWidget* source, int index, const std::string& option);

    virtual void Render(RenderPass pass) const override;

public:
    virtual void OnClick() override;

private:
    UIWidget* const m_source;
    const int m_index;
    const std::string m_option;
};

class UIQuickMenu : public UIWidget
{
    friend class UICanvas;

public:
    UIQuickMenu(UICanvas* canvas);

    virtual void Render(RenderPass pass) const override;

    void SetContent(UIWidget* source, const Vec2& pos, const Vec2& size, std::vector<std::string> options);

private:
    UIWidget*  m_source = nullptr;
    StringList m_options;
    bool       m_opened = false;
};

class UIMenuButton : public UIButton
{

public:
    UIMenuButton(const char* name);

    virtual void OnClick() override;
    virtual void OnLoseFocus() override;

public:
    StringList m_options;
};


class UICanvas : public UIWidget
{
public:
    UICanvas(const char* name = "Canvas", AABB2 box = AABB2::ZERO_TO_ONE);
    virtual ~UICanvas();

    void Active();
    void Deactive();

    inline void SetDragWidget(UIWidget* widget) { m_dragWidget = widget; }
    inline UIWidget* GetDragWidget() const { return m_dragWidget; }

    virtual void OnUpdate() override;
    virtual bool PostMessage(UIWidget* source, const std::string& message) override;
    virtual bool OnMouseClick(const Vec2& pos, char keycode) override;

    virtual void Render() const;

    UIWidget* GetFocusChild() const;

    void AddHandle(const std::string& message, MessageHandle handle);
    void OpenMenu(UIWidget* source, const Vec2& pos, const Vec2& size, const std::vector<std::string>& options);
    void CloseMenu();
    void SetFocus(UIWidget* widget);
    void SetInputEnabled(bool enabled);
    bool IsInputEnabled() const;

    bool ProcessRawButtonDown(EventArgs& args);
    bool ProcessRawButtonUp(EventArgs& args);
    bool ProcessRawInput(EventArgs& args);

    void MarkDirty();

public:
    bool m_active = false;

private:
    UIQuickMenu* const     m_quickMenu;
    Dispatcher             m_dispatcher;
    EventHandler           m_handlers[3] = {};
    Vec2                   m_dragPos[2] = {};
    Stopwatch              m_dragWatch[2] = {};
    bool                   m_dragging[2] = {};
    Vec2                   m_mousePos;
    UIWidget*              m_dragWidget = nullptr;
    bool                   m_memberDirty = false;
    bool                   m_disableInput = false;
};
