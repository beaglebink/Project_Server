#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Utility/AlsGameplayTags.h"
#include "A_EnemyManager.generated.h"

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

private:
	TMap<FGameplayTag, int32> EnemyTypeCounts;

public:
	UFUNCTION(BlueprintCallable, Category = "Enemy Manager")
	void RegisterEnemy(FGameplayTag EnemyType);

	UFUNCTION(BlueprintCallable, Category = "Enemy Manager")
	void UnregisterEnemy(FGameplayTag EnemyType);
	
	UFUNCTION(BlueprintCallable, Category = "Enemy Manager")
	int32 GetEnemyCount(FGameplayTag EnemyType) const;
};
