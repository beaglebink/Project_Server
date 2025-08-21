#include "DictionaryObjectBase.h"
#include "DictionaryManager.h"
#include "TimerManager.h"

ADictionaryManager* ADictionaryOnjectBase::ManagerInstance = nullptr;

void ADictionaryOnjectBase::BeginPlay()
{
	Super::BeginPlay();
	
    if (!ManagerInstance)
    {
        if (HasAuthority())
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ManagerInstance = GetWorld()->SpawnActor<ADictionaryManager>(ADictionaryManager::StaticClass(), FTransform::Identity, Params);
        }
    }
}

    void ADictionaryOnjectBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
    {
		Super::EndPlay(EndPlayReason);
		if (ManagerInstance)
		{
			// Удаляем менеджер, если он был создан
			if (HasAuthority())
			{
				ManagerInstance->Destroy();
				ManagerInstance = nullptr;
			}
		}
    }
