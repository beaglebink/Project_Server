#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DictionaryObjectBase.h"
#include "IPropertySupport.h"

#include "KeysActor.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API AKeysActor : public ADictionaryOnjectBase, public IPropertySupport
{
    GENERATED_BODY()

public:
    void BeginPlay() override;
    void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void ApplyProperty(const FName PropertyName, const FString& Value) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keys")
	FName KeyActorName; // Имя актора ключа, который будет применен

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keys")
	TArray<FName> Keys; // Список ключей, которые могут быть применены
};