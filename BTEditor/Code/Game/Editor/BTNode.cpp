#include "Game/Editor/BTNode.hpp"

#include "Game/Editor/UIGraph.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ByteBuffer.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

#include "Game/Entity/Actor.hpp"
#include "Game/World/World.hpp"

#include <algorithm>
#include <typeinfo>

#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Game/Entity/AI.hpp"


std::map<std::string, std::function<BTNode* ()>> CreateBTNodeRegistry()
{
	std::map<std::string, std::function<BTNode* ()>> map;

	map["CompSequence"]     = []() { return new BTNodeCompSequence(); };
    map["CompSelect"]       = []() { return new BTNodeCompSelect(); };
    map["TaskDummy"]        = []() { return new BTNodeTaskDummy(); };
    map["TaskSetValue"]     = []() { return new BTNodeTaskSetValue(); };
    map["TaskPlaySound"]    = []() { return new BTNodeTaskPlaySound(); };
    map["TaskFireEvent"]    = []() { return new BTNodeTaskFireEvent(); };
    map["TaskMoveTo"]       = []() { return new BTNodeTaskMoveTo(); };
    map["TaskWait"]         = []() { return new BTNodeTaskWait(); };
    map["TaskAttack"]       = []() { return new BTNodeTaskAttack(); };
    map["TaskRandomPoint"]  = []() { return new BTNodeTaskRandomPoint(); };
    map["TaskKeepDistance"] = []() { return new BTNodeTaskKeepDistance(); };

	return map;
}


std::map<std::string, std::function<BTNode* ()>> BT_NODE_REGISTRY = CreateBTNodeRegistry();

std::map<std::string, std::function<BTDecorator* ()>> CreateBTDecoRegistry()
{
    std::map<std::string, std::function<BTDecorator* ()>> map;
	
	map["DecoDummy"]         = []() { return new BTDecoratorDummy(); };
	map["DecoCooldown"]      = []() { return new BTDecoratorCooldown(); };
    map["DecoWatchValue"]    = []() { return new BTDecoratorWatchValue(); };
    map["DecoCanSee"]        = []() { return new BTDecoratorCanSee(); };
    map["DecoInRange"]       = []() { return new BTDecoratorIsInRange(); };
	
	// aliases
	map["DecoratorDummy"]         = []() { return new BTDecoratorDummy(); };
	map["DecoratorCooldown"]      = []() { return new BTDecoratorCooldown(); };
    map["DecoratorWatchValue"]    = []() { return new BTDecoratorWatchValue(); };
    map["DecoratorCanSee"]        = []() { return new BTDecoratorCanSee(); };
    map["DecoratorIsInRange"]     = []() { return new BTDecoratorIsInRange(); };

    return map;
}


std::map<std::string, std::function<BTDecorator* ()>> BT_DECO_REGISTRY = CreateBTDecoRegistry();


//========================================================================================
BTNodeRoot::BTNodeRoot() : BTNode()
{

}


//========================================================================================
void BTNodeRoot::Execute()
{
	if (!m_executing && m_entry)
		BeginExecute();

	if (m_executing)
		m_entry->Execute();
}


//========================================================================================
bool BTNodeRoot::IsChild(BTNode* node) const
{
	return node == m_entry;
}


//========================================================================================
bool BTNodeRoot::AddChild(BTNode* node)
{
	m_entry = node;
	return true;
}


//========================================================================================
void BTNodeRoot::RemoveChild(BTNode* node)
{
	if (node == m_entry)
		m_entry = nullptr;
}


//========================================================================================
void BTNodeRoot::NotifyAbort()
{
	if (m_executing)
		m_entry->NotifyAbort();
}


//========================================================================================
void BTNodeRoot::SetEntry(BTNode* node)
{
	m_entry = node;
}


//========================================================================================
void BTNodeRoot::ForChildNode(std::function<void(BTNode*)> action)
{
    if (!m_entry)
        return;

	action(m_entry);
}


//========================================================================================
void BTNodeRoot::ForAllChildNode(std::function<void(BTNode*)> action)
{
	if (!m_entry)
		return;

	action(m_entry);
	m_entry->ForAllChildNode(action);
}


//========================================================================================
const char* BTNodeRoot::GetRegistryName() const
{
	static const char* name = "Root";
	return name;
}


//========================================================================================
const char* BTNodeCompSequence::GetRegistryName() const
{
    static const char* name = "CompSequence";
    return name;
}


//========================================================================================
const char* BTNodeCompSelect::GetRegistryName() const
{
    static const char* name = "CompSelect";
    return name;
}


//========================================================================================
const char* BTDecoratorWatchValue::GetRegistryName() const
{
    static const char* name = "DecoWatchValue";
    return name;
}


//========================================================================================
const char* BTNodeTaskDummy::GetRegistryName() const
{
    static const char* name = "TaskDummy";
    return name;
}


//========================================================================================
const char* BTNodeTaskPlaySound::GetRegistryName() const
{
    static const char* name = "TaskPlaySound";
    return name;
}


//========================================================================================
const char* BTNodeTaskFireEvent::GetRegistryName() const
{
    static const char* name = "TaskFireEvent";
    return name;
}


//========================================================================================
void BTNodeTaskMoveTo::DoExecute()
{
	auto entry = m_context->m_table.FindEntry(m_keyHandle);
	
	if (!entry)
	{
		FinishExecute(false);
		return;
	}

	Vec3 target = Vec3::ZERO;

	if (entry->value.GetType() == BTDataType::VECTOR)
	{
		target = entry->value.GetAsVector();
	}
	else if (entry->value.GetType() == BTDataType::ACTOR)
	{
		auto targetActor = *entry->value.GetAsActor();

		if (!targetActor)
		{
			FinishExecute(false);
			return;
		}

		target = targetActor->GetPosition();
	}
	else
	{
		FinishExecute(false);
		return;
	}

	if (!m_moving)
	{
		m_moving = true;

		m_context->m_contorller->MoveTo(target);
	}
	else
	{
		if (!m_context->m_contorller->IsMoving())
		{
			m_moving = false;

			Vec3 dest = m_context->m_actor->GetPosition();

			if ((dest - target).GetLengthSquared() <= m_radius * m_radius)
			{
				FinishExecute(true);
			}
			else
			{
				DebugAddMessage("Not moved", 2, Rgba8::WHITE, Rgba8::WHITE);
				FinishExecute(true);
			}
			return;
		}
	}
}


//========================================================================================
void BTNodeTaskMoveTo::CollectProps(FieldList& fields)
{
    BTNodeTask::CollectProps(fields);

    Field* f;

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
    f->value = m_key;
    f->name = "TargetKey";
    f->callback = [this](auto text)
    {
        m_keyHandle = m_context->m_table.m_registry->GetHandle(text.c_str());
        return m_key = text;
    };

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::NUMBER;
    f->value = Stringf("%.2f", m_radius);
    f->name = "Radius";
    f->callback = [this](auto text)
    {
        return Stringf("%.2f", m_radius = (float)atof(text.c_str()));
    };
}


//========================================================================================
const char* BTNodeTaskMoveTo::GetRegistryName() const
{
    static const char* name = "TaskMoveTo";
    return name;
}


