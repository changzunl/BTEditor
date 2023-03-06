#pragma once

#include "Engine/Core/Stopwatch.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Game/UI/UIWidget.hpp"
#include <bitset>
#include <climits>

using StringList = std::vector<std::string>;
class Texture;


class UILabel : public UIWidget
{
public:
    UILabel(const char* name = "Label");

    virtual void Render(RenderPass pass) const override;

public:
    Rgba8       m_labelColor = Rgba8::WHITE;
    std::string m_label;
};


class UIPanel : public UIWidget
{
public:
    UIPanel(const char* name = "Panel", Rgba8 color = Rgba8::WHITE);
    UIPanel(Rgba8 color);

    virtual void Render(RenderPass pass) const override;
    void RenderPanel(const AABB2& box, bool frame = false) const;

public:
    Texture*    m_texture = nullptr;
    AABB2       m_textureUVs = AABB2::ZERO_TO_ONE;
    Rgba8       m_bgColor = Rgba8::BLACK;
    float       m_cornerWidth = 0.1f;
    float       m_cornerUVWidth = 0.1f;
    bool        m_frame = false;

    std::string m_text;
    float       m_fontSize = 20.0f;
    float       m_fontAspect = 0.666f;
    Vec2        m_fontAlign = Vec2(0.5f, 0.5f);
    Vec2        m_fontBoxSize = Vec2(0.8f, 0.8f);
    bool        m_overrun = false;
};


class UIText : public UIWidget
{
public:
    UIText(const std::string& text);
    UIText(const char* name, const std::string& text);

    virtual void Render(RenderPass pass) const override;

public:
    std::string m_text;
    Vec2 m_area = Vec2(0.8f, 0.8f);
    float m_fontSize = 20.0f;
    float m_fontAspect = 0.666f;
    Vec2 m_fontAlign = Vec2(0.5f, 0.5f);
    bool m_overrun = true;
};


class UIButton : public UILabel
{
public:
    UIButton(const char* name = "Button");

    virtual bool OnClick(const Vec2& pos, char keycode) override;

    virtual void OnClick();
    
};


class UIInputCheckbox : public UIWidget
{
public:
    virtual bool OnClick(const Vec2& pos, char keycode) override;
    virtual void OnChangeState(bool state) = 0;

    inline bool GetState() const { return m_state; }

private:
    bool m_state = false;
};


class UIInputText : public UIWidget
{
public:
    using Validator = std::function<bool(const std::string&)>;
    using CharSet = std::bitset<UCHAR_MAX + 1>;
    using Callback = std::function<std::string(const std::string&)>;

public:
    UIInputText(const char* name = "InputText");

    virtual void OnGainFocus(const Vec2& mousePos) override;
    virtual void OnKeyPressed(char keycode) override;
    virtual void OnCharInput(char charcode) override;
    virtual bool OnClick(const Vec2& pos, char keycode) override;
    virtual void OnLoseFocus() override;
    virtual void OnQuickMenu(int index, const std::string& option) override;

    virtual void OnInputChanged(const std::string& text);

    virtual void OnUpdate() override;
    virtual void Render(RenderPass pass) const override;

    inline const std::string& GetInput() const { return m_inputText; }

    void SetTextMode();
    void SetNumberMode();

    void SetInput(const std::string& text);
    void SetInputEnabled(bool enabled);
    void SetSuggestions(const StringList& suggestions);
    void SetCallback(Callback callback);

public:
    float                          m_fontSize = 20.0f;
    float                          m_fontAspect = 0.666f;
    bool                           m_overrun = true;

private:
    std::string                    m_inputText;
    int                            m_caretPosition = 0;
    mutable int                    m_visiblePosition = 0;
    bool                           m_caretVisible = true;
    Stopwatch                      m_caretStopwatch;

    bool                           m_inputEnabled = true;
    Validator                      m_validateFunc = [](auto) { return true; };
    CharSet                        m_validChars;
    StringList                     m_suggestions;
    Callback                       m_callbackFunc = [](auto str) { return str; };
};

