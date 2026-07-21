#include "Cooking/A_Dishes.h"
#include "GameFramework/Character.h"

AA_Dishes::AA_Dishes()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AA_Dishes::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

}

void AA_Dishes::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!StaticMesh->IsSimulatingPhysics())
	{
		StaticMesh->SetWorldRotation(FMath::RInterpTo(StaticMesh->GetComponentRotation(), IsValid(AttachedMesh) ? FRotator(0.0f, AttachedMesh->GetComponentRotation().Yaw, 0.0f) : FRotator::ZeroRotator, DeltaTime, 0.5f));
	}
}

void AA_Dishes::BeginPlay()
{
	Super::BeginPlay();

}

void AA_Dishes::Destroyed()
{
	Super::Destroyed();

}

void AA_Dishes::AttachDishToHand(ACharacter* PlayerCharacter, FName SocketName)
{
	if (!PlayerCharacter)
	{
		return;
	}

	AttachedMesh = PlayerCharacter->GetMesh();
	if (!AttachedMesh)
	{
		return;
	}

	StaticMesh->SetSimulatePhysics(false);
	StaticMesh->AttachToComponent(AttachedMesh, FAttachmentTransformRules::KeepWorldTransform, SocketName);

	GetWorldTimerManager().SetTimer(AttachTimerHandle, [this, SocketName]()
		{
			if (AttachedMesh && AttachedMesh->DoesSocketExist(SocketName))
			{
				if (GetActorLocation().Equals(AttachedMesh->GetSocketLocation(SocketName), 1.0f))
				{
					GetWorldTimerManager().ClearTimer(AttachTimerHandle);
					StaticMesh->AttachToComponent(AttachedMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Dish attached to hand"));
				}
				SetActorLocation(FMath::VInterpTo(GetActorLocation(), AttachedMesh->GetSocketLocation(SocketName), GetWorld()->GetDeltaSeconds(), 0.5f));
			}
		}, 0.01f, true);
}

void AA_Dishes::DetachDishFromHand()
{
	AttachedMesh = nullptr;
	GetWorldTimerManager().ClearTimer(AttachTimerHandle);
	StaticMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	StaticMesh->SetSimulatePhysics(true);
}

void AA_Dishes::RotateDish(float AngleDelta)
{
	StaticMesh->AddLocalRotation(FRotator(AngleDelta * 10.0f, 0.0f, 0.0f));
}

void AA_Dishes::TossDish()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Tossing dish"));
}