//========================================================================================
void BTNodeTaskMoveTo::OnAbortExecute()
{
	m_moving = false;
	if (m_context->m_contorller->IsMoving())
		m_context->m_contorller->StopMoving();
}


//========================================================================================
const char* BTNodeTaskWait::GetRegistryName() const
{
    static const char* name = "TaskWait";
    return name;
}


//========================================================================================
void BTNodeTaskWait::OnAbortExecute()
{
	m_stopwatch.Stop();
}


//========================================================================================
void BTNodeComposite::CollectProps(FieldList& fields)
{
    BTNode::CollectProps(fields);

    fields.emplace_back();
    Field* f;

    f = &fields.back();
    f->type = FieldType::ENUM;
    f->defaults = { "TRUE", "FALSE" };
    f->name = "Scoped";
	f->value = m_decoratorScoped ? "TRUE" : "FALSE";
    f->callback = [this](auto text)
    {
        if (text == "TRUE")
            m_decoratorScoped = true;
        else
            m_decoratorScoped = false;

        return text;
    };
}


//========================================================================================
bool BTNodeComposite::IsChild(BTNode* node) const
{
	for (auto child : m_children)
		if (child == node)
			return true;
	return false;
}


//========================================================================================
void BTNodeComposite::RemoveChild(BTNode* node)
{
	for (auto ite = m_children.begin(); ite != m_children.end(); ite++)
	{
		if (*ite == node)
        {
            m_children.erase(ite);
            OnChildrenChanged();
			return;
		}
	}
}


//========================================================================================
void BTNodeComposite::AcceptParent(BTNode* m_node)
{
	BTNode* parent = m_context->FindParent(this);

	if (parent == m_node)
		return;

    if (!m_node || m_node->AddChild(this))
        if (parent)
			parent->RemoveChild(this);
}


//========================================================================================
void BTNodeComposite::NotifyAbort()
{
	if (m_executing)
	{
		FinishAbort();
		m_children[m_activeNode]->NotifyAbort();
	}
}


//========================================================================================
void BTNodeTask::AcceptParent(BTNode* m_node)
{
    BTNode* parent = m_context->FindParent(this);

	if (parent == m_node)
		return;

    if (!m_node || m_node->AddChild(this))
        if (parent)
            parent->RemoveChild(this);
}


//========================================================================================
void BTNodeTask::NotifyAbort()
{
	if (m_executing)
		FinishAbort();
}


//========================================================================================
bool BTNodeComposite::AddChild(BTNode* node)
{
	m_children.push_back(node);
	OnChildrenChanged();
	return true;
}


//========================================================================================
void BTNodeComposite::OnChildrenChanged()
{
	if (IsExecuting())
	{
		FinishAbort();
	}

	std::sort(m_children.begin(), m_children.end(), BTNode::ComparePosition);
}


//========================================================================================
void BTNodeComposite::ForChildNode(std::function<void(BTNode*)> action)
{
    for (auto childNode : m_children)
    {
        action(childNode);
    }
}


//========================================================================================
void BTNodeComposite::ForAllChildNode(std::function<void(BTNode*)> action)
{
	for (auto childNode : m_children)
	{
		action(childNode);
		childNode->ForAllChildNode(action);
	}
}


//========================================================================================
bool BTNodeTask::Evaluate()
{
	return BTNode::Evaluate();
}


//========================================================================================
void BTNodeTask::Execute()
{
	if (!m_executing)
	{
		BeginExecute();
    
		// only evaluate when start executing to prevent always abort
		if (!Evaluate())
        {
            FinishExecute(false);
            return;
        }
    }

    DoExecute();
}


//========================================================================================
void BTNode::Tick()
{

}


//========================================================================================
void BTNode::OnBeginExecute()
{

}


//========================================================================================
void BTNode::OnFinishExecute(bool /*result*/)
{
	for (auto deco : m_decorators)
	{
		deco->OnExecuteFinished(m_result);
	}
}


//========================================================================================
void BTNode::OnAbortExecute()
{
    for (auto deco : m_decorators)
    {
        deco->OnExecuteFinished(m_result);
    }
}


//========================================================================================
bool BTNode::IsChild(BTNode*) const
{
    return false;
}


//========================================================================================
void BTNode::BeginExecute()
{
	DebugAddMessage(Stringf("Start executing: %s", m_name.c_str()), 2.0f, Rgba8::WHITE, Rgba8::WHITE);

	m_executing = true;
    m_result = EBTExecResult::UNKNOWN;

	if (this == m_context->m_root)
    {
        m_context->m_execStack.push_back(this);
	}
	else
	{
		if (m_context->m_execStack.empty() || !m_context->m_execStack.back()->IsChild(this))
        {
			DebugAddMessage(Stringf("Corrupt execution chain: %s", m_name.c_str()), 2.0f, Rgba8::WHITE, Rgba8::WHITE);
		}
		else
        {
            m_context->m_execStack.push_back(this);
		}
	}

	OnBeginExecute();
}


//========================================================================================
void BTNode::FinishExecute(bool success)
{
	DebugAddMessage(Stringf("Finish executing (%s): %s", success ? "success" : "fail", m_name.c_str()), 2.0f, Rgba8::WHITE, Rgba8::WHITE);

    m_executing = false;
    m_result = success ? EBTExecResult::SUCCESS : EBTExecResult::FAILED;
    if (m_context->m_execStack.empty() || m_context->m_execStack.back() != this)
    {
		DebugAddMessage(Stringf("Corrupt execution chain: %s", m_name.c_str()), 2.0f, Rgba8::WHITE, Rgba8::WHITE);
    }
    else
    {
        m_context->m_execStack.pop_back();
    }
	OnFinishExecute(success);
}


//========================================================================================
void BTNode::FinishAbort()
{
	DebugAddMessage(Stringf("Abort executing: %s", m_name.c_str()), 2.0f, Rgba8::WHITE, Rgba8::WHITE);

	m_executing = false;
    m_result = EBTExecResult::ABORTED;
    if (m_context->m_execStack.empty() || m_context->m_execStack.back() != this)
    {
		DebugAddMessage(Stringf("Corrupt execution chain: %s", m_name.c_str()), 2.0f, Rgba8::WHITE, Rgba8::WHITE);
	}
    else
    {
        m_context->m_execStack.pop_back();
    }
	OnAbortExecute();
}


//========================================================================================
void BTNode::ForAllChildNode(std::function<void(BTNode*)> action)
{

}


//========================================================================================
void BTNode::ForAllDecorator(std::function<void(BTDecorator*)> action)
{
	for (auto deco : m_decorators)
		action(deco);
}


//========================================================================================
bool BTNode::MoveUpDecorator(BTDecorator* deco)
{
	for (int i = 0; i < m_decorators.size(); i++)
	{
		if (deco == m_decorators[i])
		{
			if (i == 0)
				return false;
			m_decorators[i] = m_decorators[i - 1ULL];
            m_decorators[i - 1ULL] = deco;
            m_decorators[i]->m_order++;
            m_decorators[i - 1ULL]->m_order--;
			return true;
		}
	}
	return false;
}


