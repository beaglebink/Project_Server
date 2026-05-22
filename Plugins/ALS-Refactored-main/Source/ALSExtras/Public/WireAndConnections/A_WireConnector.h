#pragma once

#include "CoreMinimal.h"
#include "PythonContainers/A_InteractableActor.h"
#include "A_WireConnector.generated.h"

UCLASS()
class ALSEXTRAS_API AA_WireConnector : public AA_InteractableActor
{
	GENERATED_BODY()
	
public:
	AA_WireConnector();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void Destroyed() override;

public:
	UPROPERTY(EditDefaultsOnly, Category = "Wire")
	TSubclassOf<AA_WireConnector> OppositeConnectorClass;

	UPROPERTY(VisibleDefaultsOnly, Category = "Wire")
	AA_WireConnector* OppositeConnector;

	UPROPERTY(EditDefaultsOnly, Category = "Wire")
	TSubclassOf<AA_DropZone> OppositeConnectionClass;

	UPROPERTY(VisibleDefaultsOnly, Category = "Wire")
	AA_DropZone* OppositeConnection;

	UFUNCTION(BlueprintCallable, Category = "Wire")
	void SetPower(bool OnPower);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wire")
	bool GetPower() const { return bIsOnPower; }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wire")
	void OnPowerChanged(bool OnPower);

private:
	UPROPERTY(VisibleDefaultsOnly, Category = "Wire")
	uint8 bIsOnPower : 1{false};
};
