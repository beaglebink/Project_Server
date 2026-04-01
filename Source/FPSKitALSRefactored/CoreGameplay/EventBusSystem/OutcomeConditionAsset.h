#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Outcome.h"
#include "OutcomeQuery.h"
#include "OutcomeConditionAsset.generated.h"

// Comparison operator for simple conditions
// (Оператор сравнения для простых условий)
UENUM(BlueprintType)
enum class EConditionComparison : uint8
{
	Equals    UMETA(DisplayName = "==  Equals"),
	NotEquals UMETA(DisplayName = "!=  Not Equals")
};

// Row of conditions for Composite operator
// All non-Default fields are joined by AND automatically - leave field as Default to skip it
// (Строка условий для Composite - все поля != Default объединяются AND)
// (Оставьте Default чтобы пропустить поле)
USTRUCT(BlueprintType)
struct FOutcomeFilterRow
{
	GENERATED_BODY()

	// ===== TYPE =====
	// Use OutcomeType in Composite to pre-filter by event category before checking subcategory
	// (Используйте OutcomeType в Composite для фильтрации по категории события перед проверкой подкатегории)

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EOutcomeType OutcomeType = EOutcomeType::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConditionComparison OutcomeTypeComparison = EConditionComparison::Equals;

	// ===== MISSION =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EOutcomeMission MissionType = EOutcomeMission::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConditionComparison MissionComparison = EConditionComparison::Equals;

	// ===== ACTOR =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EOutcomeActor ActorType = EOutcomeActor::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConditionComparison ActorComparison = EConditionComparison::Equals;

	// ===== OBJECT =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EOutcomeObject ObjectType = EOutcomeObject::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConditionComparison ObjectComparison = EConditionComparison::Equals;

	// ===== TERMINAL =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EOutcomeTerminal TerminalType = EOutcomeTerminal::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConditionComparison TerminalComparison = EConditionComparison::Equals;

	// ===== INTERIOR =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EOutcomeInterior InteriorType = EOutcomeInterior::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConditionComparison InteriorComparison = EConditionComparison::Equals;

	// ===== SPAWN GROUP =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EOutcomeSpawnGroup SpawnGroupType = EOutcomeSpawnGroup::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConditionComparison SpawnGroupComparison = EConditionComparison::Equals;

	// ===== WORLD STATE =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWorldState WorldStateType = EWorldState::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConditionComparison WorldStateComparison = EConditionComparison::Equals;
};

// Operator type for the condition asset
// Simple operators automatically include OutcomeType check - no need to configure it manually
// (Тип оператора ассета условия)
// (Простые операторы автоматически включают проверку OutcomeType - не нужно настраивать вручную)
UENUM(BlueprintType)
enum class EConditionOperator : uint8
{
	// Composite: configure any combination of fields manually, all non-Default joined by AND
	// (Составной: задай любую комбинацию полей вручную, все != Default объединяются AND)
	Composite   UMETA(DisplayName = "Composite  (AND row)"),

	// Simple operators: automatically check OutcomeType == X AND subcategory (if not Default)
	// (Простые операторы: автоматически проверяют OutcomeType == X И подкатегорию (если не Default))
	Mission     UMETA(DisplayName = "Mission"),
	Actor       UMETA(DisplayName = "Actor"),
	Object      UMETA(DisplayName = "Object"),
	Terminal    UMETA(DisplayName = "Terminal"),
	Interior    UMETA(DisplayName = "Interior"),
	SpawnGroup  UMETA(DisplayName = "Spawn Group"),
	WorldState  UMETA(DisplayName = "World State"),

	// Logical combinators over other assets (Логические комбинаторы над другими ассетами)
	And         UMETA(DisplayName = "AND  (First + Second)"),
	Or          UMETA(DisplayName = "OR   (First + Second)"),
	Not         UMETA(DisplayName = "NOT  (First only)")
};

UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UOutcomeConditionAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// ===== OPERATOR TYPE =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1 - Operator")
	EConditionOperator OperatorType = EConditionOperator::Composite;

	// ===== COMPOSITE =====
	// Set non-Default fields - they will be joined by AND
	// (Задай поля != Default - они будут объединены через AND)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2 - Composite Conditions",
		meta = (EditCondition = "OperatorType == EConditionOperator::Composite", EditConditionHides))
	FOutcomeFilterRow FilterRow;

	// ===== LOGICAL OPERANDS =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2 - Logic Operands",
		meta = (EditCondition = "OperatorType == EConditionOperator::And || OperatorType == EConditionOperator::Or || OperatorType == EConditionOperator::Not",
				EditConditionHides, DisplayName = "First Condition"))
	TObjectPtr<UOutcomeConditionAsset> FirstCondition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2 - Logic Operands",
		meta = (EditCondition = "OperatorType == EConditionOperator::And || OperatorType == EConditionOperator::Or",
				EditConditionHides, DisplayName = "Second Condition"))
	TObjectPtr<UOutcomeConditionAsset> SecondCondition = nullptr;

	// ===== SIMPLE CONDITION: MISSION =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3 - Simple Condition",
		meta = (EditCondition = "OperatorType == EConditionOperator::Mission", EditConditionHides))
	EOutcomeMission MissionType = EOutcomeMission::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3 - Simple Condition",
		meta = (EditCondition = "OperatorType == EConditionOperator::Mission", EditConditionHides))
	EConditionComparison MissionComparison = EConditionComparison::Equals;

	// ===== SIMPLE CONDITION: ACTOR =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3 - Simple Condition",
		meta = (EditCondition = "OperatorType == EConditionOperator::Actor", EditConditionHides))
	EOutcomeActor ActorType = EOutcomeActor::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3 - Simple Condition",
		meta = (EditCondition = "OperatorType == EConditionOperator::Actor", EditConditionHides))
	EConditionComparison ActorComparison = EConditionComparison::Equals;

	// ===== SIMPLE CONDITION: OBJECT =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3 - Simple Condition",
		meta = (EditCondition = "OperatorType == EConditionOperator::Object", EditConditionHides))
	EOutcomeObject ObjectType = EOutcomeObject::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3 - Simple Condition",
		meta = (EditCondition = "OperatorType == EConditionOperator::Object", EditConditionHides))
	EConditionComparison ObjectComparison = EConditionComparison::Equals;

	// ===== SIMPLE CONDITION: TERMINAL =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3 - Simple Condition",
		meta = (EditCondition = "OperatorType == EConditionOperator::Terminal", EditConditionHides))
	EOutcomeTerminal TerminalType = EOutcomeTerminal::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3 - Simple Condition",
		meta = (EditCondition = "OperatorType == EConditionOperator::Terminal", EditConditionHides))
	EConditionComparison TerminalComparison = EConditionComparison::Equals;

	// ===== SIMPLE CONDITION: INTERIOR =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3 - Simple Condition",
		meta = (EditCondition = "OperatorType == EConditionOperator::Interior", EditConditionHides))
	EOutcomeInterior InteriorType = EOutcomeInterior::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3 - Simple Condition",
		meta = (EditCondition = "OperatorType == EConditionOperator::Interior", EditConditionHides))
	EConditionComparison InteriorComparison = EConditionComparison::Equals;

	// ===== SIMPLE CONDITION: SPAWNGROUP =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3 - Simple Condition",
		meta = (EditCondition = "OperatorType == EConditionOperator::SpawnGroup", EditConditionHides))
	EOutcomeSpawnGroup SpawnGroupType = EOutcomeSpawnGroup::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3 - Simple Condition",
		meta = (EditCondition = "OperatorType == EConditionOperator::SpawnGroup", EditConditionHides))
	EConditionComparison SpawnGroupComparison = EConditionComparison::Equals;

	// ===== SIMPLE CONDITION: WORLDSTATE =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3 - Simple Condition",
		meta = (EditCondition = "OperatorType == EConditionOperator::WorldState", EditConditionHides))
	EWorldState WorldStateType = EWorldState::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "3 - Simple Condition",
		meta = (EditCondition = "OperatorType == EConditionOperator::WorldState", EditConditionHides))
	EConditionComparison WorldStateComparison = EConditionComparison::Equals;

	// ===== DEBUG =====

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "4 - Debug")
	FString ConditionDescription = TEXT("Not compiled");

	// ===== EDITOR ACTIONS =====

	UFUNCTION(CallInEditor, Category = "4 - Debug")
	void CompileCondition();

	UFUNCTION(CallInEditor, Category = "4 - Debug")
	void ResetCondition();

	// ===== BLUEPRINT CALLABLE =====

	UFUNCTION(BlueprintCallable, Category = "EventBus|Conditions")
	FString GetConditionDescription() const { return ConditionDescription; }

	// ===== C++ ONLY =====

	TSharedPtr<IOutcomeCondition> GetCondition() const { return CompiledCondition; }

protected:
	TSharedPtr<IOutcomeCondition> CompiledCondition;
};