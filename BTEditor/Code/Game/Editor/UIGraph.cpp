#include "Game/Editor/UIGraph.hpp"

#include "Game/UI/UICanvas.hpp"

#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/ByteBuffer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/Curves.hpp"

#include "Game/Framework/Game.hpp"
#include "Game/Entity/AI.hpp"

#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Window/Dialog.hpp"
#include "Engine/Window/Window.hpp"


//========================================================================================
UIGraph::UIGraph(UIEditor* editor)
    : UIWidget("BTGraph")
    , m_editor(editor)
    , m_board(new DataRegistry())
    , m_context(new BTContext(*m_board))
{
    m_context->m_root->m_position.x = 0.5f;
    m_context->m_root->m_position.y = 0.9f;
}


//========================================================================================
void UIGraph::OnUpdate()
{
    if (m_updateField)
    {
        m_updateField = false;

        FieldList fields;
        m_board->CollectProps(fields);

        fields.emplace_back();
        fields.back().name = "Operation";
        fields.back().type = FieldType::ENUM;
        fields.back().defaults = { "ADD NEW ENTRY" };
        fields.back().value = "";
        fields.back().callback = [&](auto value)
        {
            if (value == "ADD NEW ENTRY")
            {
                AddBoardEntry();
            }
            return "";
        };
        m_editor->m_properties->SetFields(fields);
        m_editor->m_properties->SetFocusDataTable();
        m_propFocused = true;
    }

    if (m_dragging || m_selectNode)
    {
        m_dragOrigin += m_dragOffset;
        m_dragOffset = m_canvas->GetMousePos() - m_dragOrigin;
    }
    else
    if (m_selecting)
    {
        m_selectArea[1] = m_canvas->GetMousePos();
    }
    else
    if (g_theInput->IsKeyDown(KEYCODE_LCTRL))
    {
        int scroll = g_theInput->GetMouseWheel();

        if (scroll)
        {
            auto pos = GetMousePos();

            for (auto& pair : m_nodeUIMap)
            {
                auto* node = pair.second;

                float scale = scroll > 0 ? 1.1f : (1 / 1.1f);
                Vec2 center = node->m_viewBox.GetCenter();
                center = (center - pos) * scale + pos;
                node->m_viewBox.SetCenter(center);
                node->RecalBox(false);
                node->SetViewport(node->GetOwner()->m_viewBox);
            }
        }
    }

    if (!m_selected.empty())
    {
        if (m_focusChild)
            if (std::find(m_selected.begin(), m_selected.end(), m_focusChild) == m_selected.end())
                m_selected.clear();
    }

    UIWidget::OnUpdate();
}


//========================================================================================
void UIGraph::AddNode(BTNode* btnode, BTNode* parent /*= nullptr*/)
{
    UINode* node = new UINode(this, btnode);
    AddChildComponent(node, btnode->m_position, Vec2(0.15f, 0.08f));
    btnode->m_position = m_viewBox.GetPointAtUV(btnode->m_position);
    node->Refresh();
    m_context->RefreshOrders();
    m_nodeUIMap[btnode] = node;
}


//========================================================================================
void UIGraph::InitNode(BTNode* btnode, BTNode* parent /*= nullptr*/)
{
    auto name = std::string(typeid(*btnode).name()).substr(std::string("class BTNode").size());
    btnode->m_name = name;

    m_context->AddNode(btnode, parent);

    AddNode(btnode, parent);
}


//========================================================================================
bool UIGraph::OnClick(const Vec2& pos, char keycode)
{
    m_selected.clear();

    if (keycode == KEYCODE_RIGHT_MOUSE)
    {
        m_menuMousePos = pos;
        static const StringList options = 
        {
            "Add sequence", 
            "Add select", 
            "Add task wait", 
            "Add task play sound",  
            "Add task fire event", 
            "Add task attack", 
            "Add task move to",
            "Add task set value",
            "Add task random point",
            "Add task keep distance",
            "Add task",
        };

        m_canvas->OpenMenu(this, pos, m_canvas->m_viewBox.GetDimensions() * Vec2(0.15f, 0.025f), options);
        return true;
    }
    if (keycode == KEYCODE_LEFT_MOUSE)
    {
        if (!m_propFocused)
            m_updateField = true;
        return !m_propFocused;
    }

    return false;
}


//========================================================================================
void UIGraph::OnGainFocus(const Vec2& mousePos)
{
    if (!m_focused)
    {
        UIWidget::OnGainFocus(mousePos);

        UINode* node = dynamic_cast<UINode*>(m_focusChild);

        if (!node)
        {
            FieldList fields;
            m_board->CollectProps(fields);

            fields.emplace_back();
            fields.back().name = "Operation";
            fields.back().type = FieldType::ENUM;
            fields.back().defaults = {"ADD NEW ENTRY"};
            fields.back().value = "";
            fields.back().callback = [&](auto value)
            {
                if (value == "ADD NEW ENTRY")
                {
                    AddBoardEntry();
                }
                return "";
            };
            m_editor->m_properties->SetFields(fields);
            m_editor->m_properties->SetFocusDataTable();

            m_propFocused = true;
        }
    }
}


//========================================================================================
void UIGraph::OnLoseFocus()
{
    m_selected.clear();

    UIWidget::OnLoseFocus();
}


//========================================================================================
void UIGraph::OnQuickMenu(int index, const std::string&)
{
    switch (index)
    {
    case 0: // add seq
    {
        PushChanges();

        BTNodeCompSequence* seq = new BTNodeCompSequence();
        seq->m_position = m_viewBox.GetUVForPoint(m_menuMousePos);
        InitNode(seq);
        break;
    }
    case 1: // add sel
    {
        PushChanges();

        BTNodeCompSelect* sel = new BTNodeCompSelect();
        sel->m_position = m_viewBox.GetUVForPoint(m_menuMousePos);
        InitNode(sel);
        break;
    }
    case 2: // add task wait
    {
        PushChanges();

        BTNodeTask* task = new BTNodeTaskWait();
        task->m_position = m_viewBox.GetUVForPoint(m_menuMousePos);
        InitNode(task);
        break;
    }
    case 3: // add task wait
    {
        PushChanges();

        BTNodeTask* task = new BTNodeTaskPlaySound();
        task->m_position = m_viewBox.GetUVForPoint(m_menuMousePos);
        InitNode(task);
        break;
    }
    case 4: // add task wait
    {
        PushChanges();

        BTNodeTask* task = new BTNodeTaskFireEvent();
        task->m_position = m_viewBox.GetUVForPoint(m_menuMousePos);
        InitNode(task);
        break;
    }
    case 5: // add task
    {
        PushChanges();

        BTNodeTask* task = new BTNodeTaskAttack();
        task->m_position = m_viewBox.GetUVForPoint(m_menuMousePos);
        InitNode(task);
        break;
    }
    case 6: // add task
    {
        PushChanges();

        BTNodeTask* task = new BTNodeTaskMoveTo();
        task->m_position = m_viewBox.GetUVForPoint(m_menuMousePos);
        InitNode(task);
        break;
    }
    case 7: // add task wait
    {
        PushChanges();

        BTNodeTask* task = new BTNodeTaskSetValue();
        task->m_position = m_viewBox.GetUVForPoint(m_menuMousePos);
        InitNode(task);
        break;
    }
    case 8: // add task wait
    {
        PushChanges();

        BTNodeTask* task = new BTNodeTaskRandomPoint();
        task->m_position = m_viewBox.GetUVForPoint(m_menuMousePos);
        InitNode(task);
        break;
    }
    case 9: // add task wait
    {
        PushChanges();

        BTNodeTask* task = new BTNodeTaskKeepDistance();
        task->m_position = m_viewBox.GetUVForPoint(m_menuMousePos);
        InitNode(task);
        break;
    }
    case 10: // add task
    {
        PushChanges();

        BTNodeTask* task = new BTNodeTaskDummy();
        task->m_position = m_viewBox.GetUVForPoint(m_menuMousePos);
        InitNode(task);
        break;
    }
    default:
        break;
    }
}


