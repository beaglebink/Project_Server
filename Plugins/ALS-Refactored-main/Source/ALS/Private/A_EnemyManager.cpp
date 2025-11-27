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
}

void AA_EnemyManager::UnregisterEnemy(AAlsCharacterExample* Enemy, FGameplayTag EnemyType)
{
	if (int32* CountPtr = EnemyTypeCounts.Find(EnemyType))
	{
		--(*CountPtr);
		if (*CountPtr <= 0)
		{
			EnemyTypeCounts.Remove(EnemyType);
		}
	}

	ZappedEnemies.Remove(Enemy);
}

int32 AA_EnemyManager::GetEnemyCount(FGameplayTag EnemyType) const
{
	if (const int32* CountPtr = EnemyTypeCounts.Find(EnemyType))
	{
		return *CountPtr;
	}
	return 0;
}

