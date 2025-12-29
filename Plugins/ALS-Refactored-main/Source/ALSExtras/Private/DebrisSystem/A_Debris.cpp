#include "DebrisSystem/A_Debris.h"
#include "Engine/TextureRenderTarget2D.h" 
#include "Kismet/KismetRenderingLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

AA_Debris::AA_Debris()
{
	PrimaryActorTick.bCanEverTick = true;

	DebrisMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebrisMeshComponent"));
	DebrisFlowFXComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DebrisFlowFXComponent"));

	RootComponent = DebrisMeshComponent;
	DebrisFlowFXComponent->SetupAttachment(DebrisMeshComponent);

	DebrisFlowFXComponent->SetAutoActivate(false);
}

void AA_Debris::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AA_Debris::BeginPlay()
{
	Super::BeginPlay();

	if (bShouldMeshSimulatePhysics)
	{
		DebrisMeshComponent->SetSimulatePhysics(true);
	}

	if (!IsValid(MeshMaterialInstanceDynamic))
	{
		MeshMaterialInstanceDynamic = DebrisMeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	}

	MeshRenderTarget = NewObject<UTextureRenderTarget2D>(this);
	if (!MeshRenderTarget)return;

	MeshRenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
	MeshRenderTarget->InitAutoFormat(1024, 1024);
	MeshRenderTarget->ClearColor = FLinearColor::Black;
	MeshRenderTarget->UpdateResourceImmediate(true);

	UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), MeshRenderTarget, MeshMaterialInstanceDynamic);
	if (ParticlesMaterial)
	{
		UMaterialInstanceDynamic* FXDynMat = nullptr;
		DebrisFlowFXComponent->SetVariableObject(TEXT("User.SourceMesh"), DebrisMeshComponent->GetStaticMesh());
		FXDynMat = UMaterialInstanceDynamic::Create(ParticlesMaterial, this);
		FXDynMat->SetTextureParameterValue(TEXT("MeshRenderTarget"), MeshRenderTarget);
		DebrisFlowFXComponent->SetVariableMaterial(TEXT("User.RenderMaterial"), FXDynMat);
	}
}

void AA_Debris::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_Debris::DestroyOrRespawnDebris()
{
	if (ReplenishTime > 0.0f)
	{
		if (!GetWorldTimerManager().IsTimerActive(RespawnTimerHandle))
		{
			DebrisMeshComponent->SetVisibility(false);
			DebrisMeshComponent->SetSimulatePhysics(false);
			DebrisMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Alpha = 0.0f;

			GetWorldTimerManager().SetTimer(RespawnTimerHandle, [this]()
				{
					DebrisMeshComponent->SetVisibility(true);
					DebrisMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					if (bShouldMeshSimulatePhysics)
					{
						DebrisMeshComponent->SetSimulatePhysics(true);
					}
					DebrisMeshComponent->SetWorldScale3D(FVector(1.0f));
				}, ReplenishTime, false);
		}
	}
	else
	{
		Destroy();
	}
}

float AA_Debris::DebrisMassInfluence() const
{
	FVector DebrisMeshBoxExtent = DebrisMeshComponent->Bounds.BoxExtent;
	return FMath::Clamp(DebrisMeshBoxExtent.X * DebrisMeshBoxExtent.Y * DebrisMeshBoxExtent.Z / 1000.0f, 1.0f, 1000.0f) * SuctionDifficultyMultiplier;
}