//========================================================================================
bool UIGraph::OnMouseDragBegin(const Vec2& pos, char keycode)
{
    UINode* node = dynamic_cast<UINode*>(m_focusChild);
    if (!IsSelected(node))
        m_selected.clear();

    if (m_focusChild)
        return m_focusChild->OnMouseDragBegin(pos, keycode);

    if (keycode == KEYCODE_RIGHT_MOUSE && m_focused)
    {
        if (m_hoverChild)
        {
        }
        else
        {
            m_canvas->SetDragWidget(this);
            m_dragging = true;
            m_dragOrigin = pos;
            m_dragOffset = m_canvas->GetMousePos() - pos;
            return true;
        }
    }
    else if (keycode == KEYCODE_LEFT_MOUSE && m_focused)
    {
        if (m_hoverChild)
        {
        }
        else
        {
            m_canvas->SetDragWidget(this);
            m_selecting = true;
            m_selectArea[0] = pos;
            return true;
        }
    }
    return false;
}


//========================================================================================
void UIGraph::OnMouseDragFinish(const Vec2& pos, char keycode)
{
    if (m_dragging && keycode == KEYCODE_RIGHT_MOUSE)
    {
        m_dragging = false;
        m_canvas->SetDragWidget(nullptr);
    }
    if (m_selecting && keycode == KEYCODE_LEFT_MOUSE)
    {
        m_selecting = false;
        m_canvas->SetDragWidget(nullptr);
        AABB2 box;
        box = AABB2(m_selectArea[0], m_selectArea[0]);
        box.StretchToIncludePoint(m_selectArea[1]);

        for (auto& pair : m_nodeUIMap)
        {
            if (pair.second->m_viewBox.HasIntersection(box))
            {
                m_selected.push_back(pair.second);
            }
        }
    }
}


//========================================================================================
void UIGraph::Render(RenderPass pass) const
{
    static Texture* texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/UI/grey3_panel.png");

    if (m_render)
    {
        if (pass == RenderPass::BG)
        {
            AddVertsForAABB2D(g_vertsBuffer, m_viewBox, Rgba8::BLACK);
            g_theRenderer->DrawVertexArray(g_vertsBuffer);
            g_vertsBuffer.clear();

            AABB2 box = m_viewBox;
            float percent = 0.0025f * 1.176f;
            AddVertsForUIBox(g_vertsBuffer, m_viewBox, Rgba8(100, 100, 100), box.GetDimensions().x * percent, AABB2::ZERO_TO_ONE, 0.07f);
            g_theRenderer->SetSamplerMode(SamplerMode::BILINEARCLAMP);
            g_theRenderer->SetBlendMode(BlendMode::ALPHA);
            g_theRenderer->BindTexture(texture);
            g_theRenderer->DrawVertexArray(g_vertsBuffer);
            g_theRenderer->SetBlendMode(BlendMode::ALPHA);
            g_theRenderer->SetSamplerMode(SamplerMode::BILINEARCLAMP);
            g_vertsBuffer.clear();
            g_theRenderer->BindTexture(nullptr);
        }
        else if (pass == RenderPass::SYMBOL)
        {
            AABB2 box = m_viewBox;
            g_uiFont->AddVertsForTextInBox2D(g_vertsBuffer, box, 40.0f, "BT Graph", Rgba8(160, 160, 160, 160), 0.666f, Vec2(0.02f, 0.98f));
 
//             box.SetDimensions(box.GetDimensions() * Vec2(0.9f, 0.9f));
//             g_uiFont->AddVertsForTextInBox2D(g_vertsBuffer, box, 20.0f, m_name, m_enabled ? Rgba8::WHITE : Rgba8(160, 160, 160), 0.666f);
        }
    }

    // enable stencil area
    g_theRenderer->DrawVertexArray(g_vertsBuffer);
    g_theRenderer->ClearStencil(0xFF);
    g_vertsBuffer.clear();

    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->GetDepthStencilState().SetDepthTest(DepthTest::NEVER);
    g_theRenderer->GetDepthStencilState().SetStencilWrite(true);

    AABB2 box = m_viewBox;
    box.SetDimensions(box.GetDimensions() - 15);
    AddVertsForAABB2D(g_vertsBuffer, box, Rgba8::WHITE);
    g_theRenderer->DrawVertexArray(g_vertsBuffer);
    g_vertsBuffer.clear();

    g_theRenderer->GetDepthStencilState().SetStencilWrite(false);
    g_theRenderer->GetDepthStencilState().SetStencilTest(true);
    g_theRenderer->GetDepthStencilState().SetDepthTest(DepthTest::LESSEQUAL);

    if (pass == RenderPass::SYMBOL)
        g_theRenderer->BindTexture(&g_uiFont->GetTexture());

    for (auto* child : m_chidren)
        child->Render(pass);

    g_theRenderer->DrawVertexArray(g_vertsBuffer);
    g_vertsBuffer.clear();
    g_theRenderer->GetDepthStencilState().SetStencilTest(false);

    if (pass == RenderPass::BG && m_selecting)
    {
        box = AABB2(m_selectArea[0], m_selectArea[0]);
        box.StretchToIncludePoint(m_selectArea[1]);
        AddVertsForAABB2D(g_vertsBuffer, box, Rgba8(127,187,255,80));
        AddVertsForUIFrame(g_vertsBuffer, box, Rgba8(127, 187,255,80), m_viewBox.GetDimensions().x * 0.002f);

        for (auto& pair : m_nodeUIMap)
        {
            auto* node = pair.second;

            AABB2 viewBox = AABB2(node->m_viewBox.m_mins, node->m_viewBox.m_mins);
            viewBox.StretchToIncludePoint(node->m_viewBox.m_maxs);
            if (viewBox.HasIntersection(box))
                AddVertsForAABB2D(g_vertsBuffer, viewBox, Rgba8(255, 187, 127, 80));
        }
    }
}


//========================================================================================
void UIGraph::LoadBTContext(BTContext* context)
{
    UNUSED(context);
    // TODO load
}


