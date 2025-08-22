#include "DictionaryObjectBase.h"
#include "DictionaryManager.h"
#include "TimerManager.h"

ADictionaryManager* ADictionaryObjectBase::ManagerInstance = nullptr;

void ADictionaryObjectBase::BeginPlay()
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

    void ADictionaryObjectBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
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
