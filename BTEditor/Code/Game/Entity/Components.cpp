#include "Game/Entity/Components.hpp"

#include "Game/Framework/GameCommon.hpp"
#include "Game/Entity/Actor.hpp"
#include "Game/World/World.hpp"
#include "Game/Entity/ActorDefinition.hpp"
#include "Game/Entity/AI.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Core/XmlUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/SpriteDefinition.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

constexpr bool PREVENTATIVE_PHYSICS_ON = true;

std::vector<VertexFormat> g_SkeletalShaderLayout;
Shader* g_SkeletalShader;

void EntityComponent::Initialize()
{
    auto& layout = g_SkeletalShaderLayout;
    auto& shader = g_SkeletalShader;

    if (!shader)
    {
        layout.resize(6);
        layout[0].AddElement(VertexFormatElement::POSITION3F);
        layout[1].AddElement(VertexFormatElement::NORMAL3F);
        layout[2].AddElement(VertexFormatElement::COLOR4UB);
        layout[3].AddElement(VertexFormatElement::TEXCOORD2F);
        layout[4].AddElement(VertexFormatElement("BONE_IDS", VertexFormatElement::Format::UB4, VertexFormatElement::Semantic::GENERIC));
        layout[5].AddElement(VertexFormatElement("BONE_WEIGHTS", VertexFormatElement::Format::FLOAT4, VertexFormatElement::Semantic::GENERIC));
        for (int i = 0; i < layout.size(); i++)
            layout[i].m_bufferSlot = i;

        std::string fileName = "Data/Shaders/SkeletalLit.hlsl";
        std::string source;
        if (FileReadToString(source, fileName) < 0)
            ERROR_AND_DIE(Stringf("Shader source file not found: ", fileName.c_str()));
        shader = g_theRenderer->CreateShader("SkeletalLit", source, layout);

        g_theRenderer->InitializeCustomConstantBuffer(CUSTOM_CONSTANT_BUFFER_SLOT_START, sizeof(SkeletonConstants));
    }
}

EntityComponent::EntityComponent(EntityComponentType type, Actor& actor)
	: m_type(type)
	, m_actor(actor)
{
}

EntityComponent::~EntityComponent()
{
}

const EntityComponentType Physics::TYPE = EC_TYPE_PHYSICS;

Physics::Physics(Actor& actor)
	: EntityComponent(TYPE, actor)
{
	m_tickable = true;
	SetMode(m_mode);
}

void Physics::Update(float deltaSeconds)
{
	Chunk* chunk = m_actor.m_world->GetChunkAtPosition(m_actor.m_transform.m_position);

	if (m_simulated && chunk)
	{

		AddForce(-m_drag * m_velocity);
		m_velocity += m_acceleration * deltaSeconds;
		if (!m_flying)
			m_velocity.z -= m_gravity * deltaSeconds;
		// MoveTo(m_actor.m_transform.m_position + Vec3(m_velocity.x, m_velocity.y, 0) * deltaSeconds);
		MoveTo(m_actor.m_transform.m_position + Vec3(m_velocity.x, 0, 0) * deltaSeconds);
		MoveTo(m_actor.m_transform.m_position + Vec3(0, m_velocity.y, 0) * deltaSeconds);
 		MoveTo(m_actor.m_transform.m_position + Vec3(0, 0, m_velocity.z) * deltaSeconds);
		m_acceleration = Vec3::ZERO;

		m_actor.m_transform.m_orientation.m_yawDegrees   += m_angluerVelocity.m_yawDegrees * deltaSeconds;
		m_actor.m_transform.m_orientation.m_pitchDegrees += m_angluerVelocity.m_pitchDegrees * deltaSeconds;
		m_actor.m_transform.m_orientation.m_rollDegrees  += m_angluerVelocity.m_rollDegrees * deltaSeconds;

	}
}

