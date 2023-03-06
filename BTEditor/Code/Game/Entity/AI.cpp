#include "Game/Entity/AI.hpp"

#include "Game/Framework/GameCommon.hpp"
#include "Game/Framework/Game.hpp"
#include "Game/World/World.hpp"
#include "Game/Entity/Actor.hpp"
#include "Game/Entity/Player.hpp"
#include "Game/Entity/ActorDefinition.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Core/UUID.hpp"

#include "Game/Editor/BTNode.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/ByteBuffer.hpp"
#include "Game/Framework/GameCommon.hpp"

extern RandomNumberGenerator rng;

AI::AI()
    : m_uuid(UUID::randomUUID())
{
    s_btContexts[m_uuid] = AIContext();
}

AI::~AI()
{
    StopBehaviorTree();

    s_btContexts.erase(s_btContexts.find(m_uuid));
}

void AI::Update(float deltaSeconds)
{
	if (m_skipFrame)
	{
		m_skipFrame = false;
		return;
	}

	Actor* actor = GetActor();
	if (!actor || actor->IsDead())
		return;

    UpdateBehaviorTree(deltaSeconds);

    UpdateMovement(deltaSeconds);
}

void AI::RunBehaviorTree(const std::string& path)
{
    StopBehaviorTree();

    auto& context = s_btContexts[m_uuid];

    if (!FileExists(path))
    {
        DebugAddMessage(("File does not exist: " + path).c_str(), 2.0f, Rgba8::WHITE, Rgba8::WHITE);
        return;
    }

    context.btBuffer.Reset();
    if (FileReadToBuffer(context.btBuffer, path) < 0)
    {
        DebugAddMessage(("Failed to read file: " + path).c_str(), 2.0f, Rgba8::WHITE, Rgba8::WHITE);
        return;
    }

    m_btRegistry = new DataRegistry();
    m_btContext = context.context = new BTContext(*m_btRegistry);

    m_btRegistry->Load(&context.btBuffer);
    m_btContext->Load(&context.btBuffer);

    context.btBuffer.ResetRead();

    s_activeAIs.push_back(m_uuid);
}

void AI::StopBehaviorTree()
{
    auto ite = std::find(s_activeAIs.begin(), s_activeAIs.end(), m_uuid);
    if (ite != s_activeAIs.end())
        s_activeAIs.erase(ite);

    auto& context = s_btContexts[m_uuid];

    delete m_btContext;
    delete m_btRegistry;
	m_btContext = context.context = nullptr;
	m_btRegistry = nullptr;
}

void AI::SetMoveToRadius(float radius)
{
    m_radius = radius;
}

void AI::MoveTo(const IntVec2& goal)
{
    if (m_goal != goal)
    {
        m_goal = goal;
        delete m_pathfinder;
        m_pathfinder = g_theGame->GetCurrentMap()->m_navMesh->CreatePathfind(goal, false);
    }

    Actor* actor = GetActor();
    m_path.clear();
    IntVec2 position = {};
    position.x = (int)actor->GetPosition().x;
    position.y = (int)actor->GetPosition().y;

    m_pathfinder->GetPath(position, m_path);
}

void AI::MoveTo(const Vec3& goal)
{
    MoveTo(IntVec2(static_cast<int>(goal.x), static_cast<int>(goal.y)));
}

void AI::StopMoving()
{
    m_path.clear();
}

bool AI::IsMoving()
{
    return !m_path.empty();
}

AIContext* AI::FindContext(const AIIdentifier& id)
{
    if (UUID::invalidUUID() == id)
        return nullptr;

    auto ite = s_btContexts.find(id);
    if (ite == s_btContexts.end())
        return nullptr;
    return &ite->second;
}

AIIdentifier AI::FindActive(size_t id)
{
    if (s_activeAIs.empty())
        return UUID::invalidUUID();

    id = id % s_activeAIs.size();
    return s_activeAIs[id];
}

void AI::UpdateBehaviorTree(float deltaSeconds)
{
    if (m_btContext)
    {
        m_btContext->m_actor = GetActor();
        m_btContext->m_contorller = this;
        {
            auto entry = m_btContext->m_table.SetEntry(m_btRegistry->GetHandle("Self"));
            entry->value.Set(m_actorUID);
        }
        {
            auto entry = m_btContext->m_table.SetEntry(m_btRegistry->GetHandle("Player"));
            entry->value.Set(g_theGame->GetCurrentMap()->m_player[0]->GetActor()->GetUID());
        }

        m_btContext->m_root->Execute();
    }
}

void AI::UpdateMovement(float deltaSeconds)
{
    Actor* actor = GetActor();

    if (m_path.empty())
        return;

    Vec2 goal = Vec2(m_path.back().x + 0.5f, m_path.back().y + 0.5f);
    Vec2 toGoal = goal - Vec2(actor->GetPosition().x, actor->GetPosition().y);
    float distanceSq = toGoal.GetLengthSquared();

    if (distanceSq < m_radius * m_radius)
    {
        actor->RotateToFace(Vec3(toGoal.GetNormalized(), actor->GetPosition().z));
        m_path.clear();
        return;
    }

    Vec3 direction = Vec3(m_path.front().x + 0.5f, m_path.front().y + 0.5f, 0) - actor->GetPosition();
    direction.z = 0;
    float limit = direction.NormalizeAndGetPreviousLength();

    actor->RotateToFace(direction);
    DebugAddMessage(Stringf("Adjust facing: %.2f, %.2f, %.2f", actor->m_transform.m_orientation.m_yawDegrees, actor->m_transform.m_orientation.m_pitchDegrees, 0.f), 0, Rgba8::WHITE, Rgba8::WHITE);

    if (limit < 0.25f)
    {
        m_path.erase(m_path.begin());
    }
    else
    {
        Vec3 offset = direction * deltaSeconds * actor->m_definition->m_walkSpeed;

        if (offset.GetLengthSquared() > limit * limit)
            offset = offset.GetNormalized() * limit;

        actor->m_transform.m_position += offset;
    }
}

std::map<UUID, AIContext> AI::s_btContexts;

std::vector<AIIdentifier> AI::s_activeAIs;

AIContext::AIContext()
    : uuid(UUID::invalidUUID())
{

}
