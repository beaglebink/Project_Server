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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UAudioComponent* NodeAudio;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NodeProperty", meta = (AllowPrivateAccess = "true"))
	USoundBase* NodeSoundCorrectWork;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NodeProperty", meta = (AllowPrivateAccess = "true"))
	USoundBase* NodeSoundUncorrectWork;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NodeProperty", meta = (AllowPrivateAccess = "true"))
	int32 NodeNumber;

	UPROPERTY()
	int32 CurrentNodeNumber;

	UPROPERTY()
	UMaterialInstanceDynamic* DMI_BorderMaterial;

	uint8 bIsActivated : 1{false};

	UFUNCTION(BlueprintCallable, Category = "Material", meta = (AllowPrivateAccess = "true"))
	void UpdateBorderMaterial();

	UFUNCTION(BlueprintCallable, Category = "Sound", meta = (AllowPrivateAccess = "true"))
	void NodeSound();

public:
	UFUNCTION(BlueprintCallable, Category = "NodeParameters")
	void SetNodeActivation(bool IsActive);

	UFUNCTION(BlueprintCallable, Category = "NodeParameters")
	bool GetNodeActivation() const;

	UFUNCTION(BlueprintCallable, Category = "NodeParameters")
	void SetNodeNumber(FText NewNumber);

	UFUNCTION(BlueprintCallable, Category = "NodeParameters")
	int32 GetNodeNumber() const;

private:
	void OnActivationChanged();

	void OnNumberChanged();
};
