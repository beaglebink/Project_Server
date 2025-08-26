#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FVariantProperty.h"

#include "DictionaryObjectBase.generated.h"

class ADictionaryManager;

UCLASS()
class FPSKITALSREFACTORED_API ADictionaryObjectBase : public AActor
{
    GENERATED_BODY()

public:

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	FName KeyActorName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property")
    FVariantProperty Property;

protected:
    static ADictionaryManager* ManagerInstance;
};



USTRUCT(BlueprintType)
struct FDictionaryActorStruct : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ActorID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PropertyName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVariantProperty PropertyValue;
};