//========================================================================================
UINode::UINode(UIGraph* graph, BTNode* node) 
    : UIWidget("")
    , m_graph(graph)
    , m_node(node)
    , m_inputs(new UIPanel("NodeInput"))
    , m_outputs(new UIPanel("NodeOutput"))
    , m_info(new UIPanel("NodeInfo"))
{
    m_inputs->m_enabled = false;
    m_outputs->m_enabled = false;
    m_info->m_enabled = false;

    m_inputs->m_color  = Rgba8(80, 80, 220);
    m_outputs->m_color = Rgba8(80, 80, 220);
    m_info->m_color    = Rgba8(160, 160, 160);

    Refresh();
}


//========================================================================================
BTNode* UINode::GetBTNode() const
{
    return m_node;
}


//========================================================================================
void UINode::OnUpdate()
{
    if (m_dragging)
    {
        m_viewBox.SetCenter(m_dragOffset + m_canvas->GetMousePos());
        RecalBox(false);
        SetViewport(m_owner->m_viewBox);
    }
    if (m_graph->IsDragging(this))
    {
        m_viewBox.Translate(m_graph->m_dragOffset);
        RecalBox(false);
        SetViewport(m_owner->m_viewBox);
    }

    m_node->m_position = m_viewBox.GetCenter();
    m_info->m_text = m_node->m_name;

    UIWidget::OnUpdate();

    m_nodeColor = Rgba8(180, 180, 180);
    if (m_graph->m_debugBT)
    {
        auto* debugNode = m_graph->m_debugBT->FindNode(m_node->m_uuid);

        if (debugNode)
        {
            if (debugNode->IsExecuting())
            {
                auto* nodeTask = dynamic_cast<BTNodeTask*>(debugNode);

                m_nodeColor = nodeTask ? Rgba8(200, 200, 0) : Rgba8(240, 140, 0);
            }
            else if (debugNode->IsAborted())
                m_nodeColor = Rgba8(180, 0, 0);
            else if (debugNode->IsSuccess())
                m_nodeColor = Rgba8(0, 180, 0);
            else if (debugNode->IsFailed())
                m_nodeColor = Rgba8(240, 0, 0);
        }
        else
        {
            DebuggerPrintf("Node not found (debug): %s(%s)", m_node->GetName().c_str(), m_node->GetRegistryName());
        }
    }

    if (m_graph->IsSelected(this))
        m_nodeColor.g = 255;

    m_info->m_color = m_nodeColor;
}


