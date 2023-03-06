#pragma once

#include <string>
#include <vector>
#include <functional>

#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/UUID.hpp"
#include "Engine/Core/Stopwatch.hpp"

#include "Game/Editor/BTDataTable.hpp"

class Actor;
class AI;
class BTNode;
class BTNodeRoot;
class BTDecorator;
struct Field;

using FieldList = std::vector<Field>;
using BTNodeList = std::vector<BTNode*>;
using BTDecoList = std::vector<BTDecorator*>;

constexpr char BT_FORMAT_VERSION_MAJOR = 0x01;
constexpr char BT_FORMAT_VERSION_MINOR = 0x02;


// =====================================================================
// =====================================================================
enum class EBTExecResult : uint8_t
{
	UNKNOWN,
	SUCCESS,
	FAILED,
	ABORTED,
};


// =====================================================================
// =====================================================================
class BTContext
{
public:
	BTContext(DataRegistry& registry);
	~BTContext();

    void Execute();

    void RefreshOrders(BTNode* node = nullptr);

    void AddDecorator(BTNode* node, BTDecorator* decorator);
    void AddNode(BTNode* node, BTNode* parent = nullptr);

	BTNode* FindParent(BTNode* node);

	void RemoveNode(BTNode* m_node);

	int FindNodeIndex(BTNode* node);
	BTNode* FindNodeByIndex(int index);
    BTNode* FindNode(const UUID& uuid);

    void Load(ByteBuffer* buffer);
    void Save(ByteBuffer* buffer) const;

public:
	Actor* m_actor = nullptr;
    AI* m_contorller = nullptr;
	BTNodeRoot* m_root = nullptr;
	BTNodeList m_nodes;
	BTNodeList m_execStack;
	DataTable m_table;
	bool m_evaluate = true;
    AABB2 m_canvas = AABB2::ZERO_TO_ONE;
    bool m_aborting = false;
    int m_lod = 1;
};


// =====================================================================
// =====================================================================
class BTBase
{
public:
	BTBase(const std::string& name);
    virtual ~BTBase();

	virtual void Tick() = 0;
	virtual void CollectProps(FieldList& fields);

    virtual const std::string& GetName() const;

    virtual const char* GetRegistryName() const = 0;

	virtual void Load(ByteBuffer* buffer);
	virtual void Save(ByteBuffer* buffer) const;

public:
	std::string m_name;
    UUID m_uuid;
    int m_order = 0;
};


// =====================================================================
// =====================================================================
class BTNode : public BTBase
{
	friend class BTContext;

public:
	static inline bool IsValid(const BTNode* node);
	static bool ComparePosition(const BTNode* a, const BTNode* b);

public:
	BTNode(const std::string& name = "Node");
    virtual ~BTNode();

	virtual bool Evaluate();
	virtual void Execute() = 0;
    virtual void Tick() override;
    virtual void NotifyAbort() = 0;

    virtual void OnBeginExecute();
    virtual void OnFinishExecute(bool result);
    virtual void OnAbortExecute();

	virtual bool IsChild(BTNode* node) const;
	virtual void RemoveChild(BTNode*) {}
	virtual bool AddChild(BTNode*) { return false; }
	virtual void AcceptParent(BTNode*) {}

	void BeginExecute();
	void FinishExecute(bool success);
	void FinishAbort();

	virtual void ForChildNode(std::function<void(BTNode*)> action) {}
	virtual void ForAllChildNode(std::function<void(BTNode*)> action);
	void ForAllDecorator(std::function<void(BTDecorator*)> action);

	inline bool IsExecuting() const    { return m_executing; }
	inline bool IsSuccess()   const    { return m_result == EBTExecResult::SUCCESS; }
	inline bool IsFailed()    const    { return m_result == EBTExecResult::FAILED; }
	inline bool IsAborted()   const    { return m_result == EBTExecResult::ABORTED; }
    inline void ResetState()           { m_executing = false; m_result = EBTExecResult::UNKNOWN; }

	inline const Vec2& GetPosition() const { return m_position; }

