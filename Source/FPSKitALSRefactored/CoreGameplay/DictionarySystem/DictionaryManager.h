#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DictionaryObjectBase.h"
#include "Engine/DataTable.h"
#include "KeysActor.h"
#include "FVariantProperty.h"

#include "DictionaryManager.generated.h"

class ADictionaryObjectBase;

UCLASS()
class FPSKITALSREFACTORED_API ADictionaryManager : public AActor
{
    GENERATED_BODY()
public:
	void BeginPlay() override;

	void RegisterKeysActor(ADictionaryObjectBase* KeysActor);
	void UnregisterKeysActor(ADictionaryObjectBase* KeysActor);

	void RegisterPropertyActor(ADictionaryObjectBase* PropertyActor);
	void UnregisterPropertyActor(ADictionaryObjectBase* PropertyActor);

	void InitializeKeyActor(AKeysActor* KeyActor);

private:
	UFUNCTION()
	void Initialize();

private:
	UPROPERTY()
	TArray<ADictionaryObjectBase*> RegisteredKeysActors;
	UPROPERTY()
	TArray<ADictionaryObjectBase*> RegisteredPropertyActors;

	UDataTable* DictionaryActorsTable;

	//FTimerHandle TimerHandle;
};