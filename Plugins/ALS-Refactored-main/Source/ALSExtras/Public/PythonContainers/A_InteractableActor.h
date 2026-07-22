#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/I_PortalInteraction.h"
#include "Interfaces/I_PowerConnection.h"
#include "Scanning/I_ScannableObject.h"
#include "Interfaces/I_WeaponInteraction.h"
#include "InteractiveActorInterface.h"
#include "Scanning/ScannableActorData.h"
#include "A_InteractableActor.generated.h"

class AA_DropZone;

UCLASS()
class ALSEXTRAS_API AA_InteractableActor : public AActor, public II_PortalInteraction, public II_PowerConnection, public II_ScannableObject, public IInteractiveActorInterface, public II_WeaponInteraction
{
	GENERATED_BODY()

public:
	AA_InteractableActor();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StaticMesh")
	UStaticMeshComponent* StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Name")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScanningData")
	FScannableActorData ScannableData;

	UPROPERTY()
	AA_DropZone* AttachingDropZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Interaction")
	uint8 bIsHeld : 1{false};

	UFUNCTION(BlueprintCallable, Category = "TextParsing")
	bool ParseAssignCommand(FText Command, FName& OutVarName, FName& OutActorName);

	virtual void PortalInteract_Implementation(const FHitResult& Hit, const FTransform& EnterTransform, const FTransform& ExitTransform) override;

	virtual void OnPowerConnected_Implementation(bool IsConnected) override;

	virtual FScannableActorData GetScannableObjectInfo_Implementation() override;

	virtual void SetScannableObjectInfo_Implementation(const FScannableActorData& NewData) override;

	virtual UPrimitiveComponent* GetMeshComponent_Implementation() override;

	virtual void HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit);

	virtual void HandleTextFromWeapon_Implementation(const FText& TextCommand);
};