void Physics::MoveTo(Vec3 position)
{
	if (m_noclip || !PREVENTATIVE_PHYSICS_ON)
	{
		m_actor.m_transform.m_position = position;
		return;
	}

	Vec3 prevPosition = m_actor.m_transform.m_position;
	float sizeHalfXY = m_physicsRadius;
	float sizeZ = m_physicsHeight;
	AABB3 prevBox = AABB3(Vec3(prevPosition.x - sizeHalfXY, prevPosition.y - sizeHalfXY, prevPosition.z), Vec3(prevPosition.x + sizeHalfXY, prevPosition.y + sizeHalfXY, prevPosition.z + sizeZ));

	Vec3 movement = position - prevPosition;
	float moveLen = movement.GetLength();
	float impact = moveLen;
	Vec3 normal;

	for (Vec3 offsetV : { Vec3(0, 0, 0), Vec3(0, 0, sizeZ * 0.5f), Vec3(0, 0, sizeZ) })
	{
		for (Vec3 offsetH : { Vec3(+sizeHalfXY, +sizeHalfXY, 0), Vec3(-sizeHalfXY, -sizeHalfXY, 0), Vec3(+sizeHalfXY, -sizeHalfXY, 0), Vec3(-sizeHalfXY, +sizeHalfXY, 0) })
		{
			Vec3 rayStart = prevPosition + offsetV + offsetH;
			Vec3 rayEnd = rayStart + movement;
			if (prevBox.IsPointInside(rayEnd))
				continue;

			auto result = m_actor.m_world->FastRaycastVsTiles(rayStart, rayEnd).m_result;
			if (result.DidImpact())
			{
				impact = Min(impact, result.GetImpactDistance());
				normal = result.GetImpactNormal();
			}
		}
	}

	if (impact != moveLen)
	{
		movement *= (impact - 0.00001f) / moveLen;
		m_actor.m_transform.m_position = prevPosition + movement;
		if (normal.x != 0)
			m_velocity.x = 0;
		if (normal.y != 0)
			m_velocity.y = 0;
		if (normal.z != 0)
			m_velocity.z = 0;
	}
	else
	{
		m_actor.m_transform.m_position = position;
	}
}

void Physics::AddForce(const Vec3& force)
{
	m_acceleration += force;
}

void Physics::AddImpulse(const Vec3& impulse)
{
	m_velocity += impulse;
}

void Physics::SwitchMode()
{
	m_mode = PhysicsMode(int(m_mode) + 1);
	if (m_mode == PhysicsMode::SIZE)
		m_mode = PhysicsMode(0);
	SetMode(m_mode);
}

void Physics::SetMode(PhysicsMode mode)
{
	m_mode = mode;
	switch (mode)
	{
	case PhysicsMode::WALKING:
		m_flying = false;
		m_noclip = false;
		break;
	case PhysicsMode::FLYING:
		m_flying = true;
		m_noclip = false;
		break;
	case PhysicsMode::NOCLIP:
		m_flying = true;
		m_noclip = true;
		break;
	}
}

bool Physics::IsFlying() const
{
	return m_flying;
}

bool Physics::IsNoclip() const
{
	return m_noclip;
}

#include "Engine/Renderer/DebugRender.hpp"

bool Physics::IsOnGround() const
{
	World* world = m_actor.m_world;

	Vec3& position = m_actor.m_transform.m_position;
	float sizeHalfXY = m_physicsRadius;

	for (Vec3 offset : { Vec3(+sizeHalfXY, +sizeHalfXY, 0), Vec3(-sizeHalfXY, -sizeHalfXY, 0) , Vec3(+sizeHalfXY, -sizeHalfXY, 0) , Vec3(-sizeHalfXY, +sizeHalfXY, 0) })
	{
		Vec3 pos = position + offset;
		auto result = world->FastRaycastVsTiles(pos, pos - Vec3(0.0f, 0.0f, 0.1f));
		DebugAddWorldRay(pos, Vec3(0.0f, 0.0f, -1.f), 0.1f, result.m_result, 0.01f, 0.0f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::XRAY);
		if (result.m_result.DidImpact())
			return true;
	}
	return false;
}

const EntityComponentType StaticMeshComp::TYPE = EC_TYPE_STATIC_MESH;

StaticMeshComp::StaticMeshComp(Actor& actor)
	: EntityComponent(TYPE, actor)
{
}

