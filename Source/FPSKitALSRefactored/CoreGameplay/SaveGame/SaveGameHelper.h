#pragma once

#include "CoreMinimal.h"
#include "SaveDataStructures.h"
#include "SaveGameHelper.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API USaveGameHelper : public UObject
{
    GENERATED_BODY()

public:
    static TArray<FActorSaveData> SerializeWorld(UWorld* World);
    static void DeserializeWorld(UWorld* World, const TArray<FActorSaveData>& SavedActors);

    static TArray<FComponentSaveData> SerializeComponents(AActor* Actor);
    static void DeserializeComponents(AActor* Actor, const TArray<FComponentSaveData>& SavedComponents);

    static void ClearWorld(UWorld* World, const TArray<FActorSaveData>& SavedActors);

    static bool IsActorEligibleForSave(const AActor* Actor);
    static bool IsComponentEligibleForSave(const UActorComponent* Comp);

    static FString SerializeActor(AActor* Actor);
    static void DeserializeActor(AActor* Actor, const FString& JsonString);

    static FString SerializeComponent(UActorComponent* Component);
    static void DeserializeComponent(UActorComponent* Component, const FString& JsonString);
};

