#include "Game/UI/UIComponents.hpp"

#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/RgbaF.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"

#include "Game/Framework/GameCommon.hpp"
#include "Game/UI/UICanvas.hpp"

bool UIInputCheckbox::OnClick(const Vec2& pos, char keycode)
{
    return false;
}

UIInputText::UIInputText(const char* name /*= ""*/)
    : UIWidget(name)
{
    m_caretStopwatch.Start(0.75);

    SetTextMode();
}

void UIInputText::OnGainFocus(const Vec2& mousePos)
{
    if (!m_focused)
    {
        m_focused = true;
        m_caretPosition = m_inputText.size();
    }
}

void UIInputText::OnKeyPressed(char keycode)
{
    if (m_focused && m_inputEnabled)
    {
        // exit
        if (keycode == KEYCODE_ESC)
        {
            OnLoseFocus();
            return;
        }

        int inputTextSize = (int)m_inputText.size();

        // caret move
        if (keycode == KEYCODE_LEFT)
        {
            m_caretPosition = ClampInt(m_caretPosition - 1, 0, inputTextSize);
            m_caretStopwatch.Restart();
            m_caretVisible = true;
            return;
        }
        if (keycode == KEYCODE_RIGHT)
        {
            m_caretPosition = ClampInt(m_caretPosition + 1, 0, inputTextSize);
            m_caretStopwatch.Restart();
            m_caretVisible = true;
            return;
        }
        if (keycode == KEYCODE_HOME)
        {
            m_caretPosition = 0;
            m_caretStopwatch.Restart();
            m_caretVisible = true;
            return;
        }
        if (keycode == KEYCODE_END)
        {
            m_caretPosition = (int)m_inputText.size();
            m_caretStopwatch.Restart();
            m_caretVisible = true;
            return;
        }

        // caret delete
        if (keycode == KEYCODE_DELETE)
        {
            if (inputTextSize > 0 && m_caretPosition < inputTextSize)
            {
                m_inputText.erase(m_caretPosition, 1);
                m_caretStopwatch.Restart();
                m_caretVisible = true;
            }
            return;
        }
        if (keycode == KEYCODE_BACKSPACE)
        {
            if (inputTextSize > 0 && m_caretPosition > 0)
            {
                m_inputText.erase(--m_caretPosition, 1);
                m_caretStopwatch.Restart();
                m_caretVisible = true;
            }
            return;
        }
        if (keycode == KEYCODE_ESC)
        {
            m_inputText = "";
            m_caretPosition = 0;
            m_caretStopwatch.Restart();
            m_caretVisible = true;
            return;
        }

        // execute
        if (keycode == KEYCODE_ENTER)
        {
            OnLoseFocus();
            m_owner->m_focusChild = nullptr;

            if (m_owner->GetOwner())
            {
                m_owner->OnLoseFocus();
                m_owner->GetOwner()->m_focusChild = nullptr;
            }
            return;
        }

        m_caretStopwatch.Restart();
        m_caretVisible = true;
        return;
    }
}

void UIInputText::OnCharInput(char charcode)
{
    if (m_focused && m_inputEnabled)
    {
        if (m_validChars[charcode])
        {
            char c_str[2] = {};
            c_str[0] = charcode;

            std::string newStr = m_inputText;
            newStr.insert(m_caretPosition, &c_str[0]);

            if (m_validateFunc(newStr))
            {
                m_inputText = std::move(newStr);
                m_caretPosition++;
            }
            
            m_caretStopwatch.Restart();
            m_caretVisible = true;
        }
    }
}

bool UIInputText::OnClick(const Vec2& pos, char keycode)
{
    if (keycode == KEYCODE_LEFT_MOUSE)
    {
        if (!m_suggestions.empty())
        {
            float maxWidth = m_viewBox.GetDimensions().x;

            AABB2 box = m_viewBox;
            m_canvas->OpenMenu(this, box.GetPointAtUV(Vec2(0, 0)), Vec2(maxWidth, box.GetDimensions().y), m_suggestions);
        }
        return true;
    }
    return UIWidget::OnClick(pos, keycode);
}

void UIInputText::OnLoseFocus()
{
    if (m_focused)
    {
        OnInputChanged(m_inputText);
        m_focused = false;
    }
}