StaticMeshComp::~StaticMeshComp()
{
	Delete();
}

void StaticMeshComp::Upload(const VertexList& verts, Renderer* renderer)
{
	Delete();

	size_t bufferSize = verts.size() * sizeof(Vertex_PCU);
	m_vertCount = (int)verts.size();
	m_vbo = renderer->CreateVertexBuffer(bufferSize);
	renderer->CopyCPUToGPU(verts.data(), bufferSize, m_vbo);
}

void StaticMeshComp::Upload(VertexBufferBuilder& builder, Renderer* renderer)
{
	Delete();

    m_vertCount = (int)builder.Count();
    size_t bufferSize = m_vertCount * (size_t) builder.GetFormat().GetVertexStride();
	m_vbo = renderer->CreateVertexBuffer(bufferSize, &builder.GetFormat());
	renderer->CopyCPUToGPU(builder.Data(), bufferSize, m_vbo);
}

void StaticMeshComp::UploadIndex(const int* indices, size_t count, Renderer* renderer)
{
	if (m_ibo)
		delete m_ibo;

	m_vertCount = (int)count;
	m_ibo = renderer->CreateIndexBuffer(count * sizeof(int));
	renderer->CopyCPUToGPU(indices, count * sizeof(int), m_ibo);
}

void StaticMeshComp::Delete()
{
	if (m_vbo)
	{
		delete m_vbo;
		m_vbo = nullptr;
		m_vertCount = 0;

        if (m_ibo)
        {
            delete m_ibo;
			m_ibo = nullptr;
        }
	}
}

void StaticMeshComp::Draw(Renderer* renderer, const Transformation* transform, bool wireframe) const
{
	if (!m_enabled)
		return;

	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->BindShader(m_shader);
	if (m_ibo)
    {
        Mat4x4 mat = transform->GetMatrix();
        mat.Append(m_transform.GetMatrix());
        renderer->SetModelMatrix(mat);
        renderer->SetTintColor(wireframe ? m_wireframeColor : m_solidColor);
        renderer->SetFillMode(wireframe ? FillMode::WIREFRAME : FillMode::SOLID);
		renderer->DrawIndexedVertexBuffer(m_ibo, m_vbo, m_vertCount);
        renderer->SetFillMode(FillMode::SOLID);
	} 
	else if (m_vbo)
	{
		Mat4x4 mat = transform->GetMatrix();
		mat.Append(m_transform.GetMatrix());
		renderer->SetModelMatrix(mat);
		renderer->SetTintColor(wireframe ? m_wireframeColor : m_solidColor);
		renderer->SetFillMode(wireframe ? FillMode::WIREFRAME : FillMode::SOLID);
		renderer->DrawVertexBuffer(m_vbo, m_vertCount);
		renderer->SetFillMode(FillMode::SOLID);
	}
    g_theRenderer->BindTexture(nullptr);
    g_theRenderer->BindShader(nullptr);
}

void StaticMeshComp::DrawMesh(Renderer* renderer, bool wireframe) const
{
	if (m_ibo)
    {
        renderer->SetModelMatrix(m_transform.GetMatrix());
        renderer->SetFillMode(wireframe ? FillMode::WIREFRAME : FillMode::SOLID);
        renderer->DrawIndexedVertexBuffer(m_ibo, m_vbo, m_vertCount);
        renderer->SetFillMode(FillMode::SOLID);
	}
	else if (m_vbo)
	{
		renderer->SetModelMatrix(m_transform.GetMatrix());
		renderer->SetFillMode(wireframe ? FillMode::WIREFRAME : FillMode::SOLID);
		renderer->DrawVertexBuffer(m_vbo, m_vertCount);
		renderer->SetFillMode(FillMode::SOLID);
	}
}

const EntityComponentType SkeletalMeshComp::TYPE = EC_TYPE_SKELETAL_MESH;

SkeletalMeshComp::SkeletalMeshComp(Actor& actor)
    : EntityComponent(TYPE, actor)
	, m_watch(actor.m_world->GetClock())
{
	m_tickable = true;
}

SkeletalMeshComp::~SkeletalMeshComp()
{
    Delete();
}

