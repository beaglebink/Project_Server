#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OutcomeQuery.h"
#include "OutcomeConditionAsset.generated.h"

class IOutcomeCondition;

UENUM(BlueprintType)
enum class EConditionOperator : uint8
{
	Type       UMETA(DisplayName = "Event Type"),
	Mission    UMETA(DisplayName = "Mission"),
	Actor      UMETA(DisplayName = "Actor"),
	Object     UMETA(DisplayName = "Object"),
	Interior   UMETA(DisplayName = "Interior"),
	SpawnGroup UMETA(DisplayName = "Spawn Group"),
	And        UMETA(DisplayName = "AND"),
	Or         UMETA(DisplayName = "OR"),
	Not        UMETA(DisplayName = "NOT")
};

UENUM(BlueprintType)
enum class EConditionComparison : uint8
{
	Equals    UMETA(DisplayName = "Equals"),
	NotEquals UMETA(DisplayName = "Not Equals")
};

UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UOutcomeConditionAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// ===== CONFIGURATION PROPERTIES =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Setup")
	EConditionOperator OperatorType = EConditionOperator::Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|EventType")
	EOutcomeType EventType = EOutcomeType::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Mission")
	EOutcomeMission MissionType = EOutcomeMission::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Mission")
	EConditionComparison MissionComparison = EConditionComparison::Equals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Actor")
	EOutcomeActor ActorType = EOutcomeActor::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Actor")
	EConditionComparison ActorComparison = EConditionComparison::Equals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Object")
	EOutcomeObject ObjectType = EOutcomeObject::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Object")
	EConditionComparison ObjectComparison = EConditionComparison::Equals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Interior")
	EOutcomeInterior InteriorType = EOutcomeInterior::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Interior")
	EConditionComparison InteriorComparison = EConditionComparison::Equals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|SpawnGroup")
	EOutcomeSpawnGroup SpawnGroupType = EOutcomeSpawnGroup::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|SpawnGroup")
	EConditionComparison SpawnGroupComparison = EConditionComparison::Equals;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Logic")
	TObjectPtr<UOutcomeConditionAsset> FirstCondition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition|Logic")
	TObjectPtr<UOutcomeConditionAsset> SecondCondition = nullptr;

	// ===== DEBUG =====

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Debug")
	FString ConditionDescription;

	// ===== EDITOR BUTTONS (CallInEditor makes them appear as buttons in Details) =====

	UFUNCTION(CallInEditor, Category = "Condition|Actions")
	void CompileCondition();

	UFUNCTION(CallInEditor, Category = "Condition|Actions")
	void ResetCondition();

	// ===== BLUEPRINT CALLABLE =====

	UFUNCTION(BlueprintCallable, Category = "EventBus|Conditions")
	FString GetConditionDescription() const;

	// ===== C++ ONLY =====

	TSharedPtr<IOutcomeCondition> GetCondition() const { return CompiledCondition; }

protected:
	TSharedPtr<IOutcomeCondition> CompiledCondition;

	enum class EConditionState : uint8
	{
		Empty,
		Simple,
		Complex
	};
	EConditionState CurrentState = EConditionState::Empty;
};