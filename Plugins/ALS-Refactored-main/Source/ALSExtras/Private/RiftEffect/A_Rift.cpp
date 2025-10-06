#include "RiftEffect/A_Rift.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"

AA_Rift::AA_Rift()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RiftNiagaraComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RiftNiagaraComp"));
	SewAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("SewAudioComp"));
	SewTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SewTimeline"));

	RiftNiagaraComp->SetupAttachment(RootComponent);
	SewAudioComp->SetupAttachment(RootComponent);

	SewAudioComp->bAutoActivate = false;
}

void AA_Rift::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	for (size_t i = 0; i < FromLeftBoxes.Num(); ++i)
	{
		if (IsValid(FromLeftBoxes[i]) && IsValid(FromRightBoxes[i]))
		{
			FromLeftBoxes[i]->DestroyComponent();
			FromRightBoxes[i]->DestroyComponent();
		}
	}

	FromLeftBoxes.Empty();
	FromRightBoxes.Empty();

	if (!RiftNiagaraComp)
	{
		return;
	}

	FBoxSphereBounds NiagaraBounds = RiftNiagaraComp->Bounds;
	FVector Extent = NiagaraBounds.BoxExtent * 2.0f;

	int32 LoopNum = Extent.Z / BoxSize.Z - 1;
	SpaceBetweenBoxes = Extent.Y / 2.0f + BoxSize.Y / 2;
	uint8 bIsLeft = true;

	for (int32 i = 0; i < LoopNum; ++i)
	{
		FString LeftName = FString::Printf(TEXT("LeftBox_%d"), i);
		UBoxComponent* BoxCompLeft = NewObject<UBoxComponent>(this, *LeftName);
		BoxCompLeft->SetupAttachment(RootComponent);
		BoxCompLeft->RegisterComponent();
		BoxCompLeft->SetBoxExtent(BoxSize * 0.5f);
		BoxCompLeft->SetRelativeLocation(FVector(0.0f, SpaceBetweenBoxes * (static_cast<int32>(bIsLeft) * 2 - 1), BoxSize.Z * i));
		BoxCompLeft->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BoxCompLeft->SetCollisionObjectType(ECC_WorldDynamic);
		BoxCompLeft->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		BoxCompLeft->bHiddenInGame = false;

		FString RightName = FString::Printf(TEXT("RightBox_%d"), i);
		UBoxComponent* BoxCompRight = NewObject<UBoxComponent>(this, *RightName);
		BoxCompRight->SetupAttachment(RootComponent);
		BoxCompRight->RegisterComponent();
		BoxCompRight->SetBoxExtent(BoxSize * 0.5f);
		BoxCompRight->SetRelativeLocation(FVector(0.0f, SpaceBetweenBoxes * (static_cast<int32>(!bIsLeft) * 2 - 1), BoxSize.Z * i));
		BoxCompRight->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BoxCompRight->SetCollisionObjectType(ECC_WorldDynamic);
		BoxCompRight->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		BoxCompRight->bHiddenInGame = false;

		FromLeftBoxes.Add(BoxCompLeft);
		FromRightBoxes.Add(BoxCompRight);

		bIsLeft = !bIsLeft;
	}
}

void AA_Rift::BeginPlay()
{
	Super::BeginPlay();

	if (SewFloatCurve)
	{
		SewProgressFunction.BindUFunction(this, FName("SewTimelineProgress"));
		SewTimeline->AddInterpFloat(SewFloatCurve, SewProgressFunction);

		SewFinishedFunction.BindUFunction(this, FName("SewTimelineFinished"));
		SewTimeline->SetTimelineFinishedFunc(SewFinishedFunction);

		SewTimeline->SetLooping(false);
	}
}