//========================================================================================
bool BTNode::MoveDownDecorator(BTDecorator* deco)
{
    for (int i = 0; i < m_decorators.size(); i++)
    {
        if (deco == m_decorators[i])
        {
            if ((i + 1ULL) == m_decorators.size())
                return false;
            m_decorators[i] = m_decorators[i + 1ULL];
            m_decorators[i + 1ULL] = deco;
			m_decorators[i]->m_order--;
			m_decorators[i + 1ULL]->m_order++;
            return true;
        }
    }
    return false;
}


//========================================================================================
bool BTNode::RemoveDecorator(BTDecorator* deco)
{
	auto ite = std::find(m_decorators.begin(), m_decorators.end(), deco);
	if (ite != m_decorators.end())
	{
		m_decorators.erase(ite);
		delete deco;
		return true;
	}
	return false;
}


//========================================================================================
BTDecorator* BTNode::FindDecorator(const UUID& uuid)
{
	for (auto& deco : m_decorators)
		if (deco->m_uuid == uuid)
			return deco;
	return nullptr;
}


//========================================================================================
bool BTNode::ComparePosition(const BTNode* a, const BTNode* b)
{
	return a->m_position.x < b->m_position.x;
}


//========================================================================================
BTNode::BTNode(const std::string& name)
	: BTBase(name)
{
}


//========================================================================================
BTNode::~BTNode()
{
	for (auto& deco : m_decorators)
		delete deco;
}


//========================================================================================
bool BTNode::Evaluate()
{
	for (BTDecorator* decorator : m_decorators)
		if (!decorator->CheckCondition())
        {
			DebugAddMessage(Stringf("Evaluation failed: %s, %s", m_name.c_str(), decorator->m_name.c_str()), 2.0f, Rgba8::WHITE, Rgba8::WHITE);

            return false;
		}
	return true;
}


//========================================================================================
void BTNodeCompSequence::Execute()
{
	if (!m_executing)
    {
        BeginExecute();

        if (!Evaluate())
        {
			FinishExecute(false);
			return;
		}

		if (m_children.empty())
		{
			FinishExecute(true);
			return;
		}

		for (auto& child : m_children)
		{
			child->ResetState();
		}

		m_activeNode = 0;

		m_children[m_activeNode]->Execute();
		return;
	}
	else
	{
		if (m_children[m_activeNode]->IsExecuting())
		{
			m_children[m_activeNode]->Execute();
			return;
		}
		else if (!m_children[m_activeNode]->IsSuccess())
		{
			FinishExecute(false);
			return;
		}
		else if (++m_activeNode < m_children.size())
		{
			m_children[m_activeNode]->Execute();
			return;
		}
		else
		{
			FinishExecute(true);
			m_activeNode = 0;
			return;
		}
	}
}


//========================================================================================
BTContext::BTContext(DataRegistry& registry)
	: m_root(new BTNodeRoot())
	, m_table(registry)
{
	m_root->m_context = this;
}


//========================================================================================
BTContext::~BTContext()
{
	for (auto& node : m_nodes)
        delete node;
    delete m_root;
}


//========================================================================================
void BTContext::Execute()
{
	m_aborting = false;

    std::function<void(BTNode* node)> func = [&](auto node)
    {
		for (auto deco : node->m_decorators)
			deco->Tick();
    };

    func(m_root);
	m_root->ForAllChildNode(func);

	if (m_aborting)
	{
		m_aborting = false;

		while (!m_execStack.empty())
		{
			m_execStack.back()->FinishAbort();
		}
	}

	m_root->Execute();
}


//========================================================================================
void BTContext::RefreshOrders(BTNode* node)
{
	if (!node)
		node = m_root;

    int order = node->m_order;
	std::function<void(BTNode* node)> func = [&order](auto node)
	{
		for (auto deco : node->m_decorators)
			deco->m_order = order++;
		node->m_order = order++;
	};

	func(node);
	node->ForAllChildNode(func);
}


//========================================================================================
void BTContext::AddDecorator(BTNode* node, BTDecorator* decorator)
{
	decorator->m_owner = node;
	node->m_decorators.push_back(decorator);
}


//========================================================================================
BTNode* BTContext::FindParent(BTNode* node)
{
	if (m_root->IsChild(node))
		return m_root;

	for (auto cnode : m_nodes)
	{
		if (cnode->IsChild(node))
			return cnode;
	}
	return nullptr;
}


//========================================================================================
void BTContext::RemoveNode(BTNode* node)
{
	auto parent = FindParent(node);
	if (parent)
		parent->RemoveChild(node);

	auto ite = std::find(m_nodes.begin(), m_nodes.end(), node);
	if (ite != m_nodes.end())
		m_nodes.erase(ite);

	delete node;
}


//========================================================================================
int BTContext::FindNodeIndex(BTNode* node)
{
	if (!node)
		return -1;

	for (int i = 0; i < m_nodes.size(); i++)
		if (m_nodes[i] == node)
			return i;

	return -1;
}


//========================================================================================
BTNode* BTContext::FindNodeByIndex(int index)
{
    if (index < 0 || index >= m_nodes.size())
        return nullptr;

	return m_nodes[index];
}


//========================================================================================
BTNode* BTContext::FindNode(const UUID& uuid)
{
	for (auto& node : m_nodes)
		if (node->m_uuid == uuid)
			return node;
	return nullptr;
}


//========================================================================================
void BTContext::Load(ByteBuffer* buffer)
{
	for (auto& node : m_nodes)
	{
		delete node;
		node = nullptr;
	}

    uint32_t size;
    char version_major;
    char version_minor;
    char fourCC[4];
    buffer->Read(size);
	buffer->Read(version_major);
	buffer->Read(version_minor);
	buffer->Read(fourCC[0]);
	buffer->Read(fourCC[1]);

	if (fourCC[0] && fourCC[1])
	{
		ASSERT_OR_DIE(version_minor >= 2, "verification char failed");

	    buffer->Read(fourCC[2]);
		buffer->Read(fourCC[3]);

		bool verify = fourCC[0] == 'B' && fourCC[1] == 'T' && fourCC[2] == 'E' && fourCC[3] == 'D';
		ASSERT_OR_DIE(verify, "verification char failed");
	}
	else
	{
		ASSERT_OR_DIE(version_minor < 2, "verification char failed");
	}

	m_nodes.resize((size_t) size);

	if (version_major < 1)
	{
		DebuggerPrintf("Regenerating uuid for nodes\n");
	}

	if (version_minor >= 1)
	{
		buffer->Read(m_lod);
	}

	for (size_t i = 0; i < size; i++)
	{
		std::string name;
		ByteUtils::ReadString(buffer, name);
		m_nodes[i] = BT_NODE_REGISTRY[name]();
		m_nodes[i]->m_context = this;

		if (version_major >= 1)
		{
			buffer->Read(m_nodes[i]->m_uuid);
		}
	}

	m_root->Load(buffer);

	for (auto& node : m_nodes)
	{
		node->Load(buffer);
	}

	RefreshOrders();
}


