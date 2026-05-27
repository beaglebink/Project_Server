#include "Dirt/AC_Dirt.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"


UAC_Dirt::UAC_Dirt()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAC_Dirt::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAC_Dirt::BeginPlay()
{
	Super::BeginPlay();

	StaticMeshComponent = Cast<UStaticMeshComponent>(GetOwner()->GetComponentByClass(UStaticMeshComponent::StaticClass()));
	if (StaticMeshComponent)
	{
		if (UMaterialInterface* OverlayMaterial = StaticMeshComponent->GetOverlayMaterial())
		{
			StaticMeshComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel8, ECR_Block);

			RenderTarget_A = NewObject<UTextureRenderTarget2D>(this);
			RenderTarget_A->InitCustomFormat(256, 256, PF_R16F, false);
			RenderTarget_A->ClearColor = FLinearColor::Black;
			RenderTarget_A->UpdateResourceImmediate();

			RenderTarget_B = NewObject<UTextureRenderTarget2D>(this);
			RenderTarget_B->InitCustomFormat(256, 256, PF_R16F, false);
			RenderTarget_B->ClearColor = FLinearColor::Black;
			RenderTarget_B->UpdateResourceImmediate();

			OverlayDynamicMaterial = UMaterialInstanceDynamic::Create(OverlayMaterial, this);
			StaticMeshComponent->SetOverlayMaterial(OverlayDynamicMaterial);

			if (DirtTexture)
			{
				OverlayDynamicMaterial->SetTextureParameterValue("DirtTexture", DirtTexture);
			}
			OverlayDynamicMaterial->SetTextureParameterValue("RenderTarget", RenderTarget_B);

			BrushDynamicMaterial = UMaterialInstanceDynamic::Create(BrushMaterial, this);
			BrushDynamicMaterial->SetTextureParameterValue("PrevRenderTarget", RenderTarget_A);
		}
	}
}

void UAC_Dirt::HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit)
{
	if (!OverlayDynamicMaterial || !BrushDynamicMaterial)
	{
		return;
	}

	FVector2D HitUV;
	bool bHasUV = UGameplayStatics::FindCollisionUV(Hit, 1, HitUV);

	if (bHasUV)
	{
		BrushDynamicMaterial->SetTextureParameterValue("PrevRenderTarget", RenderTarget_A);
		BrushDynamicMaterial->SetVectorParameterValue("HitUV", FLinearColor(HitUV.X, HitUV.Y, 0, 0));
		BrushDynamicMaterial->SetScalarParameterValue("WashPower", Hit.Distance);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, RenderTarget_B, BrushDynamicMaterial);
		OverlayDynamicMaterial->SetTextureParameterValue("RenderTarget", RenderTarget_B);

		UTextureRenderTarget2D* Temp = RenderTarget_A;
		RenderTarget_A = RenderTarget_B;
		RenderTarget_B = Temp;
	}
}