#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_ArrayEffect.generated.h"

class AA_ArrayNode;

UCLASS()
class ALSEXTRAS_API AA_ArrayEffect : public AActor
{
	GENERATED_BODY()
	
public:	
	AA_ArrayEffect();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneComponent;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* AppendNodeComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AA_ArrayNode> NodeClass;

	AA_ArrayNode* AppendNode;

	TArray<AA_ArrayNode*> NodeArray;

	int32 ArrayIndex = 0;

	uint8 bIsAddingNode : 1{false};
	
	uint8 bIsDeletingNode : 1{false};

	FVector TargetLocation;

	UFUNCTION()
	void AddNewNode();
	
	void AddNewNodeVisual();

	UFUNCTION()
	void DeleteNode(int32 Index);

	void DeleteNodeVisual();
};