//========================================================================================
void BTContext::Save(ByteBuffer* buffer) const
{
	uint32_t size = (uint32_t) m_nodes.size(); // do not support size larger than 32 bit int

	char empty_char = 0;

    buffer->Write(size);
	buffer->Write(BT_FORMAT_VERSION_MAJOR);
	buffer->Write(BT_FORMAT_VERSION_MINOR);
	buffer->Write('B'); // 4CC
	buffer->Write('T'); // 4CC
	buffer->Write('E'); // 4CC
	buffer->Write('D'); // 4CC

	buffer->Write(m_lod);

	for (auto node : m_nodes)
	{
		ByteUtils::WriteString(buffer, node->GetRegistryName());
		buffer->Write(node->m_uuid);
	}

	m_root->Save(buffer);

    for (auto node : m_nodes)
    {
		node->Save(buffer);
    }
}


//========================================================================================
void BTContext::AddNode(BTNode* node, BTNode* parent)
{
	if (node == m_root)
		return;

	node->m_context = this;

	if (parent)
    {
        parent->AddChild(node);
        m_evaluate = true;
    }

	m_nodes.push_back(node);
}


//========================================================================================
void BTNodeCompSelect::Execute()
{
	if (!m_executing)
	{
		BeginExecute();

        if (!Evaluate())
        {
			FinishExecute(false);
			return;
		}

		if (m_children.empty())
        {
			FinishExecute(true);
            return;
        }

        for (auto& child : m_children)
        {
            child->ResetState();
        }

		m_activeNode = 0;

		m_children[m_activeNode]->Execute();
		return;
	}
	else
	{
		if (m_children[m_activeNode]->IsExecuting())
		{
			m_children[m_activeNode]->Execute();
			return;
		}
		else if (m_children[m_activeNode]->IsSuccess())
		{
			FinishExecute(true);
			return;
		}
		else if (++m_activeNode < m_children.size())
		{
			m_children[m_activeNode]->Execute();
			return;
		}
		else
		{
			FinishExecute(false);
			m_activeNode = 0;
			return;
		}
	}
}


//========================================================================================
void BTNodeTaskDummy::DoExecute()
{
	if (m_expectResult == EBTExecResult::ABORTED)
		FinishAbort();
	else if (m_expectResult == EBTExecResult::FAILED)
		FinishExecute(false);
	else if (m_expectResult == EBTExecResult::SUCCESS)
		FinishExecute(true);
}


//========================================================================================
void BTNodeTaskDummy::CollectProps(FieldList& fields)
{
    BTNodeTask::CollectProps(fields);

    fields.emplace_back();

	Field* f;

	f = &fields.back();
    f->type = FieldType::ENUM;
	f->defaults = { "ABORT", "FAIL", "SUCCESS"};
    f->name = "Result";
	f->value = m_expectResult == EBTExecResult::ABORTED ? "ABORT" : m_expectResult == EBTExecResult::FAILED ? "FAIL" : "SUCCESS";
    f->callback = [this](auto text)
    {
		if (text == "ABORT")
			m_expectResult = EBTExecResult::ABORTED;
		else if (text == "FAIL")
			m_expectResult = EBTExecResult::FAILED;
		else if (text == "SUCCESS")
			m_expectResult = EBTExecResult::SUCCESS;

		return text;
    };
}


//========================================================================================
void BTNodeTaskWait::DoExecute()
{
	if (m_stopwatch.IsStopped())
	{
		m_stopwatch.Start(m_time);
	}
	else if (m_stopwatch.HasDurationElapsed())
	{
		m_stopwatch.Stop();
		FinishExecute(true);
	}
}


//========================================================================================
void BTNodeTaskWait::CollectProps(FieldList& fields)
{
	BTNodeTask::CollectProps(fields);


    fields.emplace_back();

	Field* f = &fields.back();

	f->type = FieldType::NUMBER;
	f->name = "Delay";
	f->value = Stringf("%.2f", m_time);
	f->callback = [this](auto text) 
	{
		m_time = (float) atof(text.c_str()); 
		return Stringf("%.2f", m_time);
	};
}


//========================================================================================
void BTDecorator::OnExecuteStarted()
{

}


//========================================================================================
void BTDecorator::OnExecuteFinished(EBTExecResult result)
{
	UNUSED(result);
	// TODO
}


//========================================================================================
BTNode* BTDecorator::GetOwner() const
{
	return m_owner;
}


//========================================================================================
void BTDecorator::Load(ByteBuffer* buffer)
{
	BTBase::Load(buffer);

	char flag = 0;

	buffer->Read(flag);

	m_abortSelf  = ((flag & 1) == 1);
	m_abortLower = ((flag & 2) == 2);
}


//========================================================================================
void BTDecorator::Save(ByteBuffer* buffer) const
{
    BTBase::Save(buffer);

    char flag = 0;
    if (m_abortSelf)
        flag |= 1;
    if (m_abortLower)
        flag |= 2;

    buffer->Write(flag);
}


//========================================================================================
bool BTDecoratorDummy::CheckCondition()
{
    return m_shouldPass;
}


//========================================================================================
void BTDecoratorDummy::CollectProps(FieldList& fields)
{
    BTDecorator::CollectProps(fields);

    fields.emplace_back();
    Field* f;

    f = &fields.back();
    f->type = FieldType::ENUM;
    f->defaults = { "TRUE", "FALSE" };
    f->name = "Should Pass";
    f->value = m_shouldPass ? "TRUE" : "FALSE";
    f->callback = [this](auto text)
    {
        if (text == "TRUE")
            m_shouldPass = true;
        else
            m_shouldPass = false;

        return text;
    };
}


//========================================================================================
const char* BTDecoratorDummy::GetRegistryName() const
{
    static const char* name = "DecoDummy";
    return name;
}


//========================================================================================
void BTDecoratorDummy::Load(ByteBuffer* buffer)
{
    BTDecorator::Load(buffer);

    buffer->Read(m_shouldPass);
}


//========================================================================================
void BTDecoratorDummy::Save(ByteBuffer* buffer) const
{
    BTDecorator::Save(buffer);

    buffer->Write(m_shouldPass);
}


//========================================================================================
bool BTDecoratorCooldown::CheckCondition()
{
    return m_timer.IsStopped() || m_timer.HasDurationElapsed();
}


//========================================================================================
void BTDecoratorCooldown::CollectProps(FieldList& fields)
{
    BTDecorator::CollectProps(fields);

    fields.emplace_back();
    Field* f;

    f = &fields.back();
    f->type = FieldType::NUMBER;
    f->name = "Duration";
    f->value = Stringf("%.2f", m_duration);
    f->callback = [this](auto text)
    {
        m_duration = (float)atof(text.c_str());

		return Stringf("%.2f", m_duration);
    };
}


//========================================================================================
const char* BTDecoratorCooldown::GetRegistryName() const
{
    static const char* name = "DecoCooldown";
    return name;
}


//========================================================================================
void BTDecoratorCooldown::OnExecuteFinished(EBTExecResult result)
{
	if (result == EBTExecResult::SUCCESS)
		m_timer.Start(m_duration);
}


//========================================================================================
void BTDecoratorCooldown::Load(ByteBuffer* buffer)
{
    BTDecorator::Load(buffer);

    buffer->Read(m_duration);

	m_timer.Stop();
}


//========================================================================================
void BTDecoratorCooldown::Save(ByteBuffer* buffer) const
{
    BTDecorator::Save(buffer);

    buffer->Write(m_duration);
}


