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

	if (!StaticMesh->IsSimulatingPhysics())
	{
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), IsValid(AttachedMesh) ? FRotator(0.0f, AttachedMesh->GetComponentRotation().Yaw, 0.0f) : FRotator::ZeroRotator, DeltaTime, 0.5f));

		if (!bIsOnAttaching && AttachedMesh && AttachedMesh->DoesSocketExist(AttachedSocketName))
		{
			if (FVector::Distance(GetActorLocation(), AttachedMesh->GetSocketLocation(AttachedSocketName)) <= 5.0f)
			{
				bIsOnAttaching = true;
				StaticMesh->AttachToComponent(AttachedMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachedSocketName);
			}
			SetActorLocation(FMath::VInterpTo(GetActorLocation(), AttachedMesh->GetSocketLocation(AttachedSocketName), GetWorld()->GetDeltaSeconds(), 0.5f));
		}
	}
}

void AA_Dishes::BeginPlay()
{
	Super::BeginPlay();

	CollisionShape->OnComponentBeginOverlap.AddDynamic(this, &AA_Dishes::OnSphereOverlapBegin);
	CollisionShape->OnComponentEndOverlap.AddDynamic(this, &AA_Dishes::OnSphereOverlapEnd);
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

void AA_Dishes::OnSphereOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

	if (OtherActor && OtherComp)
	{
		if (AA_Cookable* CookableIngredient = Cast<AA_Cookable>(OtherActor))
		{
			CookableIngredients.AddUnique(CookableIngredient);
			int32& CountRef = IngredientCountMap.FindOrAdd(CookableIngredient->Name);
			++CountRef;

			if (CookableIngredient->AttachedDish == nullptr)
			{
				CookableIngredient->AttachedDish = this;
				CookableIngredient->bIsAttaching = true;
			}

			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("Added ingredient: %s, Count: %d"), *CookableIngredient->Name.ToString(), CountRef));
		}
	}
}

void AA_Dishes::OnSphereOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor && OtherComp)
	{
		if (AA_Cookable* CookableIngredient = Cast<AA_Cookable>(OtherActor))
		{
			CookableIngredients.Remove(CookableIngredient);
			int32* CountPtr = IngredientCountMap.Find(CookableIngredient->Name);
			if (CountPtr)
			{
				--(*CountPtr);
				if (*CountPtr <= 0)
				{
					IngredientCountMap.Remove(CookableIngredient->Name);
				}
			}

			if (CookableIngredient->AttachedDish == this)
			{
				CookableIngredient->AttachedDish = nullptr;
			}

			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("Removed ingredient: %s, Count: %d"), *CookableIngredient->Name.ToString(), *CountPtr));
		}
	}
}
