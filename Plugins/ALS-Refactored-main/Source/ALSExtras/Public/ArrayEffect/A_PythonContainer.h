#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_PythonContainer.generated.h"

class AA_ArrayNode;
class UBoxComponent;
class UTextRenderComponent;

UCLASS()
class ALSEXTRAS_API AA_PythonContainer : public AActor
{
	GENERATED_BODY()

public:
	AA_PythonContainer();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	USceneComponent* SceneComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UBoxComponent* CollisionComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UChildActorComponent* EndNodeComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	TSubclassOf<AA_ArrayNode> NodeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	TSubclassOf<AA_PythonContainer> ContainerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTextRenderComponent* TextComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	FText ContainerName;

	FVector DefaultLocation;

	FRotator DefaultRotation;

	AA_ArrayNode* EndNode;

	float NodeLength;

	float NodeWidth;

	float NodeHigh;

	TArray<AA_ArrayNode*> NodeArray;

	uint8 bIsOverlapping : 1{false};

	uint8 bIsSwapping : 1{false};

	uint8 bIsOnConcatenation : 1{false};

	uint8 bIsDetaching : 1{false};

	uint8 bIsAttaching : 1{false};

	int32 SizeOfConcatenatingContainer = -1;

	virtual void GetTextCommand(FText Command);

	void AppendNode(FName VariableName = NAME_None);

	void DeleteNode(int32 Index);

	virtual void ContainerPop(int32 Index);

	void ContainerClear();

	void ContainerExtend();

	void ContainerConcatenate(AA_PythonContainer* ContainerToConcatenate);

	void ContainerRename(FText NewName);

	virtual void ContainerCopy(FText Name, int32 OutLeftIndex, int32 OutRightIndex);

	UFUNCTION(BlueprintCallable)
	static bool IsValidPythonIdentifier(const FString& Str);

	bool ParseCommandToAppend(FText Command, FText& PrevName, FName& VariableName);

	bool ParseCommandToPop(FText Command, FText& PrevName, FName& VariableName, int32& Index);

	bool ParseCommandToClear(FText Command, FText& PrevName);

	bool ParseCommandToExtend(FText Command, FText& PrevName, TArray<int32>& OutArray);

	bool ParseCommandToConcatenate(FText Command, int32& OutSize1, int32& OutSize2);

	bool ParseCommandToReset(FText Command, FText& PrevName);

	bool ParseCommandToRename(FText Command, FText& PrevName, FText& NewName, int32& ArrayNum);

	bool ParseCommandToCopy(FText Command, FText& PrevName, FText& CopyName, int32& OutLeftIndex, int32& OutRightIndex);

	void AttachToCharacterCamera();

	void DetachFromCharacterCamera();

	void AttachToContainer();

	void MoveNodesConsideringOrder();

	void RefreshNameLocationAndRotation();

	void SetContainerName(FText Name);

	AActor* GetActorWithTag(const FName& Tag);

	TArray<int32> ExtendContainer;

	int32 ExtendContainerIndex = 0;
};