//========================================================================================
void BTDecoratorWatchValue::Load(ByteBuffer* buffer)
{
    BTDecorator::Load(buffer);

    buffer->Read(m_checkSet);
    buffer->Read(m_reverse);
	ByteUtils::ReadString(buffer, m_key);
	ByteUtils::ReadString(buffer, m_value);

	m_keyHandle = m_owner->m_context->m_table.m_registry->GetHandle(m_key.c_str());
}


//========================================================================================
void BTDecoratorWatchValue::Save(ByteBuffer* buffer) const
{
    BTDecorator::Save(buffer);

	buffer->Write(m_checkSet);
	buffer->Write(m_reverse);
	ByteUtils::WriteString(buffer, m_key);
	ByteUtils::WriteString(buffer, m_value);
}


//========================================================================================
void BTNodeTaskDummy::Load(ByteBuffer* buffer)
{
	BTNodeTask::Load(buffer);

    buffer->Read(m_result);
}


//========================================================================================
void BTNodeTaskDummy::Save(ByteBuffer* buffer) const
{
    BTNodeTask::Save(buffer);

    buffer->Write(m_result);
}


//========================================================================================
void BTNodeTaskPlaySound::Load(ByteBuffer* buffer)
{
    BTNodeTask::Load(buffer);

	ByteUtils::ReadString(buffer, m_soundName);
    buffer->Read(m_volume);
    buffer->Read(m_speed);
}


//========================================================================================
void BTNodeTaskPlaySound::Save(ByteBuffer* buffer) const
{
    BTNodeTask::Save(buffer);

	ByteUtils::WriteString(buffer, m_soundName);
    buffer->Write(m_volume);
    buffer->Write(m_speed);
}


//========================================================================================
void BTNodeTaskFireEvent::Load(ByteBuffer* buffer)
{
    BTNodeTask::Load(buffer);

    ByteUtils::ReadString(buffer, m_eventName);
    ByteUtils::ReadString(buffer, m_eventArgs);
}


//========================================================================================
void BTNodeTaskFireEvent::Save(ByteBuffer* buffer) const
{
    BTNodeTask::Save(buffer);

    ByteUtils::WriteString(buffer, m_eventName);
    ByteUtils::WriteString(buffer, m_eventArgs);
}


//========================================================================================
void BTNodeTaskMoveTo::Load(ByteBuffer* buffer)
{
    BTNodeTask::Load(buffer);

    buffer->Read(m_radius);
    ByteUtils::ReadString(buffer, m_key);

	m_keyHandle = m_context->m_table.m_registry->GetHandle(m_key.c_str());
}


//========================================================================================
void BTNodeTaskMoveTo::Save(ByteBuffer* buffer) const
{
    BTNodeTask::Save(buffer);

    buffer->Write(m_radius);
    ByteUtils::WriteString(buffer, m_key);
}


//========================================================================================
void BTNodeTaskWait::Load(ByteBuffer* buffer)
{
    BTNodeTask::Load(buffer);

    buffer->Read(m_time);
}


//========================================================================================
void BTNodeTaskWait::Save(ByteBuffer* buffer) const
{
    BTNodeTask::Save(buffer);

    buffer->Write(m_time);
}


//========================================================================================
void BTNodeRoot::Load(ByteBuffer* buffer)
{
    BTNode::Load(buffer);

	int index;
    buffer->Read(index);
	m_entry = m_context->FindNodeByIndex(index);
}


//========================================================================================
void BTNodeComposite::Save(ByteBuffer* buffer) const
{
    BTNode::Save(buffer);

    buffer->Write(m_decoratorScoped);

	buffer->Write(m_children.size());
	for (auto child : m_children)
	{
		buffer->Write(m_context->FindNodeIndex(child));
	}
}


//========================================================================================
void BTNodeComposite::Load(ByteBuffer* buffer)
{
    BTNode::Load(buffer);

	buffer->Read(m_decoratorScoped);

	size_t size;
    buffer->Read(size);
	m_children.resize(size);

	for (int i = 0; i < size; i++)
	{
		int index;
		buffer->Read(index);
		m_children[i] = m_context->FindNodeByIndex(index);
	}
}


//========================================================================================
void BTNodeRoot::Save(ByteBuffer* buffer) const
{
    BTNode::Save(buffer);

    buffer->Write(m_context->FindNodeIndex(m_entry));
}


//========================================================================================
BTDecorator::BTDecorator(const std::string& name)
	: BTBase(name)
{

}


//========================================================================================
BTDecorator::~BTDecorator()
{

}


//========================================================================================
void BTDecorator::Tick()
{
	bool condition = CheckCondition();

	if (condition != m_cachedCondition)
	{

		m_cachedCondition = condition;

		if (m_owner->m_context->m_aborting)
			return;

		if (condition) // just became true, should interrupt lower tasks
		{
			if (m_abortLower)
            {
				if (!m_owner->m_context->m_execStack.empty() && m_owner->m_context->m_execStack.back()->m_order > m_order)
                {
					m_owner->m_context->m_aborting = true;
                }
			}
		}
		else // just became false, should interrupt current running task
		{
			if (m_abortSelf)
			{
				for (auto execNode : m_owner->m_context->m_execStack)
				{
					if (execNode == m_owner)
					{
						m_owner->m_context->m_aborting = true;
					}
				}
			}
		}
	}
}


//========================================================================================
void BTDecorator::CollectProps(FieldList& fields)
{
    BTBase::CollectProps(fields);

    Field* f;

	fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::ENUM;
    f->defaults = { "TRUE", "FALSE" };
    f->name = "Abort Self";
    f->value = m_abortSelf ? "TRUE" : "FALSE";
    f->callback = [this](auto text)
    {
        if (text == "TRUE")
            m_abortSelf = true;
        else
            m_abortSelf = false;

        return text;
    };

	fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::ENUM;
    f->defaults = { "TRUE", "FALSE" };
    f->name = "Abort Lower";
    f->value = m_abortLower ? "TRUE" : "FALSE";
    f->callback = [this](auto text)
    {
        if (text == "TRUE")
            m_abortLower = true;
        else
            m_abortLower = false;

        return text;
    };
}


//========================================================================================
BTBase::BTBase(const std::string& name)
	: m_name(name)
	, m_uuid(UUID::randomUUID())
{

}


//========================================================================================
BTBase::~BTBase()
{

}


//========================================================================================
void BTBase::CollectProps(FieldList& fields)
{
    Field* f;

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
    f->value = GetRegistryName();
    f->name = "Type";
    f->callback = [this](auto text)
    {
        return GetRegistryName();
    };

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
	f->value = m_name;
    f->name = "Name";
    f->callback = [this](auto text)
    {
		return m_name = text;
    };
}


//========================================================================================
const std::string& BTBase::GetName() const
{
	return m_name;
}


//========================================================================================
void BTBase::Load(ByteBuffer* buffer)
{
	ByteUtils::ReadString(buffer, m_name);
}


//========================================================================================
void BTBase::Save(ByteBuffer* buffer) const
{
	ByteUtils::WriteString(buffer, m_name);
}


