#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WS_ActorRegistry.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UWS_ActorRegistry : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Actor Registry")
    void RegisterActor(AActor* Actor, const FGuid& Guid);

	UFUNCTION(BlueprintCallable, Category = "Actor Registry")
    void UnregisterActor(const FGuid& Guid);

	UFUNCTION(BlueprintCallable, Category = "Actor Registry")
    AActor* FindActorByGuid(const FGuid& Guid) const;

private:
    UPROPERTY()
    TMap<FGuid, AActor*> GuidToActorMap;
};
