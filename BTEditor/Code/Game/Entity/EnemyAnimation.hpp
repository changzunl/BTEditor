#include "Game/Entity/Actor.hpp"
#include "Game/Entity/Components.hpp"

#include "Engine/Animation/Animation.hpp"
#include "Engine/Animation/SkeletalMesh.hpp"

class EnemyAnimation
{

private:
    Actor*                m_owner;
    SkeletalMesh*         m_mesh;
    Pose                  m_pose;
                          
    Animation*            m_animWalk = nullptr;
    Animation*            m_animAttack = nullptr;

public:
    void update();

    void updateAnimation();
};

