#include "Cooking/A_Dishes.h"
#include "GameFramework/Character.h"
#include "Components/SphereComponent.h"
#include "Cooking/A_Cookable.h"

AA_Dishes::AA_Dishes()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionShape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CollisionShape"));

	CollisionShape->SetupAttachment(RootComponent);

	CollisionShape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionShape->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	CollisionShape->bHiddenInGame = true;
}

void AA_Dishes::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

}

void AA_Dishes::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (AttachedMesh)
	{
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), IsValid(AttachedMesh) ? FRotator(0.0f, AttachedMesh->GetComponentRotation().Yaw, 0.0f) : FRotator::ZeroRotator, DeltaTime, 0.5f));

		if (!bIsOnAttaching && AttachedMesh->DoesSocketExist(AttachedSocketName))
		{
			if (FVector::Distance(GetActorLocation(), AttachedMesh->GetSocketLocation(AttachedSocketName)) <= 5.0f)
			{
				bIsOnAttaching = true;
				StaticMesh->AttachToComponent(AttachedMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachedSocketName);
			}
			SetActorLocation(FMath::VInterpTo(GetActorLocation(), AttachedMesh->GetSocketLocation(AttachedSocketName), GetWorld()->GetDeltaSeconds(), 2.0f));
		}
	}

	if (bIsPlacing)
	{
		OnPlacingPauseCheckTime += GetWorld()->GetDeltaSeconds();
		if (OnPlacingPauseCheckTime > 0.5f && StaticMesh->GetPhysicsLinearVelocity().Length() < 2.0f && StaticMesh->GetPhysicsAngularVelocityInDegrees().Length() < 2.0f)
		{
			StaticMesh->SetSimulatePhysics(false);
			bIsPlacing = false;
			OnPlacingPauseCheckTime = 0.0f;
		}
	}
}

void AA_Dishes::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			CollisionShape->GetOverlappingActors(OverlappingActors, AA_Cookable::StaticClass());

			// Check if ingredient doesn't belong any dish
			for (AActor* Ingredient : OverlappingActors.Array())
			{
				if (AA_Cookable* CookableIngredient = Cast<AA_Cookable>(Ingredient))
				{
					if (CookableIngredient->AttachedDish == nullptr)
					{
						Ingredients.AddUnique(CookableIngredient);
						int32& CountRef = IngredientCountMap.FindOrAdd(CookableIngredient->Name);
						++CountRef;
						CookableIngredient->AttachedDish = this;
						CookableIngredient->bIsAttaching = true;

						GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("Added ingredient: %s, Count: %d"), *CookableIngredient->Name.ToString(), CountRef));
					}

				}
			}

			// Check if ingredient is out of dish
			for (int32 i = Ingredients.Num() - 1; i >= 0; --i)
			{
				if (!OverlappingActors.Contains(Ingredients[i]))
				{
					int32* CountPtr = IngredientCountMap.Find(Ingredients[i]->Name);
					if (CountPtr)
					{
						--(*CountPtr);
						if (*CountPtr <= 0)
						{
							IngredientCountMap.Remove(Ingredients[i]->Name);
						}
					}
					Ingredients[i]->AttachedDish = nullptr;

					GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("Removed ingredient: %s, Count: %d"), *Ingredients[i]->Name.ToString(), *CountPtr));

					Ingredients.RemoveAt(i);
				}
			}
		}, 0.5f, true);
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
	AttachedSocketName = SocketName;
	if (!AttachedMesh)
	{
		return;
	}

	StaticMesh->SetSimulatePhysics(false);
	StaticMesh->AttachToComponent(AttachedMesh, FAttachmentTransformRules::KeepWorldTransform, SocketName);
}

void AA_Dishes::DetachDishFromHand()
{
	bIsOnAttaching = false;
	AttachedMesh = nullptr;
	StaticMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	StaticMesh->SetSimulatePhysics(true);
}

void AA_Dishes::RotateDish(float AngleDelta)
{
	AddActorLocalRotation(FRotator(AngleDelta * 10.0f, 0.0f, 0.0f));
}

void AA_Dishes::TossDish()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Tossing dish"));
}