void SkeletalMeshComp::Update(float deltaSeconds)
{
	if (m_animation)
	{
        auto& pose = *m_pose;
        pose = m_mesh->m_skeleton.GetPose();

        AnimationFrame frame = m_animation->Sample((float)m_watch.GetElapsedTime(), pose);
        frame.Apply(pose);
        pose.BakeLocalToComp();
        pose.BakeFromComp();
	}
}

void SkeletalMeshComp::Upload(SkeletalMesh* mesh)
{
    auto& layout = g_SkeletalShaderLayout;

    Delete();

	m_shader = g_SkeletalShader;
    m_mesh = mesh;
    m_pose = new Pose(m_mesh->m_skeleton.GetPose());

    // load mesh into GPU
    {
        VertexBuffer* vbo;

        // vbo position
        vbo = g_theRenderer->CreateVertexBuffer(mesh->m_vertices.size() * layout[0].GetVertexStride(), &layout[0]);
        g_theRenderer->CopyCPUToGPU(mesh->m_vertices.data(), mesh->m_vertices.size() * layout[0].GetVertexStride(), vbo);
		m_vbos.push_back(vbo);

        // vbo normal
        vbo = g_theRenderer->CreateVertexBuffer(mesh->m_normals.size() * layout[1].GetVertexStride(), &layout[1]);
        g_theRenderer->CopyCPUToGPU(mesh->m_normals.data(), mesh->m_normals.size() * layout[1].GetVertexStride(), vbo);
		m_vbos.push_back(vbo);

        // vbo color
        std::vector<Rgba8> colors;
        colors.resize(mesh->m_vertices.size());
        vbo = g_theRenderer->CreateVertexBuffer(colors.size() * layout[2].GetVertexStride(), &layout[2]);
        g_theRenderer->CopyCPUToGPU(colors.data(), colors.size() * layout[2].GetVertexStride(), vbo);
		m_vbos.push_back(vbo);

        // vbo texcoords
        vbo = g_theRenderer->CreateVertexBuffer(mesh->m_uvs[0].size() * layout[3].GetVertexStride(), &layout[3]);
        g_theRenderer->CopyCPUToGPU(mesh->m_uvs[0].data(), mesh->m_uvs[0].size() * layout[3].GetVertexStride(), vbo);
		m_vbos.push_back(vbo);

        // vbo boneIds
        vbo = g_theRenderer->CreateVertexBuffer(mesh->m_boneIndices.size() * layout[4].GetVertexStride(), &layout[4]);
        g_theRenderer->CopyCPUToGPU(mesh->m_boneIndices.data(), mesh->m_boneIndices.size() * layout[4].GetVertexStride(), vbo);
		m_vbos.push_back(vbo);

        // vbo boneWeights
        vbo = g_theRenderer->CreateVertexBuffer(mesh->m_boneWeights.size() * layout[5].GetVertexStride(), &layout[5]);
        g_theRenderer->CopyCPUToGPU(mesh->m_boneWeights.data(), mesh->m_boneWeights.size() * layout[5].GetVertexStride(), vbo);
		m_vbos.push_back(vbo);

        // ibo
        m_ibo = g_theRenderer->CreateIndexBuffer(sizeof(int) * mesh->m_indices.size());
        g_theRenderer->CopyCPUToGPU(mesh->m_indices.data(), sizeof(int) * mesh->m_indices.size(), m_ibo);
    }
}

void SkeletalMeshComp::Delete()
{
    {
        delete m_mesh;
		m_mesh = nullptr;
        delete m_pose;
		m_pose = nullptr;

        delete m_ibo;
		m_ibo = nullptr;
        for (auto& vbo : m_vbos)
            delete vbo;
        m_vbos.clear();
    }
}

void SkeletalMeshComp::UploadAnimation(Animation* anim)
{
	m_watch.Start(1);

	m_animation = anim;
}

void SkeletalMeshComp::DeleteAnimation()
{
	delete m_animation;
	m_animation = nullptr;
}

SkeletonConstants skelConsts;

