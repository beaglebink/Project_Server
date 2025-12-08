#include "A_EnemyManager.h"

AA_EnemyManager::AA_EnemyManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AA_EnemyManager::BeginPlay()
{
	Super::BeginPlay();
}

void AA_EnemyManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_EnemyManager::RegisterEnemy(FGameplayTag EnemyType)
{
	++EnemyTypeCounts.FindOrAdd(EnemyType);

	//  MoonDoggies effect
	if (EnemyType.MatchesTag(FGameplayTag::RequestGameplayTag("Enemy.Ghost")))
	{
		++GhostEnemyTypes.FindOrAdd(EnemyType);
		OnGhostsTypesNumberChange.Broadcast(GhostEnemyTypes.Num());
	}
}

void AA_EnemyManager::UnregisterEnemy(AActor* Enemy, FGameplayTag EnemyType)
{
	if (int32* CountPtr = EnemyTypeCounts.Find(EnemyType))
	{
		--(*CountPtr);
		if (*CountPtr <= 0)
		{
			EnemyTypeCounts.Remove(EnemyType);
		}
	}
	//  BugZapperCoat effect
	ZappedEnemies.Remove(Enemy);

	//  MoonDoggies effect
	if (int32* CountPtr = GhostEnemyTypes.Find(EnemyType))
	{
		--(*CountPtr);
		if (*CountPtr <= 0)
		{
			GhostEnemyTypes.Remove(EnemyType);
		}
	}
	OnGhostsTypesNumberChange.Broadcast(GhostEnemyTypes.Num());
}

int32 AA_EnemyManager::GetEnemyCount(FGameplayTag EnemyType) const
{
	if (const int32* CountPtr = EnemyTypeCounts.Find(EnemyType))
	{
		return *CountPtr;
	}
	return 0;
}