	const BTDecoList& GetDecoratorList() const { return m_decorators; }

	bool MoveUpDecorator(BTDecorator* deco);
	bool MoveDownDecorator(BTDecorator* deco);
    bool RemoveDecorator(BTDecorator* deco);

    BTDecorator* FindDecorator(const UUID& uuid);

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

protected:
	BTDecoList m_decorators;

	bool m_executing = false;
	EBTExecResult m_result = EBTExecResult::UNKNOWN;

	// UI
public:
    BTContext* m_context = nullptr;
	Vec2 m_position;
};


// =====================================================================
// =====================================================================
class BTDecorator : public BTBase
{
    friend class BTContext;

public:
	BTDecorator(const std::string& name = "Decorator");
    virtual ~BTDecorator();

public:
	virtual bool CheckCondition() = 0;
    virtual void Tick() override;
    virtual void CollectProps(FieldList& fields) override;

	virtual void OnExecuteStarted();
	virtual void OnExecuteFinished(EBTExecResult result);

    BTNode* GetOwner() const;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

protected:
	BTNode * m_owner = nullptr;
    bool     m_abortSelf = false;
    bool     m_abortLower = false;

private:
    bool m_cachedCondition = false;
};


// =====================================================================
// =====================================================================
class BTNodeRoot : public BTNode
{
    friend class BTContext;

private:
    BTNodeRoot();

public:
    virtual void Execute() override;
    virtual bool IsChild(BTNode* node) const override;
	virtual bool AddChild(BTNode* node) override;
    virtual void RemoveChild(BTNode* node) override;
    virtual void NotifyAbort() override;

	void SetEntry(BTNode* node);

    virtual void ForChildNode(std::function<void(BTNode*)> action) override;
    virtual void ForAllChildNode(std::function<void(BTNode*)> action) override;

    virtual const char* GetRegistryName() const;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
	BTNode* m_entry = nullptr;
};


// =====================================================================
// =====================================================================
class BTNodeComposite : public BTNode
{
public:
    virtual void Execute() = 0;
    virtual void CollectProps(FieldList& fields) override;
    virtual bool IsChild(BTNode* node) const;
    virtual bool AddChild(BTNode* node) override;
    virtual void RemoveChild(BTNode* node) override;
	virtual void AcceptParent(BTNode* m_node) override;
    virtual void NotifyAbort() override;

	void OnChildrenChanged();

    virtual void ForChildNode(std::function<void(BTNode*)> action) override;
    virtual void ForAllChildNode(std::function<void(BTNode*)> action) override;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
	BTNodeList m_children;
	int m_activeNode = 0;
	bool m_decoratorScoped = false;
};


// =====================================================================
// =====================================================================
class BTNodeCompSequence : public BTNodeComposite
{
public:
    virtual void Execute() override;
    virtual const char* GetRegistryName() const;

private:
};


// =====================================================================
// =====================================================================
class BTNodeCompSelect : public BTNodeComposite
{
public:
    virtual void Execute() override;
    virtual const char* GetRegistryName() const;

private:
};


// =====================================================================
// =====================================================================
class BTDecoratorDummy : public BTDecorator
{
public:
    virtual bool CheckCondition() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
	bool m_shouldPass = true;
};


// =====================================================================
// =====================================================================
class BTDecoratorCooldown : public BTDecorator
{
public:
    virtual bool CheckCondition() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;

    virtual void OnExecuteFinished(EBTExecResult result) override;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
    float m_duration = true;

private:
    Stopwatch m_timer;
};


// =====================================================================
// =====================================================================
class BTDecoratorCanSee : public BTDecorator
{
public:
    virtual bool CheckCondition() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
    std::string m_key;
    float m_angle = 10;
    float m_range = 20;
    bool  m_raycast = true;
    bool  m_reverse = false;

private:
    DataEntryHandle m_keyHandle = INVALID_DATAENTRY_HANDLE;
};


// =====================================================================
// =====================================================================
class BTDecoratorIsInRange : public BTDecorator
{
public:
    virtual bool CheckCondition() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
    std::string m_key;
    float       m_range = 10;
    bool        m_reverse = false;

private:
    DataEntryHandle m_keyHandle = INVALID_DATAENTRY_HANDLE;
};