void SkeletalMeshComp::Draw(Renderer* renderer, const Transformation* transform, bool wireframe) const
{
    if (!m_enabled)
        return;

    if (m_ibo)
    {
        Mat4x4 conv = Mat4x4(Vec3(0, 1, 0), Vec3(-1, 0, 0), Vec3(0, 0, 1), Vec3::ZERO).GetOrthonormalInverse();

        Mat4x4 mat = transform->GetMatrix();
        mat.Append(m_transform.GetMatrix());
        renderer->SetModelMatrix(mat * conv);
        renderer->SetTintColor(wireframe ? m_wireframeColor : m_solidColor);
        renderer->SetFillMode(wireframe ? FillMode::WIREFRAME : FillMode::SOLID);

        g_theRenderer->BindShader(m_shader);
        g_theRenderer->BindTexture(wireframe ? nullptr : m_texture);
        g_theRenderer->SetCustomConstantBuffer(4, m_pose->m_bakedPose.GetBuffer());
        g_theRenderer->DrawIndexedVertexBuffer(m_ibo, (int)m_vbos.size(), (VertexBuffer**)m_vbos.data(), (int)m_mesh->m_indices.size());

        g_theRenderer->BindTexture(nullptr);
        g_theRenderer->BindShader(nullptr);
        renderer->SetFillMode(FillMode::SOLID);
    }
}

void SkeletalMeshComp::DrawMesh(Renderer* renderer, bool wireframe) const
{
    if (m_ibo)
    {
        Mat4x4 conv = Mat4x4(Vec3(0, 1, 0), Vec3(-1, 0, 0), Vec3(0, 0, 1), Vec3::ZERO).GetOrthonormalInverse();

        renderer->SetModelMatrix(m_transform.GetMatrix() * conv);
        renderer->SetFillMode(wireframe ? FillMode::WIREFRAME : FillMode::SOLID);
        
		g_theRenderer->BindShader(m_shader);
        g_theRenderer->BindTexture(wireframe ? nullptr : m_texture);
        g_theRenderer->SetCustomConstantBuffer(4, m_pose->m_bakedPose.GetBuffer());
        g_theRenderer->DrawIndexedVertexBuffer(m_ibo, (int)m_vbos.size(), (VertexBuffer**)m_vbos.data(), (int)m_mesh->m_indices.size());

        g_theRenderer->BindTexture(nullptr);
        g_theRenderer->BindShader(nullptr);
		renderer->SetFillMode(FillMode::SOLID);
    }
}

const EntityComponentType Health::TYPE = EC_TYPE_HEALTH;

Health::Health(Actor& actor, float maxHealth)
	: EntityComponent(TYPE, actor)
	, m_maxHealth(maxHealth)
	, m_health(maxHealth)
{
}

void Health::Heal(float amount, bool percent /*= false*/)
{
	m_health = Clamp(m_health + (percent ? m_maxHealth : 1.0f) * amount, 0.0f, m_maxHealth);
}

void Health::Damage(float amount, bool percent /*= false*/, Actor* trueSource /*= nullptr*/)
{
	m_health = Clamp(m_health - (percent ? m_maxHealth : 1.0f) * amount, 0.0f, m_maxHealth);
	m_hurtWatch.Start(1.0);

	if (trueSource)
	{
		// TODO AI handle
	}

	if (m_health <= 0.0f)
		m_health = 1.0f; // m_actor.Die(); temp remove death
}

void Health::Reset()
{
	m_health = m_maxHealth;
	m_actor.m_dead = false;
	m_actor.m_lifetimeStopwatch.Start(999999999.0);
}

float Health::GetDamageEffect() const
{
	return (m_hurtWatch.IsStopped() || m_hurtWatch.HasDurationElapsed()) ? 0 : (1 - m_hurtWatch.GetElapsedFraction());
}

FirstPersonCamera::FirstPersonCamera()
{
}

const EntityComponentType Billboard::TYPE = EC_TYPE_BILLBOARD;

Billboard::Billboard(Actor& actor)
	: EntityComponent(TYPE, actor)
{
}

Billboard::~Billboard()
{
	Delete();
}