//========================================================================================
void BTNode::Load(ByteBuffer* buffer)
{
	BTBase::Load(buffer);

	size_t size;
	buffer->Read(size);
	m_decorators.clear();
	m_decorators.reserve(size);
	for (int i = 0; i < size; i++)
	{
		std::string name;
		ByteUtils::ReadString(buffer, name);
		m_context->AddDecorator(this, BT_DECO_REGISTRY[name]());
	}

	for (auto deco : m_decorators)
	{
		deco->Load(buffer);
	}

	Vec2 position;
	buffer->Read(position);
	m_position = m_context->m_canvas.GetPointAtUV(position);
}


//========================================================================================
void BTNode::Save(ByteBuffer* buffer) const
{
	BTBase::Save(buffer);

	buffer->Write(m_decorators.size());

	for (auto deco : m_decorators)
	{
		ByteUtils::WriteString(buffer, deco->GetRegistryName());
	}

	for (auto deco : m_decorators)
	{
		deco->Save(buffer);
	}

	buffer->Write(m_context->m_canvas.GetUVForPoint(m_position));
}


//========================================================================================
void BTNodeTaskPlaySound::DoExecute()
{
	SoundID snd = g_theAudio->CreateOrGetSound(m_soundName);
	g_theAudio->StartSoundAt(snd, m_context->m_actor->GetPosition(), false, m_volume, 0.0f, 1.0f, m_speed);
}


//========================================================================================
void BTNodeTaskPlaySound::CollectProps(FieldList& fields)
{
	BTNodeTask::CollectProps(fields);

    Field* f;

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
    f->value = m_soundName;
    f->name = "Sound";
    f->callback = [this](auto text)
    {
        return m_soundName = text;
    };

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::NUMBER;
    f->value = Stringf("%.2f", m_volume);
    f->name = "Volume";
    f->callback = [this](auto text)
    {
        m_volume = (float) atof(text.c_str());

		return Stringf("%.2f", m_volume);
    };

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::NUMBER;
    f->value = Stringf("%.2f", m_speed);
    f->name = "Speed";
    f->callback = [this](auto text)
    {
        m_speed = (float)atof(text.c_str());

        return Stringf("%.2f", m_speed);
    };
}


//========================================================================================
void BTNodeTaskFireEvent::DoExecute()
{
	g_theConsole->Execute(Stringf("%s %s", m_eventName.c_str(), m_eventArgs.c_str()));
}


//========================================================================================
void BTNodeTaskFireEvent::CollectProps(FieldList& fields)
{
    BTNodeTask::CollectProps(fields);

    Field* f;

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
    f->value = m_eventName;
    f->name = "EventName";
    f->callback = [this](auto text)
    {
        return m_eventName = text;
    };

	fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
    f->value = m_eventArgs;
    f->name = "EventArgs";
    f->callback = [this](auto text)
    {
        return m_eventArgs = text;
    };
}


//========================================================================================
bool BTDecoratorWatchValue::CheckCondition()
{
	auto entry = m_owner->m_context->m_table.FindEntry(m_keyHandle);

	if (m_checkSet)
	{
		bool isSet = entry != nullptr;
		return m_reverse ? !isSet : isSet;
	}
	else
	{
		auto& value = entry ? entry->value.GetAsText() : "";
		bool equals = value == m_value;
        return m_reverse ? !equals : equals;
	}
}


//========================================================================================
void BTDecoratorWatchValue::CollectProps(FieldList& fields)
{
	BTDecorator::CollectProps(fields);

    Field* f;
    
	fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
    f->value = m_key;
    f->name = "MatchKey";
    f->callback = [this](auto text)
    {
		m_keyHandle = m_owner->m_context->m_table.m_registry->GetHandle(text.c_str());
        return m_key = text;
    };

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
    f->value = m_value;
    f->name = "MatchValue";
    f->callback = [this](auto text)
    {
        return m_value = text;
    };

	fields.emplace_back();
	f = &fields.back();
	f->type = FieldType::ENUM;
	f->defaults = { "IS SET", "EQUALS VALUE" };
	f->value = m_checkSet ? "IS SET" : "EQUALS VALUE";
	f->name = "CheckType";
	f->callback = [this](auto text)
	{
		m_checkSet = text == "IS SET";
		return text;
	};

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::ENUM;
	f->defaults = { "TRUE", "FALSE" };
    f->value = m_reverse ? "TRUE" : "FALSE";
    f->name = "Reverse";
    f->callback = [this](auto text)
    {
		m_reverse = text == "TRUE";
        return text;
    };
}



//========================================================================================
void BTNodeTaskMakeNoise::DoExecute()
{
	m_context->m_actor->m_world->AISenseMakeNoise(m_context->m_actor->GetPosition(), m_volume);
}


//========================================================================================
void BTNodeTaskMakeNoise::CollectProps(FieldList& fields)
{
    BTNodeTask::CollectProps(fields);

    Field* f;

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::NUMBER;
    f->value = Stringf("%.2f", m_volume);
    f->name = "Loudness";
    f->callback = [this](auto text)
    {
        return Stringf("%.2f", m_volume = (float)atof(text.c_str()));
    };
}


//========================================================================================
const char* BTNodeTaskMakeNoise::GetRegistryName() const
{
	static const char* name = "TaskMakeNoise";
	return name;
}


//========================================================================================
void BTNodeTaskMakeNoise::Load(ByteBuffer* buffer)
{
	BTNodeTask::Load(buffer);

	buffer->Read(m_volume);
}


//========================================================================================
void BTNodeTaskMakeNoise::Save(ByteBuffer* buffer) const
{
	BTNodeTask::Save(buffer);

	buffer->Write(m_volume);
}


//========================================================================================
bool BTDecoratorCanSee::CheckCondition()
{
	auto entry = m_owner->m_context->m_table.FindEntry(m_keyHandle);
	Actor* actor = entry ? *entry->value.GetAsActor() : nullptr;
	if (!actor)
		return false;

	Actor* owner = m_owner->m_context->m_actor;

	auto eye = owner->GetEyePosition();

	auto forward = owner->GetForward();

	if ((owner->GetPosition() - actor->GetPosition()).GetLengthSquared() > m_range * m_range)
		return m_reverse ? true : false;

	bool result = ConvertRadiansToDegrees(forward.Dot((actor->GetEyePosition() - eye).GetNormalized())) < m_angle;

	if (m_raycast && result)
	{
		auto ray = owner->m_world->FastRaycastVsTiles(owner->GetEyePosition(), actor->GetEyePosition());

		if (ray.m_hitBlock)
			result = false;
	}

	return m_reverse ? !result : result;
}


