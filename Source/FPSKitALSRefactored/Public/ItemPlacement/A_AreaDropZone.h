#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NativeGameplayTags.h"
#include "A_AreaDropZone.generated.h"

class AA_InteractableActor;
class UFloorAssignmentComponent;

UENUM(BlueprintType)
enum class EWrongItemBehavior : uint8
{
	Ignore,
	CountAsMistake,
	BlockCompletion
};

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|ParentActor")
	AActor* ParentActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropZone|Items")
	TMap<FGameplayTag, int32> ItemsTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|WrongItemsBehavior")
	EWrongItemBehavior ItemBehavior;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DropZone|AreaMesh")
	UStaticMeshComponent* DropZoneMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DropZone|AreaMeshMaterial")
	UMaterialInterface* DropZoneMeshMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* DMI_MeshMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UFloorAssignmentComponent* FloorAssignmentComponent;

	void SetMeshMaterialAndState(int32 NewState, bool IsPlacing);

private:
	uint8 IsPlacingOnScene : 1{true};

public:
	UPROPERTY(EditAnywhere, Category = "DropZone|ActiveState", meta = (AllowPrivateAccess = "true"))
	uint8 bIsActive : 1{true};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DropZone|Items")
	TMap<FGameplayTag, int32> CorrectItemsList;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DropZone|Items")
	TMap<FGameplayTag, int32> WrongItemsList;

	void UpdateInAreaItemsList();
};
