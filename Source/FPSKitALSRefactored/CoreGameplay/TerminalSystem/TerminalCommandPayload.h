#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "TerminalCommandPayload.generated.h"

UENUM(BlueprintType)
enum class ECommandObjectType : uint8
{
	ModeratelyReactive			UMETA(DisplayName = "Moderately Reactive"),
	Door						UMETA(DisplayName = "Door"),
	Terminal					UMETA(DisplayName = "Terminal")
};

UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UTerminalCommandPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Terminal Command")
	FGuid ObjectItemId;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Terminal Command")
	ECommandObjectType ObjectType;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Terminal Command")
	bool IsEnable;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Terminal Command")
	AActor* OwnerActor;
};