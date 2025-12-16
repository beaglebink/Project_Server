#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Utility/AlsGameplayTags.h"
#include "A_EnemyManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGhostsTypesNumberChange, int32, GhostTypesNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE (FOnEnemyTypeAddedRemoved);

class AAlsCharacterExample;

UCLASS()
class ALS_API AA_EnemyManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AA_EnemyManager();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	TMap<FGameplayTag, int32> EnemyTypeCounts;

public:
	UPROPERTY(BlueprintReadOnly)
	TSet<AActor*> ZappedEnemies; // BugZapperCoat effect

	UPROPERTY(BlueprintReadOnly)
	TMap<FGameplayTag, int32> GhostEnemyTypes; // MoonDoggies effect

	FOnGhostsTypesNumberChange OnGhostsTypesNumberChange;

	FOnEnemyTypeAddedRemoved OnEnemyTypeAddedRemoved;

	UFUNCTION(BlueprintCallable, Category = "Enemy Manager")
	void RegisterEnemy(FGameplayTag EnemyType);

	UFUNCTION(BlueprintCallable, Category = "Enemy Manager")
	void UnregisterEnemy(AActor* Enemy, FGameplayTag EnemyType);
	
	UFUNCTION(BlueprintCallable, Category = "Enemy Manager")
	int32 GetEnemyCount(FGameplayTag EnemyType) const;
};
