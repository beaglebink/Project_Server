#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_ArrayNode.generated.h"

UCLASS()
class ALSEXTRAS_API AA_ArrayNode : public AActor
{
	GENERATED_BODY()

public:
	AA_ArrayNode();

protected:
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* NodeBorder;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* NodeContainer;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	UPROPERTY(BlueprintReadWrite, Category = "ArrayIndex")
	int32 NodeIndex;

	UPROPERTY()
	UMaterialInstanceDynamic* DMI_BorderMaterial;

	UFUNCTION(BlueprintCallable, Category = "Material")
	void SetBorderMaterialAndIndex(int32 NewIndex = -1);
};