void Billboard::Build(Renderer* /*renderer*/)
{
// 	const ActorDefinition& def = *m_actor.m_definition;
// 
// 	Vec2 pivot = def.m_spritePivot;
// 
// 	m_transform.m_position.z = def.m_spriteSize.y * (0.5f - pivot.y);
// 	m_transform.m_position.y = def.m_spriteSize.x * (0.5f - pivot.x);
// 	m_transform.m_scale = Vec3(1.0f, def.m_spriteSize.x, def.m_spriteSize.y);
// 
// 	VertexFormat format = def.m_renderLit ? renderer->GetDefaultVF_PNCU() : renderer->GetDefaultVF_PCU();
// 
// 	m_builder.Start(format, 6);
// 	if (def.m_renderRounded)
// 	{
// 	    m_builder.begin()->pos(0.0f, -0.5f, -0.5f)->normal(0.0f, -1.0f, 0.0f)->color(Rgba8::WHITE)->tex(0.0f, 0.0f)->end();
// 	    m_builder.begin()->pos(0.0f, -0.5f,  0.5f)->normal(0.0f, -1.0f, 0.0f)->color(Rgba8::WHITE)->tex(0.0f, 1.0f)->end();
// 	    m_builder.begin()->pos(0.0f,  0.0f, -0.5f)->normal(1.0f,  0.0f, 0.0f)->color(Rgba8::WHITE)->tex(0.5f, 0.0f)->end();
// 	    m_builder.begin()->pos(0.0f,  0.0f,  0.5f)->normal(1.0f,  0.0f, 0.0f)->color(Rgba8::WHITE)->tex(0.5f, 1.0f)->end();
// 	    m_builder.begin()->pos(0.0f,  0.5f, -0.5f)->normal(0.0f,  1.0f, 0.0f)->color(Rgba8::WHITE)->tex(1.0f, 0.0f)->end();
// 	    m_builder.begin()->pos(0.0f,  0.5f,  0.5f)->normal(0.0f,  1.0f, 0.0f)->color(Rgba8::WHITE)->tex(1.0f, 1.0f)->end();
// 	}
// 	else
// 	{
// 		m_builder.begin()->pos(0.0f, -0.5f, -0.5f)->normal(1.0f, 0.0f, 0.0f)->color(Rgba8::WHITE)->tex(0.0f, 0.0f)->end();
// 		m_builder.begin()->pos(0.0f, -0.5f,  0.5f)->normal(1.0f, 0.0f, 0.0f)->color(Rgba8::WHITE)->tex(0.0f, 1.0f)->end();
// 		m_builder.begin()->pos(0.0f,  0.0f, -0.5f)->normal(1.0f, 0.0f, 0.0f)->color(Rgba8::WHITE)->tex(0.5f, 0.0f)->end();
// 		m_builder.begin()->pos(0.0f,  0.0f,  0.5f)->normal(1.0f, 0.0f, 0.0f)->color(Rgba8::WHITE)->tex(0.5f, 1.0f)->end();
// 		m_builder.begin()->pos(0.0f,  0.5f, -0.5f)->normal(1.0f, 0.0f, 0.0f)->color(Rgba8::WHITE)->tex(1.0f, 0.0f)->end();
// 		m_builder.begin()->pos(0.0f,  0.5f,  0.5f)->normal(1.0f, 0.0f, 0.0f)->color(Rgba8::WHITE)->tex(1.0f, 1.0f)->end();
// 	}
// 	m_vbo = renderer->CreateVertexBuffer(m_builder.GetBufferSize(), &format);
// 	m_builder.Upload(renderer, m_vbo);
// 
// 	unsigned int indexBuffer[12] = { 0,2,3,0,3,1,2,4,5,2,5,3 };
// 	m_ibo = renderer->CreateIndexBuffer(sizeof(indexBuffer));
// 	renderer->CopyCPUToGPU(&indexBuffer[0], sizeof(indexBuffer), m_ibo);
}

void Billboard::Delete()
{
	if (m_vbo)
	{
		delete m_vbo;
		m_vbo = nullptr;
	}
	if (m_ibo)
	{
		delete m_ibo;
		m_ibo = nullptr;
	}
}

