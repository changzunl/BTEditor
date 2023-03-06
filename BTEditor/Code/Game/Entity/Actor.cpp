#include "Game/Entity/Actor.hpp"

#include "Game/Entity/Faction.hpp"
#include "Game/Framework/Game.hpp"
#include "Game/Scene/Scene.hpp"
#include "Game/World/World.hpp"
#include "Game/Entity/ActorDefinition.hpp"
#include "Game/Entity/AI.hpp"
#include "Game/Entity/Player.hpp"
#include "Engine/Core/RgbaF.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/VertexFormat.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Animation/SkeletalMesh.hpp"
#include "Engine/Core/ByteBuffer.hpp"
#include "Engine/Core/FileUtils.hpp"

extern RandomNumberGenerator rng;

const char* g_factionNames[]  = { "Neutral", "Marine", "Demon" };
const Rgba8 g_factionColors[] = { Rgba8(0, 0, 255), Rgba8(0, 255, 0), Rgba8(255, 0, 0) };

const bool RENDER_CYLINDER_MESH = false;

constexpr bool DRAW_ENTITY_WIREFRAME = true;

Actor::Actor(World* map, const SpawnInfo* spawnInfo, const ActorUID& uid)
    : m_world(map)
	, m_definition(spawnInfo->m_definition)
	, m_type(1 << m_definition->m_faction)
	, m_faction(m_definition->m_faction)
	, m_lifetimeStopwatch(m_world->GetClock(), 999999999.0)
	, m_uid(uid)
{
	// initialize components
	m_transform.m_position = spawnInfo->m_position;
	m_transform.m_orientation = spawnInfo->m_orientation;

	if (m_definition->m_aiEnabled)
	{
		auto ai = new AI();
		ai->RunBehaviorTree(m_definition->m_aiBehavior);
		m_defaultController = ai;
	}
	else
	{
		m_defaultController = new Controller();
	}

	m_defaultController->Possess(this);

	Physics* physics = new Physics(*this);
	physics->m_physicsHeight = m_definition->m_physicsHeight;
	physics->m_physicsRadius = m_definition->m_physicsRadius;
	physics->m_velocity = spawnInfo->m_velocity;
	physics->m_simulated = m_definition->m_simulated;
	physics->m_drag = m_definition->m_drag;
	physics->SetMode(m_definition->m_flying ? PhysicsMode::FLYING : PhysicsMode::WALKING);
	AddComponent(physics);

	if (RENDER_CYLINDER_MESH && m_definition->m_name == "Player")
	{
		StaticMeshComp* mesh = new StaticMeshComp(*this);
		mesh->m_solidColor = g_factionColors[m_definition->m_faction];
		VertexBufferBuilder builder;
		builder.Start(VertexFormat::GetDefaultFormat_Vertex_PNCU(), 0);
		if (physics->m_physicsHeight > 0.0f && physics->m_physicsRadius > 0.0f)
		{
			if (spawnInfo->m_isProjectile)
			{
				BuildZCylinder(builder, Vec3(), physics->m_physicsRadius, physics->m_physicsHeight, Rgba8::WHITE);
			}
			else
			{
				BuildZCylinder(builder, Vec3(), physics->m_physicsRadius, physics->m_physicsHeight, Rgba8::WHITE);
				BuildXCone(builder, Vec3(m_definition->m_physicsRadius, 0.0f, m_definition->m_eyeHeight), m_definition->m_physicsRadius * 0.2f, m_definition->m_physicsRadius * 0.2f, Rgba8::WHITE);
			}
			mesh->Upload(builder, g_theRenderer);
		}
		builder.Reset();
		m_debugMesh = mesh;
		AddComponent(mesh);
	}
	else if (!m_definition->m_mesh.empty())
    {
        SkeletalMeshComp* mesh = new SkeletalMeshComp(*this);
        mesh->m_solidColor = g_factionColors[m_definition->m_faction];
        
		SkeletalMesh* skel = new SkeletalMesh();
		ByteBuffer buffer;
		FileReadToBuffer(buffer, m_definition->m_mesh);
		uint64_t endian;
		buffer.Read(endian);
		skel->ReadBytes(&buffer);

		mesh->m_transform.m_scale *= 0.01f;
		mesh->Upload(skel);
		if (!m_definition->m_meshTexture.empty())
			mesh->m_texture = g_theRenderer->CreateOrGetTextureFromFile(m_definition->m_meshTexture.c_str());
		mesh->m_transform.m_orientation.m_yawDegrees = 180;
        AddComponent(mesh);
    }

	if (m_definition->m_health > 0)
	{
		Health* health = new Health(*this, m_definition->m_health);
		AddComponent(health);
	}

	if (m_definition->m_visible)
	{
		// TODO appearance
	}

	if (!m_definition->m_sounds.IsEmpty())
	{
		SoundSource* sound = new SoundSource(*this);
		AddComponent(sound);
	}

	// query components
	m_physics = GetComponent<Physics>();
	m_sound = GetComponent<SoundSource>();
}

