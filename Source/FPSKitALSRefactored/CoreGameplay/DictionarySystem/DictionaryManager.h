#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DictionaryObjectBase.h"

#include "DictionaryManager.generated.h"


UCLASS()
class FPSKITALSREFACTORED_API ADictionaryManager : public AActor
{
    GENERATED_BODY()
public:
	void RegisterKeysActor(ADictionaryOnjectBase* KeysActor);
	void UnregisterKeysActor(ADictionaryOnjectBase* KeysActor);

	void RegisterPropertyActor(ADictionaryOnjectBase* PropertyActor);
	void UnregisterPropertyActor(ADictionaryOnjectBase* PropertyActor);

private:
	UPROPERTY()
	TArray<ADictionaryOnjectBase*> RegisteredKeysActors;
	UPROPERTY()
	TArray<ADictionaryOnjectBase*> RegisteredPropertyActors;
};