#include "Portal/A_Portal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AlsCharacterExample.h"
#include "AlsCameraComponent.h"
#include <Kismet/KismetMathLibrary.h>
#include "Interfaces/I_PortalInteraction.h"
#include "FPSKitALSRefactored\CoreGameplay\InteractionSystem\InteractiveItemComponent.h"

AA_Portal::AA_Portal()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	PortalMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMeshComponent"));
	PortalCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PortalCaptureComponent"));
	PortalTriggerComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("PortalTriggerComponent"));
	PortalAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("PortalAudioComponent"));
	PortalButtonMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalButtonMeshComponent"));
	PortalInteractiveComponent = CreateDefaultSubobject<UInteractiveItemComponent>(TEXT("PortalInteractiveComponent"));
	ActivationTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("ActivateTimeline"));

	PortalMeshComponent->SetupAttachment(RootComponent);
	PortalCaptureComponent->SetupAttachment(PortalMeshComponent);
	PortalTriggerComponent->SetupAttachment(PortalMeshComponent);
	PortalButtonMeshComponent->SetupAttachment(PortalMeshComponent);
	PortalAudioComponent->SetupAttachment(PortalMeshComponent);

	PortalCaptureComponent->SetRelativeLocation(FVector(20.0f, 0.0f, 0.0f));

	PortalTriggerComponent->SetBoxExtent(FVector(2.0f, 90.0f, 120.0f));
	PortalTriggerComponent->SetGenerateOverlapEvents(true);
	PortalTriggerComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PortalTriggerComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	PortalTriggerComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	PortalTriggerComponent->SetCollisionResponseToChannel(ECC_Visibility, ECollisionResponse::ECR_Block);
	PortalTriggerComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);
	PortalTriggerComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECollisionResponse::ECR_Block);

	PortalAudioComponent->bAutoActivate = false;

	PortalButtonMeshComponent->SetRelativeLocation(FVector(18.0f, -110.0f, 0.0f));
	PortalButtonMeshComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
}

void AA_Portal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!PortalRenderTarget)
	{
		PortalRenderTarget = NewObject<UTextureRenderTarget2D>(this);
		PortalRenderTarget->InitAutoFormat(1024, 1024);
		PortalCaptureComponent->TextureTarget = PortalRenderTarget;
		PortalRenderTarget->UpdateResourceImmediate(true);
	}

	PortalDynamicMaterial = PortalMeshComponent->CreateDynamicMaterialInstance(0);
	if (PortalDynamicMaterial)
	{
		PortalMeshComponent->SetMaterial(0, PortalDynamicMaterial);
		PortalDynamicMaterial->SetScalarParameterValue(TEXT("Activation"), bIsActive);
		if (ExitPortal && ExitPortal->PortalRenderTarget)
		{
			PortalDynamicMaterial->SetTextureParameterValue(TEXT("RenderTarget"), ExitPortal->PortalRenderTarget);
		}
	}

	PortalButtonDynamicMaterial = PortalButtonMeshComponent->CreateDynamicMaterialInstance(0);
	if (PortalButtonDynamicMaterial)
	{
		PortalButtonMeshComponent->SetMaterial(0, PortalButtonDynamicMaterial);
		PortalButtonDynamicMaterial->SetScalarParameterValue(TEXT("Activation"), bIsActive);
	}
}

void AA_Portal::BeginPlay()
{
	Super::BeginPlay();

	PortalTriggerComponent->OnComponentBeginOverlap.AddDynamic(this, &AA_Portal::PortalOnBeginOverlap);

	if (ActivationFloatCurve)
	{
		ActivationProgressFunction.BindUFunction(this, FName("ActivationTimelineProgress"));
		ActivationTimeline->AddInterpFloat(ActivationFloatCurve, ActivationProgressFunction);

		ActivationFinishedFunction.BindUFunction(this, FName("ActivationTimelineFinished"));
		ActivationTimeline->SetTimelineFinishedFunc(ActivationFinishedFunction);

		ActivationTimeline->SetLooping(false);
	}

	PortalInteractiveComponent->OnInteractionPressKeyEvent.AddDynamic(this, &AA_Portal::StartActivateDeactivatePortal);

	PortalInteractiveComponent->InteractiveTooltipText = FText::FromString("Press \"F\" to Activate Portal");
	if (bIsActive)
	{
		PortalAudioComponent->Play();
		PortalInteractiveComponent->InteractiveTooltipText = FText::FromString("Press \"F\" to Deactivate Portal");
	}
}

void AA_Portal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CameraFollowsCharacterView();
}