Actor::~Actor()
{
	for (auto& comp : m_comps)
		delete comp;
	m_comps.clear();

	delete m_defaultController;
}

void Actor::OnCollideWithEntity(Actor* entityToCollide)
{
	if (IsProjectile() && entityToCollide->IsProjectile())
		return;

	if (m_definition->m_impulseOnCollide)
	{
		entityToCollide->m_physics->AddImpulse((entityToCollide->m_transform.m_position - m_transform.m_position).GetNormalized() * m_definition->m_impulseOnCollide);
	}

	if (m_definition->m_damageOnCollide != FloatRange::ZERO && *m_projectileOwner != entityToCollide)
	{
		if (!entityToCollide->IsDead())
		{
			Health* health = entityToCollide->GetComponent<Health>();
			if (health)
			{
				health->Damage(rng.RollRandomFloatInRange(m_definition->m_damageOnCollide.m_min, m_definition->m_damageOnCollide.m_max), false, *m_projectileOwner);
				entityToCollide->CallEvent("effect", "damaged");
				if (entityToCollide->IsDead())
				{
					Actor* actor = *m_projectileOwner;
					if (actor && actor->m_controller)
						actor->m_controller->m_killCount++;
				}
			}
		}
	}

	if (m_definition->m_dieOnCollide)
	{
		Die();
	}
}

void Actor::Render() const
{
	Player* cameraEntity = g_theGame->GetCurrentScene()->GetWorldCameraEntity();
	if (m_controller == static_cast<Controller*>(cameraEntity) && !cameraEntity->RenderOwner())
    {
        m_controller->OnRenderActor();
        return;
	}

	if (DRAW_ENTITY_WIREFRAME)
	{
		static VertexList verts;

		verts.clear();

		AABB3 aabb;
		aabb.SetDimensions(Vec3(m_definition->m_physicsRadius * 2, m_definition->m_physicsRadius * 2, m_definition->m_physicsHeight));
		aabb.SetCenter(Vec3(0, 0, aabb.GetDimensions().z * 0.5f));
		AddVertsForAABB3D(verts, aabb, Rgba8::WHITE);

		g_theRenderer->BindShader(nullptr);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetDepthMask(true);
		g_theRenderer->SetDepthTest(DepthTest::ALWAYS);
		g_theRenderer->SetCullMode(CullMode::NONE);

		g_theRenderer->SetFillMode(FillMode::WIREFRAME);
		g_theRenderer->SetModelMatrix(Mat4x4::CreateTranslation3D(m_transform.m_position));
		g_theRenderer->DrawVertexArray(verts);
		g_theRenderer->SetModelMatrix(Mat4x4::IDENTITY);
		g_theRenderer->SetFillMode(FillMode::SOLID);
		g_theRenderer->SetCullMode(CullMode::BACK);

		DebugAddWorldWireCylinder(aabb.GetPointAtUV(Vec3(0.5f, 0.5f, 0.0f)) + m_transform.m_position, aabb.GetPointAtUV(Vec3(0.5f, 0.5f, 1.0f)) + m_transform.m_position, aabb.GetDimensions().x, 0.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::ALWAYS);
		DebugAddWorldLine(m_transform.m_position, m_transform.m_position + m_physics->m_velocity, 0.02f, 0.0f, Rgba8(255, 255, 0), Rgba8(255, 255, 0), DebugRenderMode::ALWAYS);
		DebugAddWorldLine(m_transform.m_position, m_transform.m_position + m_transform.GetForward(), 0.02f, 0.0f, Rgba8(0, 0, 255), Rgba8(0, 0, 255), DebugRenderMode::ALWAYS);
	}

	g_theRenderer->BindShader(nullptr);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMask(true);
	g_theRenderer->SetDepthTest(DepthTest::LESSEQUAL);
	for (auto& comp : GetComponents(StaticMeshComp::TYPE))
	{
		StaticMeshComp* mesh = (StaticMeshComp*)comp;
		if (mesh != m_debugMesh)
			mesh->Draw(g_theRenderer, &m_transform);
    }
    for (auto& comp : GetComponents(SkeletalMeshComp::TYPE))
    {
		SkeletalMeshComp* mesh = (SkeletalMeshComp*)comp;
        mesh->Draw(g_theRenderer, &m_transform);
    }
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);

	m_controller->OnRenderActor();
}

