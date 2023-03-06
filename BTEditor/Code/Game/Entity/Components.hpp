#pragma once

#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/Mat4x4.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Transformation.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/VertexFormat.hpp"
#include "Engine/Animation/SkeletalMesh.hpp"
#include "Engine/Animation/Animation.hpp"
#include "Engine/Core/Stopwatch.hpp"
#include <vector>

class Actor;
class WeaponDefinition;
class Renderer;
class VertexBuffer;
class IndexBuffer;
class Shader;
class Texture;
typedef std::vector<Vertex_PCU> VertexList;
typedef std::vector<const WeaponDefinition*> WeaponDefList;

typedef int EntityComponentType;

constexpr EntityComponentType EC_TYPE_TRANSFORM = 0;
constexpr EntityComponentType EC_TYPE_PHYSICS = 2;
constexpr EntityComponentType EC_TYPE_STATIC_MESH = 3;
constexpr EntityComponentType EC_TYPE_BILLBOARD = 4;
constexpr EntityComponentType EC_TYPE_SPRITE_ANIMATION = 5;
constexpr EntityComponentType EC_TYPE_HEALTH = 6;
constexpr EntityComponentType EC_TYPE_WEAPON_INVENTORY = 7;
constexpr EntityComponentType EC_TYPE_SOUND_SOURCE = 8;
constexpr EntityComponentType EC_TYPE_MESH_ANIMATOR = 9;
constexpr EntityComponentType EC_TYPE_SKELETAL_MESH = 10;

#define UNUSED(x) (void)(x);

class EntityComponent
{
public:
	static void Initialize();

	EntityComponent(EntityComponentType type, Actor& entity);
	virtual ~EntityComponent();

	virtual void Update(float deltaSeconds) { UNUSED(deltaSeconds); }
	virtual void Render() const {}

public:
	const EntityComponentType    m_type;
	Actor&                       m_actor;
	bool                         m_enabled = true;
	bool                         m_tickable = false;
};


class FirstPersonCamera
{
public:
	FirstPersonCamera();

public:
	Camera    m_camera;
	float     m_fov     = 90.0f;
};


enum class PhysicsMode
{
	WALKING,
	FLYING,
	NOCLIP,
	SIZE,
};


class Physics : public EntityComponent
{
public:
	static const EntityComponentType TYPE;
	Physics(Actor& entity);

	void Update(float deltaSeconds) override;
	void MoveTo(Vec3 newPosition);
	void AddForce(const Vec3& force);
	void AddImpulse(const Vec3& impulse);
	void SwitchMode();
	void SetMode(PhysicsMode mode);
	bool IsFlying() const;
	bool IsNoclip() const;
	bool IsOnGround() const;

public:
	float       m_physicsRadius = 1.0f;
	float       m_physicsHeight = 1.0f;
	bool        m_simulated = false;
	float       m_gravity = 98.f;
	Vec3        m_velocity;
	Vec3        m_acceleration;
	EulerAngles m_angluerVelocity;
	float       m_drag = 0.0f;
	float       m_maxAngularVel = 360.0f;

private:
	bool        m_flying = false;
	bool        m_noclip = false;
	PhysicsMode m_mode = PhysicsMode::NOCLIP;
};


class StaticMeshComp : public EntityComponent
{
public:
	static const EntityComponentType TYPE;
	StaticMeshComp(Actor& entity);
	~StaticMeshComp();

	void Upload(const VertexList& verts, Renderer* renderer);
	void Upload(VertexBufferBuilder& builder, Renderer* renderer);
	void UploadIndex(const int* indices, size_t count, Renderer* renderer);
	void Delete();

	void Draw(Renderer* renderer, const Transformation* transform = nullptr, bool wireframe = false) const;
	void DrawMesh(Renderer* renderer, bool wireframe = false) const;

public:
	Transformation     m_transform;
	Texture*           m_texture                      = nullptr;
	Shader*            m_shader                       = nullptr;
	VertexBuffer*      m_vbo                          = nullptr;
	IndexBuffer*       m_ibo                          = nullptr;
	int                m_vertCount                    = 0;
				       
	Rgba8              m_solidColor;
	Rgba8              m_wireframeColor;
};


class SkeletalMeshComp : public EntityComponent
{
public:
    static const EntityComponentType TYPE;
	SkeletalMeshComp(Actor& entity);
    ~SkeletalMeshComp();

    void Update(float deltaSeconds) override;
    void Upload(SkeletalMesh* mesh);
    void Delete();

    void UploadAnimation(Animation* anim);
    void DeleteAnimation();

    void Draw(Renderer* renderer, const Transformation* transform = nullptr, bool wireframe = false) const;
    void DrawMesh(Renderer* renderer, bool wireframe = false) const;

public:
    Transformation     m_transform;
    Texture* m_texture = nullptr;
    Shader* m_shader = nullptr;
	Stopwatch m_watch;

    SkeletalMesh* m_mesh = nullptr;
    Animation* m_animation = nullptr;
    mutable Pose* m_pose = nullptr;
    std::vector<VertexBuffer*> m_vbos;
    IndexBuffer* m_ibo = nullptr;

    Rgba8              m_solidColor;
    Rgba8              m_wireframeColor;
};


class MeshAnimator : public EntityComponent
{
public:
	static const EntityComponentType TYPE;
	MeshAnimator(Actor& entity);
	~MeshAnimator();

	void Update(float deltaSeconds) override;

public:
	float                  m_time = 0.0f;
	Transformation         m_velocity;
	StaticMeshComp*        m_mesh = nullptr;
};

class IndexBuffer;
class Texture;
class SpriteDefinition;

class Billboard : public EntityComponent
{
public:
	static const EntityComponentType TYPE;
	Billboard(Actor& entity);
	~Billboard();

	void Build(Renderer* renderer);
	void Delete();

	void SetSprite(Renderer* renderer, const SpriteDefinition* def);

	void Draw(Renderer* renderer, const Transformation& cameraView, bool wireframe = false) const;
	void DrawMesh(Renderer* renderer, bool wireframe = false) const;

public:
	Transformation              m_transform;
	Texture*                    m_texture = nullptr;
	VertexBufferBuilder         m_builder;
	IndexBuffer*                m_ibo = nullptr;
	VertexBuffer*               m_vbo = nullptr;
};


class Health : public EntityComponent
{
public:
	static const EntityComponentType TYPE;
	Health(Actor& entity, float maxHealth);

	void Heal(float amount, bool percent = false);
	void Damage(float amount, bool percent = false, Actor* trueSource = nullptr);
	void Reset();
	float GetDamageEffect() const;

public:
	bool         m_invulnerable = false;
	float        m_maxHealth;
    float        m_health;
    Stopwatch    m_hurtWatch;
};


class SoundSource : public EntityComponent
{
public:
	static const EntityComponentType TYPE;
	SoundSource(Actor& entity);

	void PlaySound(const char* name);

public:
};
