#include "SaveSubsystems/WS_ActorRegistry.h"

void UWS_ActorRegistry::RegisterActor(AActor* Actor, const FGuid& Guid)
{
	GuidToActorMap.Add(Guid, Actor);
}

void UWS_ActorRegistry::UnregisterActor(const FGuid& Guid)
{
	GuidToActorMap.Remove(Guid);
}

AActor* UWS_ActorRegistry::FindActorByGuid(const FGuid& Guid) const
{
	AActor* FoundActor = GuidToActorMap.Find(Guid) ? *GuidToActorMap.Find(Guid) : nullptr;
	return IsValid(FoundActor) ? FoundActor : nullptr;
}
