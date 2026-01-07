#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaveGameHelper.generated.h"

USTRUCT()
struct FComponentSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    FString UniqueID;

    UPROPERTY()
    FString ClassName;

    UPROPERTY()
    FTransform Transform;

    UPROPERTY()
    FString ParentID;

    UPROPERTY()
    FString SerializedData;
};

UCLASS()
class USaveGameHelper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    static FString SerializeActor(AActor* Actor);
    static void DeserializeActor(AActor* Actor, const FString& JsonString);

    static FString SerializeComponent(UActorComponent* Component);
    static void DeserializeComponent(UActorComponent* Component, const FString& JsonString);

    static TArray<FComponentSaveData> SerializeComponents(AActor* Actor);
    static void DeserializeComponents(AActor* Actor, const TArray<FComponentSaveData>& SavedComponents);
};