#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DictionaryObjectBase.h"
#include "Engine/DataTable.h"
#include "KeysActor.h"
#include "FVariantProperty.h"
#include "NiagaraSystem.h"

#include "DictionaryManager.generated.h"

class ADictionaryObjectBase;

USTRUCT(BlueprintType)
struct FDictionaryLinkStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)

	FVector StartLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)

	FVector EndLocation;
};

USTRUCT(BlueprintType)
struct FEffectsStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* KeyActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString KeyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* PropertyActor;

	// Оператор сравнения
	bool operator==(const FEffectsStruct& Other) const
	{
		return KeyActor == Other.KeyActor && KeyName == Other.KeyName && PropertyActor == Other.PropertyActor;
	}
};

FORCEINLINE uint32 GetTypeHash(const FEffectsStruct& Struct)
{
	uint32 Hash = GetTypeHash(Struct.KeyActor);
	Hash = HashCombine(Hash, GetTypeHash(Struct.KeyName));
	Hash = HashCombine(Hash, GetTypeHash(Struct.PropertyActor));

	return Hash;
}


UCLASS()
class FPSKITALSREFACTORED_API ADictionaryManager : public AActor
{
    GENERATED_BODY()
public:
	ADictionaryManager();

	void BeginPlay() override;

	void RegisterKeysActor(ADictionaryObjectBase* KeysActor);
	void UnregisterKeysActor(ADictionaryObjectBase* KeysActor);

	void RegisterPropertyActor(ADictionaryObjectBase* PropertyActor);
	void UnregisterPropertyActor(ADictionaryObjectBase* PropertyActor);

	void InitializeKeyActor(AKeysActor* KeyActor);

	AActor* VerifyProperty(const FString& PropertyType, const FString& PropertyValue, FVariantProperty& VariantProperty);

	UFUNCTION(BlueprintCallable)
	void ConnectActorChain(AActor* Start, const FString& Key, AActor* Finish, AActor* Old);

private:
	UFUNCTION()
	void Initialize();

protected:

private:
	UPROPERTY()
	TArray<ADictionaryObjectBase*> RegisteredKeysActors;
	UPROPERTY()
	TArray<ADictionaryObjectBase*> RegisteredPropertyActors;

	UDataTable* DictionaryActorsTable;

	UNiagaraSystem* ConnectionEffect;

	//TArray<UNiagaraComponent*> ActiveEffects;
	TMap<FEffectsStruct, UNiagaraComponent*> ActiveEffects;
};