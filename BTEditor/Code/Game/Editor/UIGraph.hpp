#pragma once

#include "Game/UI/UIWidget.hpp"
#include "Game/UI/UIComponents.hpp"
#include "Game/UI/UICanvas.hpp"
#include "Game/Editor/BTNode.hpp"
#include "Engine/Core/ByteBuffer.hpp"
#include "Engine/Core/UUID.hpp"

#include <map>


class UIEditor;
class UIGraph;
class UINode;
struct Field;
using FieldList = std::vector<Field>;


//========================================================================================
enum class ComponentType
{
    INVALID,
    DATA_TABLE,
    NODE,
    DECORATOR,
};


//========================================================================================
class UINodeDecorator : public UIWidget
{
    friend class UINode;

public:
    UINodeDecorator(UINode* node, BTDecorator* deco);

    virtual void OnUpdate() override;
    virtual void Render(RenderPass pass) const override;

    virtual void OnGainFocus(const Vec2& mousePos) override;
    virtual bool OnMouseClick(const Vec2& pos, char keycode) override;
    virtual void OnQuickMenu(int index, const std::string& option) override;

    BTDecorator* GetBTDecorator() const;
    BTNode*      GetBTNode() const;

private:
    UINode      *const m_node;
    BTDecorator *const m_deco;
};


//========================================================================================
class UINode : public UIWidget
{
    friend class UINodeDecorator;

public:
    UINode(UIGraph* graph, BTNode* node);

    BTNode* GetBTNode() const;

    virtual void OnUpdate() override;
    virtual void Render(RenderPass pass) const override;

    virtual void OnGainFocus(const Vec2& mousePos) override;
    virtual bool OnClick(const Vec2& pos, char keycode) override;
    virtual void OnKeyPressed(char keycode) override;
    virtual void OnQuickMenu(int index, const std::string& option) override;
    virtual bool OnMouseDragBegin(const Vec2& pos, char keycode) override; // return allow drag
    virtual void OnMouseDragFinish(const Vec2& pos, char keycode) override;
    virtual bool AcceptDraggableWidget(UIWidget* source, const Vec2& pos, char keycode) override; // return accept drag

    void Refresh();
    void AddDecorator(BTDecorator* deco);

private:
    UIGraph *const m_graph;
    BTNode  *const m_node;
    Rgba8          m_nodeColor = Rgba8(180,180,180);

    UIPanel* m_inputs;
    UIPanel* m_outputs;
    UIPanel* m_info;
    std::vector<UINodeDecorator*> m_decorators;

    bool m_propFocused = false;
    bool m_dragging = false;
    Vec2 m_dragOffset;
};


//========================================================================================
class Record
{
public:
    ByteBuffer m_buffer;
};


//========================================================================================
class UIGraph : public UIWidget
{
    friend class UINode;

public:
    UIGraph(UIEditor* editor);

    virtual void OnUpdate() override;
    virtual bool OnClick(const Vec2& pos, char keycode) override;
    virtual void OnGainFocus(const Vec2& mousePos) override;
    virtual void OnLoseFocus() override;
    virtual void OnQuickMenu(int index, const std::string& option) override;
    virtual bool OnMouseDragBegin(const Vec2& pos, char keycode) override; // return allow drag
    virtual void OnMouseDragFinish(const Vec2& pos, char keycode) override;

    virtual void Render(RenderPass pass) const override;

    void LoadBTContext(BTContext* context);

    void InitTestNodes();

    void Load(const std::string& path);
    void Load(ByteBuffer* buffer);
    void Save(const std::string& path);

    void PushChanges();
    void UndoChanges();
    void RedoChanges();

    void UpdateField();

    void AddBoardEntry();

    bool IsSelected(UINode* node) const;
    bool IsDragging(UINode* node) const;

    void OnDragBegin(UINode* node, const Vec2& pos);
    void OnDragEnd(UINode* node);

private:
    void AddNode(BTNode* node, BTNode* parent = nullptr);
    void InitNode(BTNode* node, BTNode* parent = nullptr);

public:
    UIEditor* const m_editor;
    DataRegistry* m_board;
    BTContext* m_context;
    BTContext* m_debugBT = nullptr;
    std::vector<Record*> m_records;
    std::vector<Record*> m_recordsRedo;

private:
    Vec2     m_menuMousePos;
    bool     m_updateField = false;

    bool     m_dragging = false;
    Vec2     m_dragOrigin;
    Vec2     m_dragOffset;
    bool     m_propFocused = false;

    bool     m_selecting = false;
    UINode*  m_selectNode = nullptr;
    Vec2     m_selectArea[2] = {};
    std::vector<UINode*> m_selected;

    std::map<BTNode*, UINode*> m_nodeUIMap;
};


//========================================================================================
class UIMenu : public UIPanel
{
public:
    UIMenu(UIEditor* editor);

    void AddMenu(const std::string& name, const StringList& options);

    virtual void OnQuickMenu(int index, const std::string& option) override;

public:
    UIEditor* const m_editor;
};


//========================================================================================
class UITools : public UIPanel
{
public:
    UITools(UIEditor* editor);
};


//========================================================================================
enum class FieldType
{
    NUMBER,
    TEXT,
    ENUM,
};

struct Field
{
    std::string name;
    std::string value;
    StringList defaults;
    FieldType type = FieldType::TEXT;
    UIInputText::Callback callback = [](auto str) { return str; };
};


//========================================================================================
class UIPropEntry : public UIWidget
{
public:
    UIPropEntry(const Field& field, float ratio = 0.6f);

private:
    Field        m_field;
    UIText*      m_name;
    UIInputText* m_input;
};


//========================================================================================
class UIProperties : public UIPanel
{
public:
    UIProperties(UIEditor* editor);

    void SetNoFocus();
    void SetFocusDataTable();
    void SetFocus(ComponentType type, const UUID& uuid);
    void SetFocus(ComponentType type, const UUID& uuid, const UUID& deco);
    void SetFields(FieldList fields);
    void AddField(const Field& field);
    void RemoveField(const std::string& name);
    void ClearFields();
    void Reload();

private:
    UIPanel* m_titlePanel;
    AABB2 m_initLayout;

public:
    UIEditor* m_editor;
    ComponentType m_focusType;
    UUID m_focusUuid;
    UUID m_decoUuid = UUID::invalidUUID();
};


//========================================================================================
class UIStatus : public UIPanel
{
public:
    UIStatus(UIEditor* editor);
};

class UUID;
class ByteBuffer;

typedef UUID AIIdentifier;


//========================================================================================
class UIEditor : public UICanvas
{
public:
    UIEditor(const char* name = "BTEditor", AABB2 box = AABB2::ZERO_TO_ONE);

    virtual void OnUpdate() override;
    virtual void OnKeyPressed(char keycode) override;

    void OnMenuAction(const std::string& category, const std::string& action);

    void SetDebugAI(const AIIdentifier& ai);
    void AddStatus(std::string info) const;

public:
    UIMenu *const             m_menu;
    UITools*const             m_tools;
    UIProperties *const       m_properties;
    UIGraph *const            m_graph;
    UIStatus *const           m_status;
    AIIdentifier              m_debugAI;

    std::string               m_filePath = "Data/AI/SampleBT.bt";
};


//========================================================================================
class UIOptions : public UIWidget
{
public:
    UIOptions(UIEditor* editor, const std::string& title, const FieldList& fields, std::function<void(bool)> callback);

    virtual void OnLoseFocus() override;
    virtual bool PostMessage(UIWidget* source, const std::string& message) override;

private:
    std::function<void(bool)> m_callback;
};

