#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DictionaryObjectBase.h"

#include "PropertyActor.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API APropertyActor : public ADictionaryObjectBase
{
    GENERATED_BODY()

public:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property")
	FName TypeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property")
	FVariantProperty Property;
};