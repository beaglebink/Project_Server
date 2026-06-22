#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Utility/AlsGameplayTags.h"
#include "A_AreaDropZone.generated.h"

class AA_InteractableActor;

UCLASS()
class FPSKITALSREFACTORED_API AA_AreaDropZone : public AActor
{
	GENERATED_BODY()

public:
	AA_AreaDropZone();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|Items")
	uint8 bShouldCheckItemsByTag : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|Items", meta = (EditCondition = "bShouldCheckItemsByTag", EditConditionHides))
	TArray<FGameplayTag> ItemsTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|Items", meta = (EditCondition = "!bShouldCheckItemsByTag", EditConditionHides))
	TArray<TSubclassOf<AA_InteractableActor>> ItemsClasses;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DropZone|AreaMesh")
	UStaticMeshComponent* DropZoneMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DropZone|AreaMeshMaterial")
	UMaterialInterface* DropZoneMeshMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|AreaMeshSize")
	FVector DropZoneMeshSize{ 100.0f, 100.0f, 100.0f };

	UPROPERTY()
	UMaterialInstanceDynamic* DMI_MeshMaterial;

	UPROPERTY()
	TArray<FName> ItemsNames;

	void SetMeshMaterialAndState(int32 NewState, bool IsPlacing);

private:
	uint8 IsPlacingOnScene : 1{true};
};
