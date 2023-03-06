#include "Game/Entity/Controller.hpp"
#include "Game/World/NavMesh.hpp"
#include "Engine/Core/Stopwatch.hpp"

#include "Engine/Core/UUID.hpp"
#include "Engine/Core/ByteBuffer.hpp"

#include <string>
#include <map>


class DataRegistry;
class BTContext;

typedef UUID AIIdentifier;

struct AIContext
{
	AIContext();

	AIIdentifier uuid;
	ByteBuffer   btBuffer;
	BTContext*   context = nullptr;
};

class AI : public Controller
{
public:
	AI();
	virtual ~AI();

	virtual void Update( float deltaSeconds ) override;

	void RunBehaviorTree(const std::string& path);
	void StopBehaviorTree();

	void SetMoveToRadius(float radius);
	void MoveTo(const IntVec2& goal);
	void MoveTo(const Vec3& goal);
	void StopMoving();
	bool IsMoving();

	static AIContext* FindContext(const AIIdentifier& id);
	static AIIdentifier FindActive(size_t id);

private:
	void UpdateBehaviorTree(float deltaSeconds);
	void UpdateMovement(float deltaSeconds);

public:
	Stopwatch m_meleeStopwatch;
	Stopwatch m_updateStopwatch;

private:
	static std::map<UUID, AIContext> s_btContexts;
	static std::vector<AIIdentifier> s_activeAIs;

	const AIIdentifier   m_uuid;
	DataRegistry*        m_btRegistry = nullptr;
	BTContext*           m_btContext  = nullptr;

	NavMeshInst*         m_pathfinder = nullptr;
    std::vector<IntVec2> m_path;
	IntVec2              m_goal;
	float                m_radius;
};

