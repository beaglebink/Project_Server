#include "Portal/A_Portal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AlsCharacterExample.h"
#include "AlsCameraComponent.h"
#include <Kismet/KismetMathLibrary.h>

AA_Portal::AA_Portal()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	P1SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("P1SceneComponent"));
	P2SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("P2SceneComponent"));
	P1MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("P1MeshComponent"));
	P2MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("P2MeshComponent"));
	P1CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("P1CaptureComponent"));
	P2CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("P2CaptureComponent"));
	P1TriggerComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("P1TriggerComponent"));
	P2TriggerComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("P2TriggerComponent"));
	P1AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("P1AudioComponent"));
	P2AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("P2AudioComponent"));

	P1SceneComponent->SetupAttachment(RootComponent);
	P2SceneComponent->SetupAttachment(RootComponent);
	P1MeshComponent->SetupAttachment(P1SceneComponent);
	P2MeshComponent->SetupAttachment(P2SceneComponent);
	P1CaptureComponent->SetupAttachment(P2SceneComponent);
	P2CaptureComponent->SetupAttachment(P1SceneComponent);
	P1TriggerComponent->SetupAttachment(P1SceneComponent);
	P2TriggerComponent->SetupAttachment(P2SceneComponent);
	P1AudioComponent->SetupAttachment(P1SceneComponent);
	P2AudioComponent->SetupAttachment(P2SceneComponent);

	P2SceneComponent->SetRelativeLocation(FVector(200.0f, 0.0f, 0.0f));
	P2SceneComponent->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	P1CaptureComponent->SetRelativeLocation(FVector(20.0f, 0.0f, 0.0f));
	P2CaptureComponent->SetRelativeLocation(FVector(20.0f, 0.0f, 0.0f));

	P1TriggerComponent->SetBoxExtent(FVector(2.0f, 90.0f, 120.0f));
	P2TriggerComponent->SetBoxExtent(FVector(2.0f, 90.0f, 120.0f));
}

void AA_Portal::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	P1DynamicMaterial = P1MeshComponent->CreateDynamicMaterialInstance(0);
	P2DynamicMaterial = P2MeshComponent->CreateDynamicMaterialInstance(0);
	if (P1DynamicMaterial)
	{
		P1MeshComponent->SetMaterial(0, P1DynamicMaterial);
	}
	if (P2DynamicMaterial)
	{
		P2MeshComponent->SetMaterial(0, P2DynamicMaterial);
	}
}

void AA_Portal::BeginPlay()
{
	Super::BeginPlay();

	if (!P1RenderTarget)
	{
		P1RenderTarget = NewObject<UTextureRenderTarget2D>(this);
		P1RenderTarget->InitAutoFormat(1024, 1024);
		P1RenderTarget->UpdateResourceImmediate(true);
	}

	if (!P2RenderTarget)
	{
		P2RenderTarget = NewObject<UTextureRenderTarget2D>(this);
		P2RenderTarget->InitAutoFormat(1024, 1024);
		P2RenderTarget->UpdateResourceImmediate(true);
	}

	P1CaptureComponent->TextureTarget = P1RenderTarget;
	P2CaptureComponent->TextureTarget = P2RenderTarget;

	if (P1DynamicMaterial)
	{
		P1DynamicMaterial->SetTextureParameterValue(TEXT("RenderTarget"), P1RenderTarget);
	}
	if (P2DynamicMaterial)
	{
		P2DynamicMaterial->SetTextureParameterValue(TEXT("RenderTarget"), P2RenderTarget);
	}

	P1TriggerComponent->OnComponentBeginOverlap.AddDynamic(this, &AA_Portal::P1OnBeginOverlap);
	P2TriggerComponent->OnComponentBeginOverlap.AddDynamic(this, &AA_Portal::P2OnBeginOverlap);
}

void AA_Portal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CameraFollowsCharacterView();
}

void AA_Portal::CameraFollowsCharacterView()
{
	if (AAlsCharacterExample* Character = Cast<AAlsCharacterExample>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
	{
		if (FVector::Distance(Character->GetActorLocation(), P1SceneComponent->GetComponentLocation()) > 5000.0f && FVector::Distance(Character->GetActorLocation(), P2SceneComponent->GetComponentLocation()) > 5000.0f)
		{
			return;
		}

		FMinimalViewInfo ViewInfo;
		Character->Camera->GetViewInfo(ViewInfo);
		FRotator CameraRotation = ViewInfo.Rotation;
		FRotator P1Rotation = (-P1SceneComponent->GetForwardVector()).Rotation();
		FRotator P2Rotation = (-P2SceneComponent->GetForwardVector()).Rotation();
		FRotator P1DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(CameraRotation, P1Rotation);
		FRotator P2DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(CameraRotation, P2Rotation);

		P1DeltaRot.Pitch = FMath::Clamp(P1DeltaRot.Pitch, -60.0f, 60.0f);
		P1DeltaRot.Yaw = FMath::Clamp(P1DeltaRot.Yaw, -60.0f, 60.0f);
		P2DeltaRot.Pitch = FMath::Clamp(P2DeltaRot.Pitch, -60.0f, 60.0f);
		P2DeltaRot.Yaw = FMath::Clamp(P2DeltaRot.Yaw, -60.0f, 60.0f);

		P1CaptureComponent->SetRelativeRotation(P1DeltaRot);
		P2CaptureComponent->SetRelativeRotation(P2DeltaRot);

		float ChToP1Dist = FVector::Distance(Character->GetActorLocation(), P1SceneComponent->GetComponentLocation());
		float ChToP2Dist = FVector::Distance(Character->GetActorLocation(), P2SceneComponent->GetComponentLocation());

		P1CaptureComponent->FOVAngle = FMath::Clamp(90.0f - (ChToP1Dist / 100.0f), 30.0f, 90.0f);
		P2CaptureComponent->FOVAngle = FMath::Clamp(90.0f - (ChToP2Dist / 100.0f), 30.0f, 90.0f);
	}
}

void AA_Portal::CheckPlayerDirectionToPortal(UPrimitiveComponent* TriggeredComponent)
{
}

void AA_Portal::P1OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	CheckPlayerDirectionToPortal(OverlappedComponent);
}

void AA_Portal::P2OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	CheckPlayerDirectionToPortal(OverlappedComponent);
}