void Billboard::SetSprite(Renderer* renderer, const SpriteDefinition* def)
{
	m_texture = &def->GetTexture();

	const AABB2& uvBox = def->GetUVs();
	float minX = uvBox.m_mins.x;
	float maxX = uvBox.m_maxs.x;
	float minY = uvBox.m_mins.y;
	float maxY = uvBox.m_maxs.y;
	float midX = Lerp(minX, maxX, 0.5f);

	m_builder.SetUV(0, minX, minY);
	m_builder.SetUV(1, minX, maxY);
	m_builder.SetUV(2, midX, minY);
	m_builder.SetUV(3, midX, maxY);
	m_builder.SetUV(4, maxX, minY);
	m_builder.SetUV(5, maxX, maxY);

	m_builder.Upload(renderer, m_vbo);
}

void Billboard::Draw(Renderer* /*renderer*/, const Transformation& /*cameraView*/, bool /*wireframe*/ /*= false*/) const
{
// 	if (!m_enabled)
// 		return;
// 
// 	if (m_vbo)
// 	{
// 		const ActorDefinition& def = *m_actor.m_definition;
// 
// 		Transform actorTransform = m_actor.m_transform;
// 		actorTransform.m_orientation.m_pitchDegrees = 0.0f;
// 
// 		if (def.m_billboardType == BillboardType::ALIGNED)
// 		{
// 			actorTransform.m_orientation.m_yawDegrees = cameraView.m_orientation.m_yawDegrees + 180;
// 		}
// 		if (def.m_billboardType == BillboardType::FACING)
// 		{
// 			EulerAngles rotation = DirectionToRotation((cameraView.m_position - actorTransform.m_position).GetNormalized());
// 			actorTransform.m_orientation.m_yawDegrees = rotation.m_yawDegrees;
// 		}
// 
// 		Mat4x4 mat = actorTransform.GetMatrix();
// 		mat.Append(m_transform.GetMatrix());
// 		renderer->SetModelMatrix(mat);
// 		renderer->SetTintColor(Rgba8::WHITE);
// 		renderer->SetDepthTest(def.m_renderDepth ? DepthTest::LESSEQUAL : DepthTest::ALWAYS);
// 		renderer->BindTexture(m_texture);
// 		DrawMesh(renderer, wireframe);
// 		renderer->SetDepthTest(DepthTest::LESSEQUAL);
// 	}
}

void Billboard::DrawMesh(Renderer* renderer, bool wireframe /*= false*/) const
{
	if (m_vbo)
	{
		renderer->SetFillMode(wireframe ? FillMode::WIREFRAME : FillMode::SOLID);
		renderer->DrawIndexedVertexBuffer(m_ibo, m_vbo, 12);
		renderer->SetFillMode(FillMode::SOLID);
	}
}

const EntityComponentType SoundSource::TYPE = EC_TYPE_SOUND_SOURCE;

SoundSource::SoundSource(Actor& entity)
	: EntityComponent(TYPE, entity)
{
}

void SoundSource::PlaySound(const char* name)
{
	const ActorDefinition& def = *m_actor.m_definition;

	const SoundClip* clip = def.m_sounds.FindSound(name);
	if (clip)
		clip->PlaySoundAt(m_actor.m_transform.m_position); // TODO play sound 3d
}

const EntityComponentType MeshAnimator::TYPE = EC_TYPE_MESH_ANIMATOR;

MeshAnimator::MeshAnimator(Actor& entity)
	: EntityComponent(TYPE, entity)
{
	m_tickable = true;
}

MeshAnimator::~MeshAnimator()
{
}

void MeshAnimator::Update(float deltaSeconds)
{
	m_time -= deltaSeconds;
	if (m_time >= 0 && m_mesh)
	{
		m_mesh->m_transform.m_position += m_velocity.m_position * deltaSeconds;
		m_mesh->m_transform.m_scale += m_velocity.m_scale * deltaSeconds;
		Vec3* meshOri = (Vec3*)&m_mesh->m_transform.m_orientation;
		Vec3* velOri = (Vec3*)&m_velocity.m_orientation;
		*meshOri += *velOri * deltaSeconds;
	}
}
