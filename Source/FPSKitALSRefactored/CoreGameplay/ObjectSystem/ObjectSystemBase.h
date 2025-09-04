#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObjectSystemBase.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API AObjectSystemBase : public AActor
{
    GENERATED_BODY()

public:
	TArray<FString> ParseCommands(const FText& InputText) const;

	UFUNCTION(BlueprintCallable, Category = "ObjectSystem")
	void ParseText(const FText Text);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ObjectSystem")
	void VariableAssignment(const FString& VariableName, const FString& Value);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "ObjectSystem")
	void ExecuteCommand(const FString& Command, const TArray<FString>& Arguments);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ObjectSystem")
	FString ObjectActorName;
};