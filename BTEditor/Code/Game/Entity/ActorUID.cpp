#include "Game/Entity/ActorUID.hpp"

#include "Game/Framework/GameCommon.hpp"
#include "Game/Framework/Game.hpp"
#include "Game/World/World.hpp"


bool ActorUID::IsValid() const
{
    return GetMap()->IsEntityUIDValid(*this);
}

Actor* ActorUID::operator*() const
{
	return GetMap()->GetEntityFromUID(*this);
}

World* ActorUID::GetMap()
{
	return g_theGame->GetCurrentMap();
}