// =====================================================================
// =====================================================================
class BTDecoratorWatchValue : public BTDecorator
{
public:
    virtual bool CheckCondition() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
    std::string m_key;
    bool        m_checkSet = true;
	bool        m_reverse = false;
	std::string m_value;

private:
	DataEntryHandle m_keyHandle = INVALID_DATAENTRY_HANDLE;
};


// =====================================================================
// =====================================================================
class BTNodeTask : public BTNode
{
public:
	virtual bool Evaluate() override final;
	virtual void Execute() override final;
    virtual void DoExecute() = 0;
    virtual void AcceptParent(BTNode* m_node) override;
    virtual void NotifyAbort() override;
};


// =====================================================================
// =====================================================================
class BTNodeTaskDummy : public BTNodeTask
{
public:
    virtual void DoExecute() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
	EBTExecResult m_expectResult;
};


// =====================================================================
// =====================================================================
class BTNodeTaskSetValue : public BTNodeTask
{
public:
    virtual void DoExecute() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
    std::string m_key;
    std::string m_fromKey;

private:
    DataEntryHandle m_keyHandle = INVALID_DATAENTRY_HANDLE;
    DataEntryHandle m_fromKeyHandle = INVALID_DATAENTRY_HANDLE;
};


// =====================================================================
// =====================================================================
class BTNodeTaskPlaySound : public BTNodeTask
{
public:
    virtual void DoExecute() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
    std::string m_soundName;
	float m_volume = 1.0f;
	float m_speed = 1.0f;
};


// =====================================================================
// =====================================================================
class BTNodeTaskMakeNoise : public BTNodeTask
{
public:
    virtual void DoExecute() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
    float m_volume = 1.0f;
};


// =====================================================================
// =====================================================================
class BTNodeTaskFireEvent : public BTNodeTask
{
public:
    virtual void DoExecute() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
    std::string m_eventName;
    std::string m_eventArgs;
};


// =====================================================================
// =====================================================================
class BTNodeTaskMoveTo : public BTNodeTask
{
public:
    virtual void DoExecute() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;
    virtual void OnAbortExecute() override;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
    std::string m_key;
	float       m_radius = 0.5f;

private:
    DataEntryHandle m_keyHandle = INVALID_DATAENTRY_HANDLE;
	bool m_moving = false;
};


// =====================================================================
// =====================================================================
class BTNodeTaskAttack : public BTNodeTask
{
public:
    virtual void DoExecute() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
    std::string m_key;
    float       m_damage = 0.5f;

private:
    DataEntryHandle m_keyHandle = INVALID_DATAENTRY_HANDLE;
    bool m_moving = false;
};


// =====================================================================
// =====================================================================
class BTNodeTaskRandomPoint : public BTNodeTask
{
public:
    virtual void DoExecute() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
    std::string m_targetKey;
    float       m_range = 10.0f;

private:
    DataEntryHandle m_keyHandle = INVALID_DATAENTRY_HANDLE;
};


// =====================================================================
// =====================================================================
class BTNodeTaskKeepDistance : public BTNodeTask
{
public:
    virtual void DoExecute() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
    std::string m_targetKey;
    float       m_range = 10.0f;

private:
    DataEntryHandle m_keyHandle = INVALID_DATAENTRY_HANDLE;
    bool m_moving = false;
};


// =====================================================================
// =====================================================================
class BTNodeTaskWait : public BTNodeTask
{
public:
    virtual void DoExecute() override;
    virtual void CollectProps(FieldList& fields) override;
    virtual const char* GetRegistryName() const;
    virtual void OnAbortExecute() override;

    virtual void Load(ByteBuffer* buffer);
    virtual void Save(ByteBuffer* buffer) const;

public:
	float m_time = 1.0f;
	Stopwatch m_stopwatch;
};






// =====================================================================
// =====================================================================
// =====================================================================
bool BTNode::IsValid(const BTNode* node)
{
	return node != nullptr;
}