//========================================================================================
void BTDecoratorCanSee::CollectProps(FieldList& fields)
{
	BTDecorator::CollectProps(fields);


    Field* f;

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
    f->value = m_key;
    f->name = "Entity";
    f->callback = [this](auto text)
    {
        m_keyHandle = m_owner->m_context->m_table.m_registry->GetHandle(text.c_str());
        return m_key = text;
    };

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::NUMBER;
    f->value = Stringf("%.2f", m_angle);
    f->name = "Angle";
    f->callback = [this](auto text)
    {
        m_angle = (float)atof(text.c_str());

        return Stringf("%.2f", m_angle);
    };

	fields.emplace_back();
	f = &fields.back();
	f->type = FieldType::NUMBER;
	f->value = Stringf("%.2f", m_range);
	f->name = "Range";
	f->callback = [this](auto text)
	{
		m_range = (float)atof(text.c_str());

		return Stringf("%.2f", m_range);
    };

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::ENUM;
    f->defaults = { "TRUE", "FALSE" };
    f->value = m_raycast ? "TRUE" : "FALSE";
    f->name = "CheckSight";
    f->callback = [this](auto text)
    {
        m_raycast = text == "TRUE";
        return text;
    };

	fields.emplace_back();
	f = &fields.back();
	f->type = FieldType::ENUM;
	f->defaults = { "TRUE", "FALSE" };
	f->value = m_reverse ? "TRUE" : "FALSE";
	f->name = "Reverse";
	f->callback = [this](auto text)
	{
		m_reverse = text == "TRUE";
		return text;
	};
}


//========================================================================================
const char* BTDecoratorCanSee::GetRegistryName() const
{
	static const char* name = "DecoratorCanSee";
	return name;
}


//========================================================================================
void BTDecoratorCanSee::Load(ByteBuffer* buffer)
{
	BTDecorator::Load(buffer);
    ByteUtils::ReadString(buffer, m_key);
	buffer->Read(m_angle);
	buffer->Read(m_range);

	char flags = 0;
	buffer->Read(flags);
	m_reverse = (flags & 1) == 1;
	m_raycast = (flags & 2) == 2;

    m_keyHandle = m_owner->m_context->m_table.m_registry->GetHandle(m_key.c_str());
}


//========================================================================================
void BTDecoratorCanSee::Save(ByteBuffer* buffer) const
{
    BTDecorator::Save(buffer);

    ByteUtils::WriteString(buffer, m_key);
	buffer->Write(m_angle);
	buffer->Write(m_range);

    char flags = 0;
	if (m_reverse)
        flags |= 0x1;
    if (m_raycast)
        flags |= 0x2;

	buffer->Write(flags);
}


//========================================================================================
bool BTDecoratorIsInRange::CheckCondition()
{
	Vec3 target;

	auto entry = m_owner->m_context->m_table.FindEntry(m_keyHandle);

	if (!entry)
		return false;

	if (entry->value.GetType() == BTDataType::ACTOR)
	{
		Actor* actor = *entry->value.GetAsActor();
		if (!actor)
			return false;

		target = actor->GetPosition();
	}
	else
	{
		target = entry->value.GetAsVector();
	}

	Actor* owner = m_owner->m_context->m_actor;

	if ((owner->GetPosition() - target).GetLengthSquared() > m_range * m_range)
		return m_reverse ? true : false;

	return m_reverse ? false : true;
}


//========================================================================================
void BTDecoratorIsInRange::CollectProps(FieldList& fields)
{
	BTDecorator::CollectProps(fields);

    Field* f;

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
    f->value = m_key;
    f->name = "Entity";
    f->callback = [this](auto text)
    {
        m_keyHandle = m_owner->m_context->m_table.m_registry->GetHandle(text.c_str());
        return m_key = text;
    };

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::NUMBER;
    f->value = Stringf("%.2f", m_range);
    f->name = "Range";
    f->callback = [this](auto text)
    {
        m_range = (float)atof(text.c_str());

        return Stringf("%.2f", m_range);
    };

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::ENUM;
    f->defaults = { "TRUE", "FALSE" };
    f->value = m_reverse ? "TRUE" : "FALSE";
    f->name = "Reverse";
    f->callback = [this](auto text)
    {
        m_reverse = text == "TRUE";
        return text;
    };
}


//========================================================================================
const char* BTDecoratorIsInRange::GetRegistryName() const
{
    static const char* name = "DecoInRange";
    return name;
}


//========================================================================================
void BTDecoratorIsInRange::Load(ByteBuffer* buffer)
{
    BTDecorator::Load(buffer);
    ByteUtils::ReadString(buffer, m_key);
    buffer->Read(m_range);
	buffer->Read(m_reverse);

    m_keyHandle = m_owner->m_context->m_table.m_registry->GetHandle(m_key.c_str());
}


//========================================================================================
void BTDecoratorIsInRange::Save(ByteBuffer* buffer) const
{
    BTDecorator::Save(buffer);

    ByteUtils::WriteString(buffer, m_key);
    buffer->Write(m_range);
	buffer->Write(m_reverse);
}


//========================================================================================
void BTNodeTaskAttack::DoExecute()
{
	auto entry = m_context->m_table.FindEntry(m_keyHandle);

	Actor* actor = entry ? *entry->value.GetAsActor() : nullptr;

	if (!actor)
	{
		FinishExecute(false);
		return;
	}

	Health* health = actor->GetComponent<Health>();

	if (health)
		health->Damage(m_damage);
	
	FinishExecute(true);
}


//========================================================================================
void BTNodeTaskAttack::CollectProps(FieldList& fields)
{
    BTNodeTask::CollectProps(fields);

    Field* f;

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
    f->value = m_key;
    f->name = "TargetKey";
    f->callback = [this](auto text)
    {
        m_keyHandle = m_context->m_table.m_registry->GetHandle(text.c_str());
        return m_key = text;
    };

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::NUMBER;
    f->value = Stringf("%.2f", m_damage);
    f->name = "Damage";
    f->callback = [this](auto text)
    {
        return Stringf("%.2f", m_damage = (float)atof(text.c_str()));
    };
}


//========================================================================================
const char* BTNodeTaskAttack::GetRegistryName() const
{
    static const char* name = "TaskAttack";
    return name;
}


//========================================================================================
void BTNodeTaskAttack::Load(ByteBuffer* buffer)
{
    BTNodeTask::Load(buffer);

    buffer->Read(m_damage);
    ByteUtils::ReadString(buffer, m_key);

    m_keyHandle = m_context->m_table.m_registry->GetHandle(m_key.c_str());
}


//========================================================================================
void BTNodeTaskAttack::Save(ByteBuffer* buffer) const
{
    BTNodeTask::Save(buffer);

    buffer->Write(m_damage);
    ByteUtils::WriteString(buffer, m_key);
}


//========================================================================================
void BTNodeTaskRandomPoint::DoExecute()
{
	Actor* actor = m_context->m_actor;

	if (!actor)
	{
		FinishExecute(false);
		return;
	}

	static RandomNumberGenerator RNG;

	Vec3 random = Vec3::ZERO;

	int i = 0;

    while (true)
    {
		if (i++ > 100)
		{
			FinishExecute(false);
			return;
		}

        do
        {
            random.x = (RNG.RollRandomFloatZeroToOne() * 2 - 1) * m_range;
            random.y = (RNG.RollRandomFloatZeroToOne() * 2 - 1) * m_range;
        } while (random.GetLengthSquared() > m_range * m_range);

		auto loc = actor->GetPosition() + random;

		if (!actor->m_world->m_navMesh->QueryAccessible(IntVec2((int)loc.x, (int)loc.y), false))
			continue;

        auto entry = m_context->m_table.SetEntry(m_keyHandle);

        entry->value.Set(actor->GetPosition() + random);
		break;
	}

    FinishExecute(true);
}