void UIInputText::OnQuickMenu(int index, const std::string& option)
{
    m_inputText = option;
    OnInputChanged(m_inputText);
    m_caretPosition = m_inputText.size();
    m_caretStopwatch.Restart();
}

void UIInputText::OnInputChanged(const std::string& text)
{
    m_inputText = m_callbackFunc(text);
}

void UIInputText::OnUpdate()
{
    if (m_caretStopwatch.CheckDurationElapsedAndDecrement())
    {
        m_caretVisible = !m_caretVisible;
    }
}

void UIInputText::Render(RenderPass pass) const
{
    float charWidth = g_uiFont->GetTextWidth(m_fontSize, "0", m_fontAspect); // TODO: support mutable char width

    if (pass == RenderPass::BG)
    {
        AABB2 box = m_viewBox;
        AddVertsForAABB2D(g_vertsBuffer, box, m_focused ? Rgba8::YELLOW : m_hovered ? Rgba8(0, 128, 255) : Rgba8::BLACK);
        box.SetDimensions(box.GetDimensions() - Vec2(5, 5));
        AddVertsForAABB2D(g_vertsBuffer, box, Rgba8::BLACK);

        if (m_focused && m_caretVisible)
        {
            AABB2 cbox = m_viewBox;
            cbox.SetDimensions(cbox.GetDimensions() * Vec2(0.9f, 0.8f));
            cbox.SetCenter(cbox.GetPointAtUV(Vec2(0.0f, 0.5f)) + Vec2(charWidth * (m_caretPosition - m_visiblePosition),  0.0f));
            cbox.SetDimensions(Vec2(2.0f, cbox.GetDimensions().y * 0.8f));

            AddVertsForAABB2D(g_vertsBuffer, cbox, Rgba8::WHITE);
        }
    }
    else if (pass == RenderPass::SYMBOL)
    {
        AABB2 box = m_viewBox;
        box.SetDimensions(box.GetDimensions() * Vec2(0.9f, 0.8f));

        int charSize = static_cast<int>(floor(box.GetDimensions().x / charWidth));

        if (m_inputText.size() <= charSize)
        {
            m_visiblePosition = 0;
            g_uiFont->AddVertsForTextInBox2D(g_vertsBuffer, box, m_fontSize, m_inputText, Rgba8::WHITE, m_fontAspect, Vec2(0.0f, 0.5f));
        }
        else
        {
            if (m_caretPosition > m_visiblePosition + charSize)
                m_visiblePosition = m_caretPosition - charSize;
            if (m_caretPosition < m_visiblePosition)
                m_visiblePosition = m_caretPosition;

            if (m_inputText.size() < m_visiblePosition + charSize)
                m_visiblePosition = m_inputText.size() - charSize;

            auto text = m_inputText.substr(m_visiblePosition, charSize);
            g_uiFont->AddVertsForTextInBox2D(g_vertsBuffer, box, m_fontSize, text, Rgba8::WHITE, m_fontAspect, Vec2(0.0f, 0.5f));
        }
    }

    for (auto* child : m_chidren)
        child->Render(pass);
}

void UIInputText::SetTextMode()
{
    for (unsigned char c = 0; ; c++)
    {
        m_validChars[c] = (c >= 32 && c <= 126 && c != '`' && c != '~');

        if (c == 0xFF)
            break;
    }

    m_validateFunc = [](auto str) { return true; };
}

void UIInputText::SetNumberMode()
{
    for (unsigned char c = 0; ; c++)
    {
        m_validChars[c] = ((c >= '0' && c <= '9') || c == '.');

        if (c == 0xFF)
            break;
    }

    m_validateFunc = [](auto str) 
    {
        int cnt = 0;
        for (auto c : str)
        {
            if (c == '.')
            {
                if (cnt > 0)
                    return false;
                else
                    cnt++;
            }
        }

        return true;
    };
}

void UIInputText::SetInput(const std::string& text)
{
    m_inputText = text;
    m_caretPosition = (int) m_inputText.size();
}

void UIInputText::SetInputEnabled(bool enabled)
{
    m_inputEnabled = enabled;
}

void UIInputText::SetSuggestions(const StringList& suggestions)
{
    m_suggestions = suggestions;
}

void UIInputText::SetCallback(Callback callback)
{
    m_callbackFunc = callback;
}

UIButton::UIButton(const char* name) : UILabel(name)
{

}

