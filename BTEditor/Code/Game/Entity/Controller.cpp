#include "Game/Entity/Controller.hpp"

#include "Game/Framework/GameCommon.hpp"
#include "Game/Framework/Game.hpp"
#include "Game/World/World.hpp"
#include "Game/Entity/Actor.hpp"

Controller::Controller()
{
}

Controller::~Controller()
{

}

void Controller::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

}

void Controller::OnRenderActor()
{

}

void Controller::Possess(Actor* actor)
{
	Actor* previous = GetActor();
	if (previous)
		previous->OnUnpossessed(this);

	if (actor)
	{
		m_actorUID = actor->GetUID();
		actor->OnPossessed(this);
	}
	else
	{
		m_actorUID = ActorUID::INVALID();
	}
	m_skipFrame = true;
}

Actor* Controller::GetActor() const
{
	return *m_actorUID;
}