void AA_Portal::CameraFollowsCharacterView()
{
	if (!bIsActive)
	{
		return;
	}

	AAlsCharacterExample* Character = Cast<AAlsCharacterExample>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (Character && ExitPortal)
	{
		if (FVector::Distance(Character->GetActorLocation(), GetActorLocation()) > 5000.0f)
		{
			return;
		}

		FMinimalViewInfo ViewInfo;
		Character->Camera->GetViewInfo(ViewInfo);
		FRotator CameraRotation = ViewInfo.Rotation;
		FRotator PortalRotation = (-GetActorForwardVector()).Rotation();
		FRotator PortalDeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(CameraRotation, PortalRotation);

		PortalDeltaRot.Pitch = FMath::Clamp(PortalDeltaRot.Pitch, -60.0f, 60.0f);
		PortalDeltaRot.Yaw = FMath::Clamp(PortalDeltaRot.Yaw, -60.0f, 60.0f);

		ExitPortal->PortalCaptureComponent->SetRelativeRotation(PortalDeltaRot);

		float ChToPortalDist = FVector::Distance(Character->GetActorLocation(), GetActorLocation());

		ExitPortal->PortalCaptureComponent->FOVAngle = FMath::Clamp(90.0f - (ChToPortalDist / 100.0f), 30.0f, 90.0f);
	}
}

void AA_Portal::PortalOnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bIsActive || !ExitPortal)
	{
		return;
	}

	if (GetActorForwardVector().Dot(SweepResult.ImpactNormal) > 0.0f)
	{
		return;
	}

	if (!OtherActor || !OtherActor->GetClass()->ImplementsInterface(UI_PortalInteraction::StaticClass()) || OtherActor->ActorHasTag(FName(TEXT("BeingGrabbed"))))
	{
		return;
	}

	PortalTriggerComponent->SetGenerateOverlapEvents(false);
	ExitPortal->PortalTriggerComponent->SetGenerateOverlapEvents(false);
	FTimerHandle TimerHandleOverlap;
	GetWorldTimerManager().SetTimer(TimerHandleOverlap, [this]()
		{
			PortalTriggerComponent->SetGenerateOverlapEvents(true);
			ExitPortal->PortalTriggerComponent->SetGenerateOverlapEvents(true);
		}, CoolDownTime, false);

	II_PortalInteraction::Execute_PortalInteract(OtherActor, SweepResult, GetActorTransform(), ExitPortal->GetActorTransform());
}

void AA_Portal::HandleWeaponShot_Implementation(FHitResult& Hit)
{
	if (Hit.Component != PortalTriggerComponent)
	{
		return;
	}

	FVector ShotDirection = (Hit.ImpactPoint - Hit.TraceStart).GetSafeNormal();
	FVector Start = Hit.ImpactPoint;
	FVector End = Start + ShotDirection * FVector::Distance(Hit.ImpactPoint, Hit.TraceEnd);

	FCollisionQueryParams Params;
	Params.AddIgnoredComponent(PortalTriggerComponent);

	if (GetActorForwardVector().Dot(ShotDirection) < 0.0f && bIsActive && ExitPortal)
	{
		FRotator ShotRotation = ShotDirection.Rotation();
		FVector PortalDeltaLocation = ExitPortal->GetActorLocation() - GetActorLocation();
		FRotator PortalDeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(ExitPortal->GetActorRotation(), GetActorRotation());
		PortalDeltaRotation.Yaw += 180.0f;

		Params.ClearIgnoredComponents();
		Params.AddIgnoredComponent(ExitPortal->PortalTriggerComponent);

		Start += PortalDeltaLocation;
		ShotRotation += PortalDeltaRotation;
		End = Start + ShotRotation.Vector() * FVector::Distance(Hit.ImpactPoint, Hit.TraceEnd);
	}

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		DrawDebugLine(GetWorld(), Start, Hit.ImpactPoint, FColor::Red, false, 2.f, 0, 2.f);
		DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10.f, FColor::Yellow, false, 2.f);
		DrawDebugLine(GetWorld(), Hit.ImpactPoint, End, FColor::Green, false, 2.f, 0, 2.f);
	}
	else
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.f, 0, 2.f);
	}

	return;
}

void AA_Portal::ActivationTimelineProgress(float Value)
{
	PortalButtonMeshComponent->SetRelativeLocation(FVector(18.0f - 4.0f * Value, -110.0f, 0.0f));

	static float LastValue = 0.0f;
	bool bIsIncreasing = Value > LastValue;
	LastValue = Value;

	if (PortalDynamicMaterial && PortalButtonDynamicMaterial)
	{
		if (!bIsActive && bIsIncreasing)
		{
			PortalDynamicMaterial->SetScalarParameterValue(TEXT("Activation"), Value);
			PortalButtonDynamicMaterial->SetScalarParameterValue(TEXT("Activation"), Value);
		}
		else if (bIsActive && bIsIncreasing)
		{
			PortalDynamicMaterial->SetScalarParameterValue(TEXT("Activation"), 1 - Value);
			PortalButtonDynamicMaterial->SetScalarParameterValue(TEXT("Activation"), 1 - Value);
		}
	}
}

void AA_Portal::ActivationTimelineFinished()
{
	bIsActive = !bIsActive;
}

void AA_Portal::StartActivateDeactivatePortal(UInteractivePickerComponent* Picker)
{
	if (ActivationTimeline->IsPlaying())
	{
		return;
	}

	if (bIsActive)
	{
		PortalAudioComponent->Stop();
		PortalInteractiveComponent->InteractiveTooltipText = FText::FromString("Press \"F\" to Activate Portal");
	}
	else
	{
		PortalAudioComponent->Play();
		PortalInteractiveComponent->InteractiveTooltipText = FText::FromString("Press \"F\" to Deactivate Portal");
	}

	ActivationTimeline->PlayFromStart();
}