void Actor::Update(float deltaSeconds)
{
	if (!m_dead && m_definition->m_lifetime >= 0)
		if (m_lifetimeStopwatch.GetElapsedTime() >= m_definition->m_lifetime)
			Die();

	m_prevTransform = m_transform;

	Vec3 position = m_transform.m_position;

	if (m_dead && m_lifetimeStopwatch.HasDurationElapsed())
	{
		m_garbage = true;
		return;
	}

	m_controller->Update(deltaSeconds);
	for (auto& comp : m_tickableComps)
	{
		comp->Update(deltaSeconds);
	}

	if (m_definition->m_collidesWithWorld)
	{
		// int coordX = Floor(m_transform.m_position.x);
		// int coordY = Floor(m_transform.m_position.y);
		// TODO repair coords for collision
	}
}

void Actor::Die()
{
	if (!m_dead)
	{
		m_dead = true;
		m_lifetimeStopwatch.Start(m_definition->m_corpseLifetime);

		if (m_physics)
			m_physics->m_simulated = false;

		CallEvent("effect", "died");
	}
}

bool Actor::IsOutOfWorld() const
{
	return m_transform.m_position.z < 0.0f || m_transform.m_position.z >= 256.0f;
}

bool Actor::IsOfTypeMask(int entityTypeMask) const
{
	return (m_type & entityTypeMask) == m_type;
}

Vec3 Actor::GetPosition() const
{
	return m_transform.m_position;
}

EulerAngles Actor::GetOrientation() const
{
	return m_transform.m_orientation;
}

Vec3 Actor::GetEyePosition() const
{
	return m_transform.m_position + Vec3(0.0f, 0.0f, m_definition->m_eyeHeight);
}

Vec3 Actor::GetForward() const
{
	return m_transform.m_orientation.GetVectorXForward();
}

bool Actor::IsProjectile() const
{
	return m_projectileOwner != ActorUID::INVALID();
}

int Actor::GetType() const
{
	return m_type;
}

const ActorUID& Actor::GetUID() const
{
	return m_uid;
}

void Actor::AddComponent(EntityComponent* component)
{
	m_comps.push_back(component);

	if (component->m_tickable)
		m_tickableComps.push_back(component);
}

EntityComponent* Actor::GetComponent(EntityComponentType type)
{
	for (EntityComponent* comp : m_comps)
	{
		if (type == comp->m_type)
			return comp;
	}
	return nullptr;
}

void Actor::OnPossessed(Controller* controller)
{
	m_controller = controller;
}

void Actor::OnUnpossessed(Controller* controller)
{
	if (m_controller == controller)
		m_controller = m_defaultController;
}

void Actor::MoveInDirection(Vec3 direction, float speed)
{
	Vec3 position = m_transform.m_position;

	m_transform.m_position += (float)m_lifetimeStopwatch.m_clock->GetDeltaTime() * speed * direction;

	if (m_definition->m_collidesWithWorld)
	{
		// int coordX = Floor(m_transform.m_position.x);
		// int coordY = Floor(m_transform.m_position.y);
		// TODO repair position for collision
	}
}

void Actor::TurnInDirection(EulerAngles direction)
{
	m_transform.m_orientation.m_yawDegrees   = GetTurnedTowardDegrees(m_transform.m_orientation.m_yawDegrees  , direction.m_yawDegrees  , m_definition->m_turnSpeed);
	m_transform.m_orientation.m_pitchDegrees = GetTurnedTowardDegrees(m_transform.m_orientation.m_pitchDegrees, direction.m_pitchDegrees, m_definition->m_turnSpeed);
	m_transform.m_orientation.m_rollDegrees  = GetTurnedTowardDegrees(m_transform.m_orientation.m_rollDegrees , direction.m_rollDegrees , m_definition->m_turnSpeed);
}

void Actor::CallEvent(const char* type, const char* name)
{
	if (_stricmp(type, "effect") == 0)
		HandleEventEffect(name);
}

void Actor::HandleEventEffect(const char* name)
{
	if (_stricmp(name, "attack") == 0)
	{
		if (m_sound)
			m_sound->PlaySound("Attack");
	}
	if (_stricmp(name, "damaged") == 0)
	{
		if (m_sound)
			m_sound->PlaySound("Hurt");
	}
	if (_stricmp(name, "died") == 0)
	{
		if (m_sound)
			m_sound->PlaySound("Death");
	}
}

std::vector<EntityComponent*> Actor::GetComponents(EntityComponentType type) const
{
	std::vector<EntityComponent*> result;
	result.reserve(m_comps.size());
	for (EntityComponent* comp : m_comps)
	{
		if (type == comp->m_type)
			result.push_back(comp);
	}
	return result;
}

void Actor::RotateToFace(Vec3 direction)
{
	m_transform.m_orientation.m_pitchDegrees = ConvertRadiansToDegrees(asinf(direction.z));

	direction.z = 0;
	direction.NormalizeAndGetPreviousLength();

	m_transform.m_orientation.m_yawDegrees = ConvertRadiansToDegrees(atan2f(direction.y, direction.x));

	m_transform.m_orientation.m_rollDegrees = 0;
}

