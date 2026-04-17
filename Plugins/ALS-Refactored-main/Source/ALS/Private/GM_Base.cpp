#include "GM_Base.h"
#include "A_EnemyManager.h"
#include "PlayerController_I.h"

AGM_Base::AGM_Base()
{
	bUseSeamlessTravel = true;
}

void AGM_Base::NotifyLoadingStarted()
{
	OnLevelLoadingStarted.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("AGM_Base: OnLevelLoadingStarted broadcast"));
}

void AGM_Base::NotifyLoadingFinished()
{
	OnLevelLoadingFinished.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("AGM_Base: OnLevelLoadingFinished broadcast"));
}

void AGM_Base::PostSeamlessTravel()
{
	Super::PostSeamlessTravel();

	UWorld* W = GetWorld();
	if (!W) return;

	APlayerController* PC = W->GetFirstPlayerController();
	if (PC)
	{
		if (PC->GetClass()->ImplementsInterface(UPlayerController_I::StaticClass()))
		{
			IPlayerController_I::Execute_RestoreWeaponState(PC);
		}
		NotifyLoadingFinished();
	}
}

void AGM_Base::GetSeamlessTravelActorList(bool bToTransition, TArray<AActor*>& ActorList)
{
	Super::GetSeamlessTravelActorList(bToTransition, ActorList);

	APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PC)
	{
	/*
		if (APawn* Pawn = PC->GetPawn())
		{
			ActorList.AddUnique(Pawn);
		}
	*/
	}

	TArray<AActor*> AdditionalActors;
	GetAdditionalSeamlessTravelActors(AdditionalActors);
	for (AActor* A : AdditionalActors)
	{
		if (IsValid(A))
		{
			ActorList.AddUnique(A);
		}
	}

	IsSpawnPlayer = false; // Сбрасываем флаг, чтобы при следующем переходе игрок не сохранялся, если он уже был сохранён

}

void AGM_Base::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->SpawnActor<AA_EnemyManager>();
}

void AGM_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
