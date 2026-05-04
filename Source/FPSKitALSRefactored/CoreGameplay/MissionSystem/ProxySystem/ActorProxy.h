#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ProxyProperty.h"
#include "ProxyFunctionCall.h"
#include "ActorProxy.generated.h"

class UFloorAsset;

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UActorProxy : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY() UFloorAsset* FloorAsset;
    UPROPERTY() FName ActorName;
    UPROPERTY() TSubclassOf<AActor> ActorClass;
    UPROPERTY() TArray<FName> Tags;

    // Методы работы с прокси
    void SetProperty(FName PropertyName, const FString& Value);
    void CallFunction(FName FunctionName, const TArray<FString>& Args);
    void ApplyToActor(AActor* Target);

    UFUNCTION(BlueprintCallable) UFloorAsset* GetFloorAsset() const { return FloorAsset; }
    UFUNCTION(BlueprintCallable) FName GetActorName() const { return ActorName; }
    UFUNCTION(BlueprintCallable) bool HasTag(FName Tag) const { return Tags.Contains(Tag); }
    UFUNCTION(BlueprintCallable) bool IsOfClass(TSubclassOf<AActor> Class) const { return ActorClass == Class; }

private:
    TArray<FProxyProperty> PendingPropertyValues;
    TArray<FProxyFunctionCall> PendingCalls;
};
