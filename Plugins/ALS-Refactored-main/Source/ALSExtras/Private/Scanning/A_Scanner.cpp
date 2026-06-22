#include "Scanning/A_Scanner.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"
#include "Components/WidgetComponent.h"
#include "Scanning/I_ScannableObject.h"
#include "Scanning/W_ScannerSummary.h"
#include "PythonContainers/A_InteractableActor.h"

AA_Scanner::AA_Scanner()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	ScannerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScannerMesh"));
	ScannerLaser = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScannerLaser"));
	ScannerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ScannerSphere"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	ScanningTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("ScanningTimeline"));
	ScanningSummaryWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ScanningSummaryWidget"));

	ScannerMesh->SetupAttachment(RootComponent);
	ScannerLaser->SetupAttachment(ScannerMesh, FName("Laser_Socket"));
	ScannerSphere->SetupAttachment(RootComponent);
	AudioComponent->SetupAttachment(RootComponent);
	ScanningSummaryWidgetComponent->SetupAttachment(RootComponent);

	ScannerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ScannerLaser->SetRelativeRotation(FRotator(10.0f, 0.0f, 0.0f));
	ScannerLaser->SetVisibility(false);
	ScannerLaser->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ScannerSphere->SetSphereRadius(10.0f);
	ScannerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ScannerSphere->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel9);
	ScannerSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	ScannerSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel4, ECollisionResponse::ECR_Overlap);

	AudioComponent->bAutoActivate = false;

	ScanningSummaryWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	ScanningSummaryWidgetComponent->SetDrawSize(FVector2D(300.f, 100.f));
}

void AA_Scanner::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

}

void AA_Scanner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_Scanner::BeginPlay()
{
	Super::BeginPlay();

	if (ScanningFloatCurve)
	{
		ScanningProgressFunction.BindUFunction(this, FName("ScanningTimelineProgress"));
		ScanningTimeline->AddInterpFloat(ScanningFloatCurve, ScanningProgressFunction);

		ScanningFinishedFunction.BindUFunction(this, FName("ScanningTimelineFinished"));
		ScanningTimeline->SetTimelineFinishedFunc(ScanningFinishedFunction);

		ScanningTimeline->SetLooping(false);
	}

	ScannerSphere->OnComponentBeginOverlap.AddDynamic(this, &AA_Scanner::OnScannerBeginOverlap);
	ScannerSphere->OnComponentEndOverlap.AddDynamic(this, &AA_Scanner::OnScannerEndOverlap);

	ScanningSummaryWidget = Cast<UW_ScannerSummary>(ScanningSummaryWidgetComponent->GetUserWidgetObject());
}

void AA_Scanner::ScanningTimelineProgress(float Value)
{
	ScannerLaser->SetVisibility(true);
	ScannerLaser->SetRelativeRotation(FMath::Lerp(FRotator(10.0f, 0.0f, 0.0f), FRotator(-20.0f, 0.0f, 0.0f), Value));
}

void AA_Scanner::ScanningTimelineFinished()
{
	AudioComponent->SetSound(ScanningFailSound);

	LastScannedItem = CurrentlyScannedActor;

	if (CurrentlyScannedActor->GetClass()->ImplementsInterface(UI_ScannableObject::StaticClass()))
	{
		FScannableActorData ScannableData = II_ScannableObject::Execute_GetScannableObjectInfo(CurrentlyScannedActor);
		if (!ScannableData.bHasBeenScanned && CheckIfInAcceptedList(CurrentlyScannedActor) && CheckIfByOrderList(CurrentlyScannedActor))
		{
			AudioComponent->SetSound(ScanningSuccessSound);
			ScannedActors.Add(CurrentlyScannedActor);
			ScannableData.bHasBeenScanned = true;
			II_ScannableObject::Execute_SetScannableObjectInfo(CurrentlyScannedActor, ScannableData);
			ShowScanningSummary();
		}
	}

	ScannerLaser->SetVisibility(false);
	AudioComponent->Play();
}

void AA_Scanner::OnScannerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || CurrentlyScannedActor != nullptr)
	{
		return;
	}

	if (GetWorldTimerManager().IsTimerActive(ScanCooldownTimerHandle))
	{
		return;
	}
	GetWorldTimerManager().SetTimer(ScanCooldownTimerHandle, ScanCooldown, false);

	CurrentlyScannedActor = OtherActor;

	if (ScanningTimeline)
	{
		ScanningTimeline->PlayFromStart();
		if (AudioComponent && ScanningSound)
		{
			AudioComponent->SetSound(ScanningSound);
			AudioComponent->Play();
		}
	}
}

void AA_Scanner::OnScannerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor || CurrentlyScannedActor != OtherActor || CurrentlyScannedActor == nullptr)
	{
		return;
	}

	if (ScanningTimeline)
	{
		if (ScanningTimeline->IsPlaying())
		{
			ScanningTimeline->Stop();
			ScannerLaser->SetVisibility(false);

			if (AudioComponent && ScanningFailSound)
			{
				AudioComponent->SetSound(ScanningFailSound);
				AudioComponent->Play();
			}
		}
	}

	CurrentlyScannedActor = nullptr;
}

void AA_Scanner::ShowScanningSummary()
{
	if (ScanningSummaryWidget)
	{
		ScanningSummaryWidget->RefreshSummary(ScannedActors);
	}
}

bool AA_Scanner::CheckIfInAcceptedList(AActor* ActorToScan)
{
	if (ActorToScan->GetClass()->ImplementsInterface(UI_ScannableObject::StaticClass()))
	{
		FScannableActorData ScannableData = II_ScannableObject::Execute_GetScannableObjectInfo(ActorToScan);
		return AcceptedItemTypes.Contains(ScannableData.ItemTypeTag);
	}
	return false;
}

bool AA_Scanner::CheckIfByOrderList(AActor* ActorToScan)
{
	//add logic by later request
	return true;
}
