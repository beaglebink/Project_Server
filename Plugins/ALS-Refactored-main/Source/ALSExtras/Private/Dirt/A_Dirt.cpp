#include "Dirt/A_Dirt.h"
#include "Components/DecalComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetRenderingLibrary.h"

AA_Dirt::AA_Dirt()
{
	PrimaryActorTick.bCanEverTick = false;

	DecalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComponent"));
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));

	RootComponent = DecalComponent;
	BoxComponent->SetupAttachment(RootComponent);

	DecalComponent->DecalSize = FVector(10.0f, 100.0f, 100.0f);
	DecalComponent->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
}

void AA_Dirt::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	BoxComponent->SetBoxExtent(DecalComponent->DecalSize);
	BoxComponent->SetNotifyRigidBodyCollision(true);

	if (!DecalDynamicMaterial)
	{
		DecalDynamicMaterial = DecalComponent->CreateDynamicMaterialInstance();
		DecalComponent->SetMaterial(0, DecalDynamicMaterial);
	}

	if (DirtTexture)
	{
		DecalDynamicMaterial->SetTextureParameterValue("DirtTexture", DirtTexture);
	}

	if (!BrushDynamicMaterial)
	{
		BrushDynamicMaterial = UMaterialInstanceDynamic::Create(BrushMaterial, this);
	}
}

void AA_Dirt::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_Dirt::BeginPlay()
{
	Super::BeginPlay();

	RenderTarget_A = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget_A->InitCustomFormat(256, 256, PF_R16F, false);
	RenderTarget_A->ClearColor = FLinearColor::Black;
	RenderTarget_A->UpdateResourceImmediate();
	BrushDynamicMaterial->SetTextureParameterValue("PrevRenderTarget", RenderTarget_A);

	RenderTarget_B = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget_B->InitCustomFormat(256, 256, PF_R16F, false);
	RenderTarget_B->ClearColor = FLinearColor::Black;
	RenderTarget_B->UpdateResourceImmediate();

	DecalDynamicMaterial->SetTextureParameterValue("RenderTarget", RenderTarget_B);
}

void AA_Dirt::HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit)
{

	FVector Local = DecalComponent->GetComponentTransform().InverseTransformPosition(Hit.ImpactPoint);
	Local *= 0.5f;
	float U = Local.Z / DecalComponent->DecalSize.Z;
	float V = Local.Y / DecalComponent->DecalSize.Y;

	BrushDynamicMaterial->SetTextureParameterValue("PrevRenderTarget", RenderTarget_A);
	BrushDynamicMaterial->SetVectorParameterValue("HitUV", FLinearColor(U, V, 0, 0));
	BrushDynamicMaterial->SetScalarParameterValue("WashPower", Hit.Distance);
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, RenderTarget_B, BrushDynamicMaterial);
	DecalDynamicMaterial->SetTextureParameterValue("RenderTarget", RenderTarget_B);

	UTextureRenderTarget2D* Temp = RenderTarget_A;
	RenderTarget_A = RenderTarget_B;
	RenderTarget_B = Temp;
}
