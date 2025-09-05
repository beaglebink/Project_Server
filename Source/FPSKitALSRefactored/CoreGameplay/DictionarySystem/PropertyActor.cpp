#include "PropertyActor.h"
#include "DictionaryManager.h"

void APropertyActor::BeginPlay()
{
	Super::BeginPlay();

    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
        {
            ADictionaryManager* Manager = ADictionaryObjectBase::ManagerInstance.Get();

            if (IsValid(Manager))
            {
                Manager->RegisterPropertyActor(this);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("PropertyActor: ManagerInstance is still null or invalid after tick!"));
            }
        });
}

void APropertyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (IsValid(ManagerInstance.Get()))
	{
		ManagerInstance->UnregisterPropertyActor(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PropertyActor: ManagerInstance is null during EndPlay!"));
	}
}