bool UIButton::OnClick(const Vec2& pos, char keycode)
{
    if (keycode == KEYCODE_LEFT_MOUSE)
        OnClick();
    return true;
}

void UIButton::OnClick()
{
    g_theConsole->AddLine(DevConsole::LOG_WARN, std::string("Button clicked: ") + m_name);
    PostMessage(this, std::string("Btn:") + m_name);
}

UILabel::UILabel(const char* name) : UIWidget(name)
{
}

void UILabel::Render(RenderPass pass) const
{
    if (m_render)
    {
        if (pass == RenderPass::SYMBOL)
        {
            AABB2 box = m_viewBox;
            box.SetDimensions(box.GetDimensions() * Vec2(0.8f, 0.8f));
            auto color = m_enabled ? m_labelColor : Rgba8(160, 160, 160);

            g_uiFont->AddVertsForTextInBox2D(g_vertsBuffer, box, 20.0f, m_label, color, 0.666f);
        }
    }

    RenderChildren(pass);
}

UIPanel::UIPanel(const char* name /*= "Panel"*/, Rgba8 color /*= Rgba8::WHITE*/)
    : UIWidget(name)
{
    m_color = color;
}

UIPanel::UIPanel(Rgba8 color)
    : UIPanel("Panel", color)
{

}

void UIPanel::Render(RenderPass pass) const
{
    if (m_render)
    {
        if (pass == RenderPass::BG)
        {
            if (m_frame)
            {
                RenderChildren(pass);
                RenderPanel(m_viewBox, true);
            }
            else
            {
                AddVertsForAABB2D(g_vertsBuffer, m_viewBox, m_bgColor);
                RenderPanel(m_viewBox, false);
                RenderChildren(pass);
                RenderPanel(m_viewBox, true);
            }
            return;
        }
        else if (pass == RenderPass::SYMBOL && !m_text.empty())
        {
            AABB2 box = m_viewBox;
            box.SetDimensions(box.GetDimensions() * m_fontBoxSize);
            g_uiFont->AddVertsForTextInBox2D(g_vertsBuffer, box, m_fontSize, m_text, Rgba8::WHITE, m_fontAspect, m_fontAlign, m_overrun ? TextDrawMode::OVERRUN: TextDrawMode::SHRINK_TO_FIT);
        }
    }

    RenderChildren(pass);
}

void UIPanel::RenderPanel(const AABB2& box, bool frame /*= false*/) const
{
    g_theRenderer->DrawVertexArray(g_vertsBuffer);
    g_vertsBuffer.clear();

    if (frame)
    {
        AddVertsForUIFrame(g_vertsBuffer, box, m_color, m_cornerWidth * box.GetDimensions().x, m_textureUVs, m_cornerUVWidth);
    }
    else
    {
        AddVertsForUIBox(g_vertsBuffer, box, m_color, m_cornerWidth * box.GetDimensions().x, m_textureUVs, m_cornerUVWidth);
    }
    g_theRenderer->SetSamplerMode(SamplerMode::BILINEARCLAMP);
    g_theRenderer->SetBlendMode(BlendMode::ALPHA);
    g_theRenderer->BindTexture(m_texture);
    g_theRenderer->DrawVertexArray(g_vertsBuffer);
    g_theRenderer->SetBlendMode(BlendMode::ALPHA);
    g_theRenderer->SetSamplerMode(SamplerMode::BILINEARCLAMP);
    g_vertsBuffer.clear();
    g_theRenderer->BindTexture(nullptr);
}

UIText::UIText(const std::string& text)
    : UIText("Text", text)
{

}

UIText::UIText(const char* name, const std::string& text)
    : UIWidget(name)
    , m_text(text)
{

}

void UIText::Render(RenderPass pass) const
{
    if (pass == RenderPass::SYMBOL && !m_text.empty())
    {
        AABB2 box = m_viewBox;
        box.SetDimensions(box.GetDimensions() * m_area);
        g_uiFont->AddVertsForTextInBox2D(g_vertsBuffer, box, m_fontSize, m_text, m_color, m_fontAspect, m_fontAlign, m_overrun ? TextDrawMode::OVERRUN : TextDrawMode::SHRINK_TO_FIT);
    }

    for (auto* child : m_chidren)
        child->Render(pass);
}
