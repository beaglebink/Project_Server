#include "DictionaryObjectBase.h"
#include "DictionaryManager.h"
#include "TimerManager.h"

//static TWeakObjectPtr<ADictionaryManager> ManagerInstance;
TWeakObjectPtr<ADictionaryManager> ADictionaryObjectBase::ManagerInstance = nullptr;


void ADictionaryObjectBase::BeginPlay()
{
	Super::BeginPlay();
	
    if (!IsValid(ManagerInstance.Get()))
    {
        if (HasAuthority())
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            ADictionaryManager* SpawnedManager = GetWorld()->SpawnActor<ADictionaryManager>(
                ADictionaryManager::StaticClass(),
                FTransform::Identity,
                Params
            );

            ManagerInstance = SpawnedManager;
        }
    }
}

    void ADictionaryObjectBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
    {
		Super::EndPlay(EndPlayReason);
    }

    ADictionaryManager* ADictionaryObjectBase::GetManager(UWorld* World)
{
    if (!IsValid(ManagerInstance.Get()))
    {
        if (World && World->GetAuthGameMode())
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            ManagerInstance = World->SpawnActor<ADictionaryManager>(ADictionaryManager::StaticClass(), FTransform::Identity, Params);
        }
    }
    return ManagerInstance.Get();
}