void AA_Rift::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_Rift::HandleWeaponShot_Implementation(FHitResult& Hit)
{
	FString CurrentSide = "";
	int32 CurrentIndex = -1;

	if (ParceComponentName(Hit.GetComponent()->GetFName(), CurrentSide, CurrentIndex))
	{
		if (PrevIndex == -1)
		{
			PrevSide = CurrentSide;
			PrevLocation = Hit.Location;

			if (CurrentIndex == 0)
			{
				Direction = 1;
				PrevIndex = 0;
				SpawnSewHole(Hit.GetComponent(), PrevLocation);
			}
			else if (CurrentIndex == FromLeftBoxes.Num() - 1)
			{
				Direction = -1;
				PrevIndex = FromLeftBoxes.Num() - 1;
				SpawnSewHole(Hit.GetComponent(), PrevLocation);
			}
		}
		else if (CurrentIndex == PrevIndex + Direction && CurrentSide == PrevSide)
		{

			PrevIndex = CurrentIndex;
			SpawnSewHole(Hit.GetComponent(), Hit.Location);
			SpawnSeam(PrevLocation, Hit.Location);
			PrevLocation = Hit.Location;
		}

		if (Direction == 1 && CurrentIndex == FromLeftBoxes.Num() - 1)
		{
			TightenTheSeam(Hit.Location);
		}
		else if (Direction == -1 && CurrentIndex == 0)
		{
			TightenTheSeam(Hit.Location);
		}
	}
}

void AA_Rift::HandleTextFromWeapon_Implementation(const FText& TextCommand)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Rift Text Command: %s"), *TextCommand.ToString()));
}

bool AA_Rift::ParceComponentName(const FName& Name, FString& OutSide, int32& OutIndex)
{
	FString NameStr = Name.ToString();
	if (NameStr.StartsWith("LeftBox_"))
	{
		OutSide = "Left";
		FString IndexStr = NameStr.RightChop(8);
		OutIndex = FCString::Atoi(*IndexStr);
		return true;
	}
	else if (NameStr.StartsWith("RightBox_"))
	{
		OutSide = "Right";
		FString IndexStr = NameStr.RightChop(9);
		OutIndex = FCString::Atoi(*IndexStr);
		return true;
	}
	return false;
}

void AA_Rift::SpawnSewHole(UPrimitiveComponent* Component, FVector HoleLocation)
{
	static int32 i = 0;
	FString HoleName = FString::Printf(TEXT("Hole_%d"), i++);

	UStaticMeshComponent* HoleMeshComp = NewObject<UStaticMeshComponent>(this, *HoleName);
	HoleMeshComp->SetupAttachment(Component);
	HoleMeshComp->RegisterComponent();
	HoleMeshComp->SetWorldLocation(HoleLocation);
	if (HoleStaticMesh)
	{
		HoleMeshComp->SetStaticMesh(HoleStaticMesh);
	}
}

void AA_Rift::SpawnSeam(FVector StartLocation, FVector EndLocation)
{
}

void AA_Rift::TightenTheSeam(FVector Location)
{
	SewAudioComp->Play();
	if (Direction == 1)
	{

	}
	else
	{

	}

	SewTimeline->PlayFromStart();
}

void AA_Rift::SewTimelineProgress(float Value)
{
	for (size_t i = 0; i < FromLeftBoxes.Num(); ++i)
	{
		FVector DefaultLocation = FromLeftBoxes[i]->GetRelativeLocation();
		FVector TargetLocation = FromLeftBoxes[i]->GetRelativeLocation();
		TargetLocation.Y = BoxSize.Y / 2 * (DefaultLocation.Y > 0 ? 1 : -1);
		DefaultLocation.Y = SpaceBetweenBoxes * (DefaultLocation.Y > 0 ? 1 : -1);

		FromLeftBoxes[i]->SetRelativeLocation(FVector(0.0f, FMath::Lerp(DefaultLocation.Y, TargetLocation.Y, Value), DefaultLocation.Z));
		FromRightBoxes[i]->SetRelativeLocation(FVector(0.0f, FMath::Lerp(-DefaultLocation.Y, -TargetLocation.Y, Value), DefaultLocation.Z));
	}
	RiftNiagaraComp->SetNiagaraVariableFloat("User.Ribbon Width", FMath::Lerp(20.0f, 0.0f, Value));
}

void AA_Rift::SewTimelineFinished()
{
	SewAudioComp->Stop();
	//Destroy();
}
