#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_DiscreteSystemNode.generated.h"

UCLASS()
class ALSEXTRAS_API AA_DiscreteSystemNode : public AActor
{
	GENERATED_BODY()

public:
	AA_DiscreteSystemNode();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* SM_ZoneBorder;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* SKM_Node;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NodeProperty", meta = (AllowPrivateAccess = "true"))
	int32 NodeNumber;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Material", meta = (AllowPrivateAccess = "true"))
	UMaterialInstanceDynamic* DMI_BorderMaterial;

	UFUNCTION(BlueprintCallable, Category = "Material", meta = (AllowPrivateAccess = "true"))
	void UpdateBorderMaterial();

public:
	UPROPERTY(BlueprintReadWrite, Category = "NodeProperty")
	uint8 bIsActivated : 1{false};

	UPROPERTY(BlueprintReadWrite, Category = "NodeProperty")
	int32 CurrentNodeNumber;
};
