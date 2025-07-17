#include "Inventory/UI/Visual3D/A_3DDescription.h"
#include "Components/SphereComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"

AA_3DDescription::AA_3DDescription()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	SpotLightComponent = CreateDefaultSubobject<USpotLightComponent>(TEXT("SpotLightComponent"));
	SceneCaptureComponent2D = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComponent2D"));

	RootComponent = SceneComponent;
	SphereComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->SetupAttachment(SphereComponent);
	SpotLightComponent->SetupAttachment(RootComponent);
	SceneCaptureComponent2D->SetupAttachment(RootComponent);
}

void AA_3DDescription::BeginPlay()
{
	Super::BeginPlay();
	
	SceneCaptureComponent2D->ShowOnlyActorComponents(this);
}

void AA_3DDescription::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SphereComponent->AddWorldRotation(FRotator(0.0f, 3.0f, 0.0f));
}