//========================================================================================
void BTNodeTaskRandomPoint::CollectProps(FieldList& fields)
{
    BTNodeTask::CollectProps(fields);

    Field* f;

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
    f->value = m_targetKey;
    f->name = "TargetKey";
    f->callback = [&](auto text)
    {
        m_keyHandle = m_context->m_table.m_registry->GetHandle(text.c_str());
        return m_targetKey = text;
    };

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::NUMBER;
    f->value = Stringf("%.2f", m_range);
    f->name = "Range";
    f->callback = [this](auto text)
    {
        return Stringf("%.2f", m_range = (float)atof(text.c_str()));
    };
}


//========================================================================================
const char* BTNodeTaskRandomPoint::GetRegistryName() const
{
    static const char* name = "TaskRandomPoint";
    return name;
}


//========================================================================================
void BTNodeTaskRandomPoint::Load(ByteBuffer* buffer)
{
    BTNodeTask::Load(buffer);

    buffer->Read(m_range);
    ByteUtils::ReadString(buffer, m_targetKey);

    m_keyHandle = m_context->m_table.m_registry->GetHandle(m_targetKey.c_str());
}


//========================================================================================
void BTNodeTaskRandomPoint::Save(ByteBuffer* buffer) const
{
    BTNodeTask::Save(buffer);

    buffer->Write(m_range);
    ByteUtils::WriteString(buffer, m_targetKey);
}


//========================================================================================
void BTNodeTaskKeepDistance::DoExecute()
{
    Actor* actor = m_context->m_actor;

	if (m_moving)
	{
		if (!m_context->m_contorller->IsMoving())
		{
			FinishExecute(true);
			return;
		}
		else
		{
			return;
		}
	}

    if (!actor)
    {
        FinishExecute(false);
        return;
    }

	Vec3 direction;
	Vec3 position;

	if (m_keyHandle == INVALID_DATAENTRY_HANDLE)
	{
		FinishExecute(false);
		return;
	}

	auto entry = m_context->m_table.FindEntry(m_keyHandle);

	if (!entry)
	{
		FinishExecute(false);
		return;
	}

	if (entry->value.GetType() == BTDataType::ACTOR)
	{
		auto target = *entry->value.GetAsActor();

		if (!target)
		{
			FinishExecute(false);
			return;
		}

		direction = target->GetForward();
		direction.z = 0;
		direction.NormalizeAndGetPreviousLength();

		position = target->GetPosition();
	}
	else if (entry->value.GetType() == BTDataType::VECTOR)
	{
		position = entry->value.GetAsVector();

		direction = (actor->GetPosition() - position).GetNormalized();
	}
	else
	{
		FinishExecute(false);
		return;
	}

	for (float angle = 0; angle < 90; angle += 10)
	{
		auto target1 = direction.GetRotatedAboutZDegrees(angle) * m_range;

		auto result1 = actor->m_world->FastRaycastVsTiles(position, target1);

		if (!result1.m_hitBlock)
		{
			m_context->m_contorller->MoveTo(target1);
			m_moving = true;
			return;
		}

		auto target2 = direction.GetRotatedAboutZDegrees(-angle) * m_range;

        auto result2 = actor->m_world->FastRaycastVsTiles(position, target2);

        if (!result2.m_hitBlock)
        {
            m_context->m_contorller->MoveTo(target1);
            m_moving = true;
            return;
        }
	}

	FinishExecute(false);
	return;
}


//========================================================================================
void BTNodeTaskKeepDistance::CollectProps(FieldList& fields)
{
    BTNodeTask::CollectProps(fields);

    Field* f;

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
    f->value = m_targetKey;
    f->name = "TargetKey";
    f->callback = [&](auto text)
    {
        m_keyHandle = m_context->m_table.m_registry->GetHandle(text.c_str());
        return m_targetKey = text;
    };

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::NUMBER;
    f->value = Stringf("%.2f", m_range);
    f->name = "Range";
    f->callback = [this](auto text)
    {
        return Stringf("%.2f", m_range = (float)atof(text.c_str()));
    };
}


//========================================================================================
const char* BTNodeTaskKeepDistance::GetRegistryName() const
{
    static const char* name = "TaskKeepDistance";
    return name;
}


//========================================================================================
void BTNodeTaskKeepDistance::Load(ByteBuffer* buffer)
{
    BTNodeTask::Load(buffer);

    buffer->Read(m_range);
    ByteUtils::ReadString(buffer, m_targetKey);

    m_keyHandle = m_context->m_table.m_registry->GetHandle(m_targetKey.c_str());
}


//========================================================================================
void BTNodeTaskKeepDistance::Save(ByteBuffer* buffer) const
{
    BTNodeTask::Save(buffer);

    buffer->Write(m_range);
    ByteUtils::WriteString(buffer, m_targetKey);
}


//========================================================================================
void BTNodeTaskSetValue::DoExecute()
{
	if (m_keyHandle == INVALID_DATAENTRY_HANDLE)
	{
		FinishExecute(false);
		return;
	}

	if (m_fromKeyHandle == INVALID_DATAENTRY_HANDLE)
	{
		m_fromKeyHandle = m_context->m_table.m_registry->GetHandle(m_fromKey.c_str());
	}

	auto fromEntry = m_context->m_table.FindEntry(m_fromKeyHandle);

	if (!fromEntry)
	{
		m_context->m_table.UnsetEntry(m_keyHandle);
	}
	else
	{
		auto entry = m_context->m_table.SetEntry(m_keyHandle);

		if (entry)
		{
			entry->value = fromEntry->value;
		}

	}
    FinishExecute(true);
}


//========================================================================================
void BTNodeTaskSetValue::CollectProps(FieldList& fields)
{
    BTNodeTask::CollectProps(fields);

    Field* f;

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
    f->value = m_key;
    f->name = "TargetKey";
    f->callback = [&](auto text)
    {
        m_keyHandle = m_context->m_table.m_registry->GetHandle(text.c_str());
        return m_key = text;
    };

    fields.emplace_back();
    f = &fields.back();
    f->type = FieldType::TEXT;
    f->value = m_fromKey;
    f->name = "FromKey";
    f->callback = [&](auto text)
    {
        m_fromKeyHandle = m_context->m_table.m_registry->GetHandle(text.c_str());
        return m_fromKey = text;
    };
}


//========================================================================================
const char* BTNodeTaskSetValue::GetRegistryName() const
{
    static const char* name = "TaskSetValue";
    return name;
}


//========================================================================================
void BTNodeTaskSetValue::Load(ByteBuffer* buffer)
{
    BTNodeTask::Load(buffer);

    ByteUtils::ReadString(buffer, m_key);
    ByteUtils::ReadString(buffer, m_fromKey);

    m_keyHandle = m_context->m_table.m_registry->GetHandle(m_key.c_str());
    m_fromKeyHandle = m_context->m_table.m_registry->GetHandle(m_fromKey.c_str());
}


//========================================================================================
void BTNodeTaskSetValue::Save(ByteBuffer* buffer) const
{
    BTNodeTask::Save(buffer);

    ByteUtils::WriteString(buffer, m_key);
    ByteUtils::WriteString(buffer, m_fromKey);
}