//========================================================================================
void UINode::Render(RenderPass pass) const
{
    if (m_render)
    {
        if (pass == RenderPass::BG)
        {
            g_theRenderer->DrawVertexArray(g_vertsBuffer);
            g_vertsBuffer.clear();

            AABB2 box = m_viewBox;
            box.SetDimensions(box.GetDimensions() + box.GetDimensions().x * 0.04f);
            AddVertsForUIBox(g_vertsBuffer, box, m_focused || m_hovered ? Rgba8::WHITE : Rgba8(127,127,127), 15.0f, AABB2::ZERO_TO_ONE, 0.08f);
            const char* image = m_focused ? "Data/Images/UI/red_panel.png" : m_hovered ? "Data/Images/UI/blue_panel.png" : "Data/Images/UI/grey4_panel.png";
            auto* texture = g_theRenderer->CreateOrGetTextureFromFile(image);
            g_theRenderer->BindTexture(texture);
            g_theRenderer->SetSamplerMode(SamplerMode::BILINEARCLAMP);
            g_theRenderer->SetBlendMode(BlendMode::ALPHA);
            g_theRenderer->DrawVertexArray(g_vertsBuffer);
            g_vertsBuffer.clear();
            g_theRenderer->SetBlendMode(BlendMode::ALPHA);
            g_theRenderer->SetSamplerMode(SamplerMode::BILINEARCLAMP);
            g_theRenderer->BindTexture(nullptr);
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

    for (auto* child : m_chidren)
        child->Render(pass);

    if (m_render)
    {
        if (pass == RenderPass::SYMBOL)
        {
            AABB2 box = m_info->m_viewBox.GetSubBox(AABB2(0.9f,0.5f,1.0f,0.9f));
            auto color = Rgba8::WHITE;
            color.a = 255;
            g_uiFont->AddVertsForTextInBox2D(g_vertsBuffer, box, 12.0f, Stringf("%d", m_node->m_order), color, 0.666f, Vec2(0.5f,0.5f), TextDrawMode::OVERRUN);
        }

        if (pass == RenderPass::OVERLAY)
        {
            g_theRenderer->DrawVertexArray(g_vertsBuffer);
            g_vertsBuffer.clear();

            g_theRenderer->SetSamplerMode(SamplerMode::BILINEARCLAMP);
            g_theRenderer->SetBlendMode(BlendMode::ALPHA);
            AABB2 icon = m_info->m_viewBox;
            icon.m_maxs.x = icon.m_mins.x + icon.GetDimensions().y;
            icon.SetDimensions(icon.GetDimensions() * 0.8f);
            AddVertsForAABB2D(g_vertsBuffer, icon, Rgba8::WHITE);

            static auto imagePaths = {
                "Data/Images/Node/rhombus_question.png",
                "Data/Images/Node/crown_b.png",
                "Data/Images/Node/sword.png",
                "Data/Images/Node/arrow_sequence.png",
                "Data/Images/Node/arrow_reserve.png",
            };
            static auto images = {
                 g_theRenderer->CreateOrGetTextureFromFile(imagePaths.begin()[0]),
                 g_theRenderer->CreateOrGetTextureFromFile(imagePaths.begin()[1]),
                 g_theRenderer->CreateOrGetTextureFromFile(imagePaths.begin()[2]),
                 g_theRenderer->CreateOrGetTextureFromFile(imagePaths.begin()[3]),
                 g_theRenderer->CreateOrGetTextureFromFile(imagePaths.begin()[4]),
            };
            std::string regName = m_node->GetRegistryName();
            auto* texture = images.begin()[0];
            if (regName.rfind("Root", 0) == 0)
            {
                texture = images.begin()[1];
            }
            else if (regName.rfind("Task", 0) == 0)
            {
                texture = images.begin()[2];
            }
            else if (regName.rfind("CompSequence", 0) == 0)
            {
                texture = images.begin()[3];
            }
            else if (regName.rfind("CompSelect", 0) == 0)
            {
                texture = images.begin()[4];
            }
            g_theRenderer->BindTexture(texture);
            g_theRenderer->DrawVertexArray(g_vertsBuffer);
            g_theRenderer->SetBlendMode(BlendMode::ALPHA);
            g_theRenderer->SetSamplerMode(SamplerMode::BILINEARCLAMP);
            g_vertsBuffer.clear();
            g_theRenderer->BindTexture(nullptr);

            Vec2 from = m_outputs->m_viewBox.GetCenter();
            m_node->ForChildNode([&, from](auto child)
                {
                    UINode* toNode = m_graph->m_nodeUIMap[child];
                    Vec2 to = toNode->m_inputs->m_viewBox.GetCenter();

                    float vel = 250.f * m_graph->m_context->m_lod;

                    CubicHermiteCurve2D curve(from, Vec2(0, -vel), to, Vec2(0, -vel));

                    AddVertsForHermite2D(g_vertsBuffer, curve, 2.f, Rgba8::WHITE);
                    AddVertsForArrow2D(g_vertsBuffer, to + Vec2(0.f, 1.f), to, 10.0f, 2.0f, Rgba8::WHITE);
                });

            if (m_canvas->GetDragWidget() == this && !m_dragging)
            {
                Vec2 to = m_canvas->GetMousePos();

                float vel = 250.f * m_graph->m_context->m_lod;

                CubicHermiteCurve2D curve(from, Vec2(0, -vel), to, Vec2(0, -vel));

                AddVertsForHermite2D(g_vertsBuffer, curve, 2.f, Rgba8::WHITE);
                AddVertsForArrow2D(g_vertsBuffer, to + Vec2(0.f, 1.f), to, 10.0f, 2.0f, Rgba8::WHITE);
            }
        }
    }
}

//========================================================================================
void UINode::OnGainFocus(const Vec2& mousePos)
{
    if (!m_focused)
    {
        UIWidget::OnGainFocus(mousePos);

        if (!dynamic_cast<UINodeDecorator*>(m_focusChild))
        {
            FieldList fields;
            m_node->CollectProps(fields);
            m_graph->m_editor->m_properties->SetFields(fields);
            m_graph->m_editor->m_properties->SetFocus(ComponentType::NODE, m_node->m_uuid);
            m_propFocused = true;
        }

        m_graph->m_propFocused = false;
    }
}


//========================================================================================
bool UINode::OnClick(const Vec2& pos, char keycode)
{
    if (keycode == KEYCODE_RIGHT_MOUSE)
    {
        static const StringList options =
        {
            "Add Deco Dummy", 
            "Add Deco Cooldown", 
            "Add Deco Watch", 
            "Add Deco CanSee", 
            "Add Deco IsInRange", 
            "Break Link to Parent", 
            "Delete",
        };
        m_canvas->OpenMenu(this, pos, m_canvas->m_viewBox.GetDimensions() * Vec2(0.1f, 0.015f), options);
        return true;
    }
    if (keycode == KEYCODE_LEFT_MOUSE)
    {
        if (!m_propFocused && !dynamic_cast<UINodeDecorator*>(m_hoverChild))
        {
            FieldList fields;
            m_node->CollectProps(fields);
            m_graph->m_editor->m_properties->SetFields(fields);
            m_graph->m_editor->m_properties->SetFocus(ComponentType::NODE, m_node->m_uuid);
            m_propFocused = true;
            return true;
        }
    }
    return false;
}


//========================================================================================
void UINode::OnKeyPressed(char keycode)
{
    if (keycode == KEYCODE_DELETE)
    {
        m_graph->PushChanges();

        if (dynamic_cast<BTNodeRoot*>(m_node))
            return; // ignore delete root
        m_graph->m_context->RemoveNode(m_node);
        m_graph->m_nodeUIMap.erase(m_graph->m_nodeUIMap.find(m_node));
        m_graph->m_context->RefreshOrders();
        m_graph->m_updateField = true;
        m_graph->m_propFocused = false;
        m_graph->DeleteChild(this);
    }
    else
    {
        UIWidget::OnKeyPressed(keycode);
    }
}


//========================================================================================
void UINode::OnQuickMenu(int index, const std::string&)
{
    switch (index)
    {
    case 0: // add deco
    {
        m_graph->PushChanges();

        auto deco = new BTDecoratorDummy();
        AddDecorator(deco);
        return;
    }
    case 1: // add deco
    {
        m_graph->PushChanges();

        auto deco = new BTDecoratorCooldown();
        AddDecorator(deco);
        return;
    }
    case 2: // add deco
    {
        m_graph->PushChanges();

        auto deco = new BTDecoratorWatchValue();
        AddDecorator(deco);
        return;
    }
    case 3: // add deco
    {
        m_graph->PushChanges();

        auto deco = new BTDecoratorCanSee();
        AddDecorator(deco);
        return;
    }
    case 4: // add deco
    {
        m_graph->PushChanges();

        auto deco = new BTDecoratorIsInRange();
        AddDecorator(deco);
        return;
    }
    case 5: // break link
    {
        m_graph->PushChanges();

        m_node->AcceptParent(nullptr);
        m_node->m_context->RefreshOrders();
        break;
    }
    case 6: // delete
    {
        m_graph->PushChanges();

        if (dynamic_cast<BTNodeRoot*>(m_node))
            return; // ignore delete root
        m_graph->m_context->RemoveNode(m_node);
        m_graph->m_nodeUIMap.erase(m_graph->m_nodeUIMap.find(m_node));
        m_graph->m_context->RefreshOrders();

        m_graph->m_editor->m_properties->ClearFields();
        m_propFocused = true;
        m_graph->m_propFocused = false;
        
        m_graph->DeleteChild(this);
        break;
    }
    }
}


//========================================================================================
bool UINode::OnMouseDragBegin(const Vec2& pos, char keycode)
{
    if (keycode == KEYCODE_LEFT_MOUSE && m_focused)
    {
        if (m_hoverChild == m_outputs)
        {
            m_canvas->SetDragWidget(this);
            return true;
        }
        else if (m_graph->IsSelected(this))
        {
            m_canvas->SetDragWidget(this);
            m_graph->OnDragBegin(this, pos);
            return true;
        }
        else
        {
            m_canvas->SetDragWidget(this);
            m_dragging = true;
            m_dragOffset = m_viewBox.GetCenter() - pos;
            return true;
        }
    }
    return false;
}


//========================================================================================
void UINode::OnMouseDragFinish(const Vec2&, char keycode)
{
    m_graph->OnDragEnd(this);

    if (m_dragging && keycode == KEYCODE_LEFT_MOUSE)
    {
        m_dragging = false;
        m_canvas->SetDragWidget(nullptr);

        auto bt_parent = dynamic_cast<BTNodeComposite*>(m_graph->m_context->FindParent(m_node));

        if (bt_parent)
        {
            bt_parent->OnChildrenChanged();
            m_node->m_context->RefreshOrders();
        }
    }
}


//========================================================================================
bool UINode::AcceptDraggableWidget(UIWidget* source, const Vec2&, char keycode)
{
    if (keycode == KEYCODE_LEFT_MOUSE && m_hovered)
    {
        if (m_hoverChild == m_inputs)
        {
            m_canvas->SetDragWidget(nullptr);

            UINode* node = dynamic_cast<UINode*>(source);
            if (node && !node->m_dragging)
            {
                m_node->AcceptParent(node->m_node);
                m_node->m_context->RefreshOrders();
            }

            return true;
        }
    }
    return false;
}


//========================================================================================
void UINode::Refresh()
{
    RemoveAllChildren();

    m_rect.m_size.y = 0.02f * 2 + 0.04f + 0.02f;
    m_rect.m_size.y *= 0.75f;
    if (m_owner)
        SetViewport(m_owner->m_viewBox);

    m_layoutBox = AABB2::ZERO_TO_ONE;
    m_layoutBox.SetDimensions(Vec2(0.9f, 1.0f));
    AddChildLayout(m_inputs, ELayoutSide::TOP, 0.2f);
    AddChildLayout(m_outputs, ELayoutSide::BOTTOM, 0.2f);
    m_layoutBox.SetDimensions(Vec2(0.98f, m_layoutBox.GetDimensions().y));
    AddChildLayout(m_info, ELayoutSide::TOP, 0.6f);

    auto decorators = m_decorators;
    m_decorators.clear();

    for (auto btdeco : m_node->GetDecoratorList())
    {
        auto ite = std::find_if(decorators.begin(), decorators.end(), [btdeco](auto deco) { return deco->m_deco == btdeco; });
        auto deco = ite == decorators.end() ? new UINodeDecorator(this, btdeco) : *ite;
        m_decorators.push_back(deco);
        if (ite != decorators.end())
            decorators.erase(ite);
    }

    for (auto deco : decorators)
        delete deco; // garbage collect (decorator already be removed from somewhere else)

    float sizeY = m_info->m_viewBox.GetDimensions().y;
    for (auto deco : m_decorators)
    {
        AddChild(deco);
        deco->m_viewBox = m_info->m_viewBox;
        m_info->m_viewBox.Translate(Vec2(0, -sizeY));
        m_outputs->m_viewBox.Translate(Vec2(0, -sizeY));
    }

    m_viewBox.StretchToIncludePoint(m_outputs->m_viewBox.m_mins);
    RecalBox();
}


//========================================================================================
void UINode::AddDecorator(BTDecorator* deco)
{
    auto name = std::string(typeid(*deco).name()).substr(std::string("class BTDecorator").size());
    deco->m_name = name;
    m_graph->m_context->AddDecorator(m_node, deco);
    m_decorators.push_back(new UINodeDecorator(this, deco));
    Refresh();
    m_node->m_context->RefreshOrders(m_node);
}


//========================================================================================
UINodeDecorator::UINodeDecorator(UINode* node, BTDecorator* deco)
    : UIWidget("")
    , m_node(node)
    , m_deco(deco)
{
    m_rect.m_align = Vec2(0.5f, 0.5f);

    m_color = Rgba8(160, 160, 80);
}


//========================================================================================
void UINodeDecorator::OnUpdate()
{
}


//========================================================================================
void UINodeDecorator::Render(RenderPass pass) const
{
    if (m_render)
    {
        if (pass == RenderPass::SYMBOL)
        {
            AABB2 box = m_viewBox;
            auto color = Rgba8::WHITE;
            g_uiFont->AddVertsForTextInBox2D(g_vertsBuffer, box, 12.0f, m_deco->m_name, color, 0.666f, Vec2(0.5f, 0.5f), TextDrawMode::OVERRUN);
        }
    }

    UIWidget::Render(pass);

    if (m_render)
    {
        if (pass == RenderPass::SYMBOL)
        {
            AABB2 box =m_viewBox.GetSubBox(AABB2(0.0, 0.5, 0.1, 0.9));
            auto color = Rgba8::WHITE;
            g_uiFont->AddVertsForTextInBox2D(g_vertsBuffer, box, 12.0f, Stringf("%d", m_deco->m_order), color, 0.666f, Vec2(0.5f, 0.5f), TextDrawMode::OVERRUN);
        }
    }
}


//========================================================================================
void UINodeDecorator::OnGainFocus(const Vec2&)
{
    FieldList fields;
    m_deco->CollectProps(fields);
    m_node->m_graph->m_editor->m_properties->SetFields(fields);
    m_node->m_graph->m_editor->m_properties->SetFocus(ComponentType::DECORATOR, m_node->m_node->m_uuid, m_deco->m_uuid);
    m_node->m_propFocused = false;
}


//========================================================================================
bool UINodeDecorator::OnMouseClick(const Vec2& pos, char keycode)
{
    if (keycode == KEYCODE_RIGHT_MOUSE)
    {
        m_canvas->OpenMenu(this, pos, m_canvas->m_viewBox.GetDimensions() * Vec2(0.1f, 0.015f), { "Move Up", "Move Down", "Delete" });
        return true;
    }
    return false;
}


//========================================================================================
void UINodeDecorator::OnQuickMenu(int index, const std::string&)
{
    switch (index)
    {
    case 0: // move up
    {
        m_node->m_graph->PushChanges();

        auto node = m_deco->GetOwner();
        if (node->MoveUpDecorator(m_deco))
            m_node->Refresh();
        break;
    }
    case 1: // move down
    {
        m_node->m_graph->PushChanges();

        auto node = m_deco->GetOwner();
        if (node->MoveDownDecorator(m_deco))
            m_node->Refresh();
        break;
    }
    case 2: // delete
    {
        m_node->m_graph->PushChanges();

        auto node = m_deco->GetOwner();
        if (!node->RemoveDecorator(m_deco))
            throw "dangling decorator";

        m_node->Refresh(); // also handle deleting this (gc)
        break;
    }
    }
}


//========================================================================================
BTDecorator* UINodeDecorator::GetBTDecorator() const
{
    return m_deco;
}


//========================================================================================
BTNode* UINodeDecorator::GetBTNode() const
{
    return m_deco->GetOwner();
}


//========================================================================================
UIEditor::UIEditor(const char* name/* = "Canvas"*/, AABB2 box/* = AABB2::ZERO_TO_ONE*/)
    : UICanvas(name, box)
    , m_menu(new UIMenu(this))
    , m_tools(new UITools(this))
    , m_properties(new UIProperties(this))
    , m_graph(new UIGraph(this))
    , m_status(new UIStatus(this))
    , m_debugAI(UUID::invalidUUID())
{
    AddChildLayout(m_menu, ELayoutSide::TOP, 0.025f);
    AddChildLayout(m_tools, ELayoutSide::TOP, 0.05f);
    AddChildLayout(m_status, ELayoutSide::BOTTOM, 0.025f);
    AddChildLayout(m_properties, ELayoutSide::LEFT, 0.15f);
    AddChildLayout(m_graph, ELayoutSide::RIGHT, -1.0f);

    m_menu->AddMenu("File", { "New (Sample)...", "Load...", "Save...", "Quit" });
    m_menu->AddMenu("Edit", { "Undo(WIP)", "Redo(WIP)" });
    m_menu->AddMenu("About", { "About BT Editor...(WIP)" });

    m_graph->Load(m_filePath);

    auto nameFunc = [](auto node)
    {
        auto name = std::string(typeid(*node).name()).substr(std::string("class BTNode").size());
        g_theConsole->AddLine(DevConsole::LOG_INFO, Stringf("%s : %s", name.c_str(), node->m_name.c_str()));
    };

    nameFunc(m_graph->m_context->m_root);

    for (int i = 0; ; i++)
    {
        auto* node = m_graph->m_context->FindNodeByIndex(i);
        if (!node)
            break;
        nameFunc(node);
    }

    AddStatus("Initialize complete.");
}


//========================================================================================
void UIEditor::OnUpdate()
{
    UICanvas::OnUpdate();

    auto& debugBT = m_graph->m_debugBT;

    auto* debug = debugBT;
    auto* ai = AI::FindContext(m_debugAI);
    debugBT = ai ? ai->context : nullptr;

    if (debugBT && debugBT != debug)
    {
        m_graph->Load(&ai->btBuffer);
        ai->btBuffer.ResetRead();
    }
}


//========================================================================================
void UIEditor::OnMenuAction(const std::string& category, const std::string& action)
{
    if (action == "New (Sample)...")
    {
        m_filePath = "Data/AI/NewBT.bt";
        m_graph->InitTestNodes();

        AddStatus(Stringf("Initialized template."));
    }
    else
    if (action == "Load...")
    {
        auto filter = "Behavior Tree\0*.BT\0All\0*.*\0";
        auto ext = "BT";

        auto path = BasicFileOpen(g_editorWindow->GetHwnd(), filter);

        if (!path.empty())
        {
            m_filePath = std::move(path);
            m_graph->Load(m_filePath);

            AddStatus(Stringf("Loaded file: %s", path.c_str()));
        }

//        FieldList list;
//        list.emplace_back();
//
//        auto& val = list.back();
//        val.name = " Path";
//        val.value = m_filePath;
//        val.type = FieldType::TEXT;
//        val.callback = [&](auto text)
//        {
//            m_filePath = text;
//            return text;
//        };
//
//        auto comp = new UIOptions(this, "Load File", list, [&](bool result) 
//            {
//                if (result)
//                {
//                    m_graph->Load(m_filePath);
//                }
//            });
//        AddChildComponent(comp, Vec2(0.5f, 0.5f), Vec2(0.2f, 0.2f));
//        m_focusChild = comp;
//        comp->OnGainFocus(comp->m_viewBox.GetCenter());
    }
    else
    if (action == "Save...")
    {
        auto filter = "Behavior Tree\0*.BT\0All\0*.*\0";
        auto ext = "BT";

        auto path = BasicFileSave(g_editorWindow->GetHwnd(), filter, ext);

        if (!path.empty())
        {
            m_filePath = std::move(path);
            m_graph->Save(m_filePath);

            AddStatus(Stringf("Saved file: %s", path.c_str()));
        }

//        FieldList list;
//        list.emplace_back();
//
//        auto& val = list.back();
//        val.name = " Path";
//        val.value = m_filePath;
//        val.type = FieldType::TEXT;
//        val.callback = [&](auto text)
//        {
//            m_filePath = text;
//            return text;
//        };
//
//        auto comp = new UIOptions(this, "Save File", list, [&](bool result)
//            {
//                if (result)
//                {
//                    m_graph->Save(m_filePath);
//                }
//            });
//        AddChildComponent(comp, Vec2(0.5f, 0.5f), Vec2(0.2f, 0.2f));
//        m_focusChild = comp;
//        comp->OnGainFocus(comp->m_viewBox.GetCenter());
    }
    else
    if (action == "Quit")
    {
        g_theGame->LoadScene(nullptr);
        g_theGame->m_clickSound->PlaySound();
    }
    else
    if (action == "Undo(WIP)")
    {
        m_graph->UndoChanges();
    }
    else
    if (action == "Redo(WIP)")
    {
        m_graph->RedoChanges();
    }
}


//========================================================================================
void UIEditor::SetDebugAI(const AIIdentifier& ai)
{
    m_debugAI = ai;
}


//========================================================================================
void UIEditor::AddStatus(std::string info) const
{
    m_status->m_text = std::move(info);
}


//========================================================================================
void UIEditor::OnKeyPressed(char keycode)
{
    if (g_theInput->IsKeyDown(KEYCODE_LCTRL))
    {
        if (keycode == KEYCODE_S)
        {
            m_graph->Save(m_filePath);
            return;
        }
        if (keycode == KEYCODE_Z)
        {
            m_graph->UndoChanges();
            return;
        }
        if (keycode == KEYCODE_Y)
        {
            m_graph->RedoChanges();
            return;
        }
    }
    
    UIWidget::OnKeyPressed(keycode);
}


//========================================================================================
void UIGraph::InitTestNodes()
{
    ClearAllChildren();
    m_nodeUIMap.clear();
    delete m_context;
    delete m_board;

    m_board = new DataRegistry();
    m_context = new BTContext(*m_board);

    m_context->m_root->m_position.x = 0.5f;
    m_context->m_root->m_position.y = 0.8f;
    InitNode(m_context->m_root);

	BTNodeComposite* nodeComp = new BTNodeCompSequence();
	nodeComp->m_position.x = 0.5f;
	nodeComp->m_position.y = 0.7f;

    InitNode(nodeComp, m_context->m_root);

	BTNodeComposite* nodeComp2 = new BTNodeCompSelect();
	nodeComp2->m_position.x = 0.3f;
	nodeComp2->m_position.y = 0.5f;

    InitNode(nodeComp2, nodeComp);

	BTNodeComposite* nodeComp3 = new BTNodeCompSequence();
	nodeComp3->m_position.x = 0.7f;
	nodeComp3->m_position.y = 0.5f;

    InitNode(nodeComp3, nodeComp);

	BTNodeTaskDummy* dummy;
	BTNodeTaskWait* wait;

	dummy = new BTNodeTaskDummy();
	dummy->m_position.x = 0.2f;
    dummy->m_position.y = 0.3f;
    dummy->m_expectResult = EBTExecResult::FAILED;

    InitNode(dummy, nodeComp2);

	dummy = new BTNodeTaskDummy();
	dummy->m_position.x = 0.3f;
    dummy->m_position.y = 0.3f;
    dummy->m_expectResult = EBTExecResult::FAILED;

    InitNode(dummy, nodeComp2);

	wait = new BTNodeTaskWait();
	wait->m_position.x = 0.4f;
	wait->m_position.y = 0.3f;
	wait->m_time = 1.0f;

    InitNode(wait, nodeComp2);

	wait = new BTNodeTaskWait();
	wait->m_position.x = 0.6f;
	wait->m_position.y = 0.3f;
	wait->m_time = 3.0f;

    InitNode(wait, nodeComp3);

	wait = new BTNodeTaskWait();
	wait->m_position.x = 0.7f;
	wait->m_position.y = 0.3f;
	wait->m_time = 2.0f;

    InitNode(wait, nodeComp3);

	wait = new BTNodeTaskWait();
	wait->m_position.x = 0.8f;
	wait->m_position.y = 0.3f;
	wait->m_time = 1.0f;

    InitNode(wait, nodeComp3);
}


//========================================================================================
void UIGraph::Load(const std::string& path)
{
    if (!FileExists(path))
    {
        DebugAddMessage(("File does not exist: " + path).c_str(), 2.0f, Rgba8::WHITE, Rgba8::WHITE);
        return;
    }

    ByteBuffer buffer;
    if (FileReadToBuffer(buffer, path) < 0)
    {
        DebugAddMessage(("Failed to read file: " + path).c_str(), 2.0f, Rgba8::WHITE, Rgba8::WHITE);
        return;
    }

    Load(&buffer);
    DebugAddMessage(("Loaded file: " + path).c_str(), 2.0f, Rgba8::WHITE, Rgba8::WHITE);
}


//========================================================================================
void UIGraph::Load(ByteBuffer* buffer)
{
    for (auto& r : m_records)
        delete r;
    m_records.clear();
    for (auto& r : m_recordsRedo)
        delete r;
    m_recordsRedo.clear();

    ClearAllChildren();
    m_nodeUIMap.clear();
    m_selected.clear();
    delete m_context;
    delete m_board;

    m_board = new DataRegistry();
    m_context = new BTContext(*m_board);

    m_board->Load(buffer);
    m_context->m_canvas = AABB2::ZERO_TO_ONE;
    m_context->Load(buffer);

    AddNode(m_context->m_root);

    BTNode* node;
    int index = 0;
    while (node = m_context->FindNodeByIndex(index++))
    {
        AddNode(node, m_context->FindParent(node));
    }

    m_context->RefreshOrders();
}


//========================================================================================
void UIGraph::Save(const std::string& path)
{
    ByteBuffer buffer;
    m_board->Save(&buffer);
    m_context->m_canvas = m_viewBox;
    m_context->Save(&buffer);

    if (FileWriteFromBuffer(buffer, path) < 0)
    {
        DebugAddMessage(("Failed to write file: " + path).c_str(), 2.0f, Rgba8::WHITE, Rgba8::WHITE);
        return;
    }

    DebugAddMessage(("Saved file: " + path).c_str(), 2.0f, Rgba8::WHITE, Rgba8::WHITE);
}


//========================================================================================
void UIGraph::PushChanges()
{
    Record* record = new Record();
    m_records.push_back(record);

    m_board->Save(&record->m_buffer);
    m_context->m_canvas = m_viewBox;
    m_context->Save(&record->m_buffer);

    for (auto& rec : m_recordsRedo)
        delete rec;
    m_recordsRedo.clear();

    DebugAddMessage("Push changes", 2.0f, Rgba8::WHITE, Rgba8::WHITE);
}


//========================================================================================
void UIGraph::RedoChanges()
{
    // warn: carefully clear references on UIProperties

    if (m_recordsRedo.empty())
        return;

    ClearAllChildren();
    m_nodeUIMap.clear();
    delete m_context;
    delete m_board;

    m_board = new DataRegistry();
    m_context = new BTContext(*m_board);

    Record* record = m_records.back();
    m_records.pop_back();
    m_recordsRedo.push_back(record);

    m_board->Load(&record->m_buffer);
    m_context->m_canvas = AABB2::ZERO_TO_ONE;
    m_context->Load(&record->m_buffer);

    AddNode(m_context->m_root);

    BTNode* node;
    int index = 0;
    while (node = m_context->FindNodeByIndex(index++))
    {
        AddNode(node, m_context->FindParent(node));
    }

    m_context->RefreshOrders();

    m_editor->m_properties->Reload();

    DebugAddMessage("Pop changes", 2.0f, Rgba8::WHITE, Rgba8::WHITE);

    m_editor->AddStatus(Stringf("Redo action."));
}


//========================================================================================
void UIGraph::UpdateField()
{
    m_updateField = true;
}


//========================================================================================
void UIGraph::UndoChanges()
{
    // warn: carefully clear references on UIProperties

    if (m_records.empty())
        return;

    ClearAllChildren();
    m_nodeUIMap.clear();
    delete m_context;
    delete m_board;

    m_board = new DataRegistry();
    m_context = new BTContext(*m_board);

    Record* record = m_records.back();
    m_records.pop_back();
    m_recordsRedo.push_back(record);

    m_board->Load(&record->m_buffer);
    m_context->m_canvas = AABB2::ZERO_TO_ONE;
    m_context->Load(&record->m_buffer);

    AddNode(m_context->m_root);

    BTNode* node;
    int index = 0;
    while (node = m_context->FindNodeByIndex(index++))
    {
        AddNode(node, m_context->FindParent(node));
    }

    m_context->RefreshOrders();

    m_editor->m_properties->Reload();

    DebugAddMessage("Pop changes", 2.0f, Rgba8::WHITE, Rgba8::WHITE);

    m_editor->AddStatus(Stringf("Undo action."));
}


//========================================================================================
void UIGraph::AddBoardEntry()
{
    if (m_updateField)
        return;

    int i = 0;
    DataEntryHandle handle = INVALID_DATAENTRY_HANDLE;
    while (handle == INVALID_DATAENTRY_HANDLE)
    {
        std::string name = Stringf("New entry %d", i++);
        handle = m_board->RegisterEntry(name.c_str(), BTDataType::BOOLEAN);
    }
    m_updateField = true;
}


//========================================================================================
bool UIGraph::IsSelected(UINode* node) const
{
    return std::find(m_selected.begin(), m_selected.end(), node) != m_selected.end();
}


//========================================================================================
bool UIGraph::IsDragging(UINode* node) const
{
    return m_dragging || (IsSelected(node) && m_selectNode);
}


//========================================================================================
void UIGraph::OnDragBegin(UINode* node, const Vec2& pos)
{
    m_selectNode = node;
    m_dragOrigin = pos;
    m_dragOffset = m_canvas->GetMousePos() - pos;
}


//========================================================================================
void UIGraph::OnDragEnd(UINode* node)
{
    if (node == m_selectNode)
        m_selectNode = nullptr;
}


//========================================================================================
UIMenu::UIMenu(UIEditor* editor)
    : UIPanel("EditorMenu")
    , m_editor(editor)
{
    m_canvas = editor;

    m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/UI/grey3_panel.png");
    m_color = Rgba8::WHITE;
    m_cornerWidth = 0.0025f;
    m_cornerUVWidth = 0.07f;
}


//========================================================================================
void UIMenu::AddMenu(const std::string& name, const StringList& options)
{
    UIMenuButton* button = new UIMenuButton(name.c_str());
    button->m_label = name;
    button->m_options = options;
    AddChildLayout(button, ELayoutSide::LEFT, 0.05f);
}


//========================================================================================
void UIMenu::OnQuickMenu(int index, const std::string& option)
{
    m_editor->OnMenuAction("default", option);
}


//========================================================================================
UIProperties::UIProperties(UIEditor* editor)
    : UIPanel("EditorProperties")
    , m_titlePanel(new UIPanel("PropertiesTitle"))
    , m_focusType(ComponentType::INVALID)
    , m_focusUuid(UUID::invalidUUID())
    , m_editor(editor)
{
    m_canvas = editor;
    m_layoutBox.SetDimensions(Vec2(0.99f, 0.995f));

    AddChildLayout(m_titlePanel, ELayoutSide::TOP, 0.025f);
    m_initLayout = m_layoutBox;
    m_titlePanel->m_enabled = false;
    m_titlePanel->m_text = "Properties";
    m_titlePanel->m_color = Rgba8(80, 80, 180);
    m_titlePanel->m_fontSize = 15.0f;

    m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/UI/grey3_panel.png");
    m_color = Rgba8::WHITE;
    m_cornerWidth = 0.0025f * 6.666f;
    m_cornerUVWidth = 0.07f;
}


//========================================================================================
void UIProperties::SetNoFocus()
{
    SetFocus(ComponentType::INVALID, UUID::invalidUUID());
}


//========================================================================================
void UIProperties::SetFocusDataTable()
{
    SetFocus(ComponentType::DATA_TABLE, UUID::invalidUUID());
}


//========================================================================================
void UIProperties::SetFocus(ComponentType type, const UUID& uuid)
{
    m_focusType = type;
    m_focusUuid = uuid;
}


//========================================================================================
void UIProperties::SetFocus(ComponentType type, const UUID& uuid, const UUID& deco)
{
    SetFocus(type, uuid);
    m_decoUuid = deco;
}


//========================================================================================
void UIProperties::SetFields(FieldList fields)
{
    ClearFields();

    for (auto& f : fields)
        AddChildLayout(new UIPropEntry(f), ELayoutSide::TOP, 0.025f);

    if (m_hovered)
    {
        Vec2 mousePos = m_canvas->GetMousePos();
        OnHoverBegin(mousePos);
        OnHoverUpdate(mousePos);
    }
}


//========================================================================================
void UIProperties::AddField(const Field& field)
{
    AddChildLayout(new UIPropEntry(field), ELayoutSide::TOP, 0.025f);
}


//========================================================================================
void UIProperties::RemoveField(const std::string& name)
{
    for (auto* child : m_chidren)
    {
        if (child->m_name == name)
        {
            DeleteChild(child);
            return;
        }
    }
}


//========================================================================================
void UIProperties::ClearFields()
{
    SetNoFocus();
    auto children = m_chidren;
    std::reverse(children.begin(), children.end());

    for (auto* child : children)
    {
        if (dynamic_cast<UIPropEntry*>(child))
            DeleteChild(child);
    }

    m_layoutBox = m_initLayout;
}


//========================================================================================
void UIProperties::Reload()
{
    ClearFields();

    switch (m_focusType)
    {
    case ComponentType::INVALID:
    {
        break;
    }
    case ComponentType::DATA_TABLE:
    {
        FieldList fields;
        m_editor->m_graph->m_board->CollectProps(fields);

        fields.emplace_back();
        fields.back().name = "Operation";
        fields.back().type = FieldType::ENUM;
        fields.back().defaults = { "ADD NEW ENTRY" };
        fields.back().value = "";
        fields.back().callback = [&](auto value)
        {
            if (value == "ADD NEW ENTRY")
            {
                m_editor->m_graph->AddBoardEntry();
            }
            return "";
        };
        SetFields(fields);
        break;
    }
    case ComponentType::NODE:
    {
        auto node = m_editor->m_graph->m_context->FindNode(m_focusUuid);

        if (node)
        {
            FieldList fields;

            node->CollectProps(fields);

            SetFields(fields);
        }
        break;
    }
    case ComponentType::DECORATOR:
    {
        auto node = m_editor->m_graph->m_context->FindNode(m_focusUuid);

        if (node)
        {
            auto deco = node->FindDecorator(m_decoUuid);

            if (deco)
            {
                FieldList fields;

                deco->CollectProps(fields);

                SetFields(fields);
            }
        }
        break;
    }
    default:
        break;
    }
}


//========================================================================================
UIStatus::UIStatus(UIEditor* editor)
    : UIPanel("EditorStatus")
{
    m_canvas = editor;

    m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/UI/grey3_panel.png");
    m_color = Rgba8::WHITE;
    m_cornerWidth = 0.0025f;
    m_cornerUVWidth = 0.07f;

    m_fontAlign.x = 0.0f;
    m_fontBoxSize.x = 0.995f;
}


//========================================================================================
UITools::UITools(UIEditor* editor)
    : UIPanel("EditorTools")
{
    m_canvas = editor;

    m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/UI/grey3_panel.png");
    m_color = Rgba8::WHITE;
    m_cornerWidth = 0.0025f;
    m_cornerUVWidth = 0.07f;
}


//========================================================================================
UIPropEntry::UIPropEntry(const Field& field, float ratio)
    : UIWidget("PropEntry")
    , m_name(new UIText("Name", field.name))
    , m_input(new UIInputText())
{
    m_color = Rgba8(50, 50, 50);

    AddChildComponent(m_input, Vec2(1.00f, 0.5f), Vec2(ratio, 0.9f), Vec2(1.0f, 0.5f));
    AddChildComponent(m_name,  Vec2(0.00f, 0.5f), Vec2(1 - ratio, 0.9f), Vec2(0.0f, 0.5f));

    m_name->m_area = Vec2(0.95f, 0.9f);
    m_name->m_fontAlign = Vec2(0.05f, 0.5f);
    m_name->m_fontSize = 15;
    m_name->m_color = Rgba8(120,120,120);
    m_input->m_fontSize = 15;
    m_input->SetInput(field.value);
    m_input->SetSuggestions(field.defaults);
    switch (field.type)
    {
    case FieldType::ENUM:
        m_input->SetInputEnabled(false);
        break;
    case FieldType::TEXT:
        m_input->SetInputEnabled(true);
        m_input->SetTextMode();
        break;
    case FieldType::NUMBER:
        m_input->SetInputEnabled(true);
        m_input->SetNumberMode();
        break;
    default:
        throw "No corresponding enum case";
    }
    m_input->SetCallback([field, this](auto text)
        {
            UIProperties* prop = dynamic_cast<UIProperties*>(m_owner);

            if (prop)
            {
                prop->m_editor->m_graph->PushChanges();
            }

            return field.callback(text);
        });
}


//========================================================================================
UIOptions::UIOptions(UIEditor* editor, const std::string& title, const FieldList& fields, std::function<void(bool)> callback)
    : UIWidget("options")
    , m_callback(callback)
{
    m_zDepth = -0.5f;

    auto titlePanel(new UIPanel("OptionsTitle"));
    m_canvas = editor;
    m_layoutBox.SetDimensions(Vec2(0.99f, 0.995f));

    AddChildLayout(titlePanel, ELayoutSide::TOP, 0.15f);
    titlePanel->m_enabled = false;
    titlePanel->m_text = title;
    titlePanel->m_color = Rgba8(80, 80, 180);
    titlePanel->m_fontSize = 15.0f;

    m_layoutBox.ChopOffTop(0.1f, false);
    m_layoutBox.ChopOffLeft(0.1f, false);
    m_layoutBox.ChopOffRight(0.1f, false);

    for (auto& f : fields)
    {
        AddChildLayout(new UIPropEntry(f, 0.8f), ELayoutSide::TOP, 0.15f);
    }

    auto button = new UIButton("Accept");
    button->m_zDepth = -0.1f;
    AddChildComponent(button, Vec2(0.5f, 0.2f), Vec2(0.2f, 0.15f));
    auto buttonPanel = new UIPanel("Accept", Rgba8(80, 180, 80));
    buttonPanel->m_text = "Accept";
    buttonPanel->m_fontSize = 20;
    AddChildComponent(buttonPanel, Vec2(0.5f, 0.2f), Vec2(0.2f, 0.15f));
}


//========================================================================================
void UIOptions::OnLoseFocus()
{
    m_callback(false);
    m_owner->m_focusChild = nullptr;
    m_owner->RemoveChild(this);
    delete this;
}


//========================================================================================
bool UIOptions::PostMessage(UIWidget* source, const std::string& message)
{
    if (message == "Btn:Accept")
    {
        m_callback(true);
        m_owner->m_focusChild = nullptr;
        m_owner->RemoveChild(this);
        delete this;
        return true;
    }

    return m_owner->PostMessage(source, message);
}

