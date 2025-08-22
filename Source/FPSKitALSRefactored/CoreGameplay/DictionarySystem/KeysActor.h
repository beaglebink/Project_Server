#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DictionaryObjectBase.h"
#include "IPropertySupport.h"

#include "KeysActor.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API AKeysActor : public ADictionaryObjectBase, public IPropertySupport
{
	GENERATED_BODY()

public:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent /*BlueprintImplementableEvent*/, Category = "KeysActor")
	void ApplyProperty(const FName PropertyName, const FVariantProperty Value) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keys")
	FName KeyActorName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keys")
	TMap<FName, FName> KeyValues;

};