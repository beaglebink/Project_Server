#include "OutcomeConditionAsset.h"

// Macro: include field when value != Default OR comparison is NotEquals (i.e. "!= Default" is meaningful)
// Skip only when: value == Default AND comparison == Equals (would match all Default events - useless)
// (Включить поле если значение != Default ИЛИ сравнение == NotEquals)
// (Пропустить только если: значение == Default И сравнение == Equals)
#define FILTER_SHOULD_INCLUDE(Value, DefaultValue, Comparison) \
	((Value) != (DefaultValue) || (Comparison) == EConditionComparison::NotEquals)

// Build AND chain from a FilterRow - skips fields where value == Default AND comparison == Equals
// (Строит цепочку AND из FilterRow - пропускает поля где значение == Default И сравнение == Equals)
static TSharedPtr<IOutcomeCondition> BuildFromFilterRow(const FOutcomeFilterRow& Row,
	FString& OutDescription)
{
	TSharedPtr<FAndCondition> AndChain = FOutcomeQueryBuilder::And();
	TArray<FString> Parts;

	if (FILTER_SHOULD_INCLUDE(Row.OutcomeType, EOutcomeType::Default, Row.OutcomeTypeComparison))
	{
		const bool bNegate = (Row.OutcomeTypeComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::Type(Row.OutcomeType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	if (FILTER_SHOULD_INCLUDE(Row.MissionType, EOutcomeMission::Default, Row.MissionComparison))
	{
		const bool bNegate = (Row.MissionComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::Mission(Row.MissionType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	if (FILTER_SHOULD_INCLUDE(Row.ActorType, EOutcomeActor::Default, Row.ActorComparison))
	{
		const bool bNegate = (Row.ActorComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::Actor(Row.ActorType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	if (FILTER_SHOULD_INCLUDE(Row.ObjectType, EOutcomeInventory::Default, Row.ObjectComparison))
	{
		const bool bNegate = (Row.ObjectComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::Object(Row.ObjectType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	if (FILTER_SHOULD_INCLUDE(Row.TerminalType, EOutcomeTerminal::Default, Row.TerminalComparison))
	{
		const bool bNegate = (Row.TerminalComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::Terminal(Row.TerminalType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	if (FILTER_SHOULD_INCLUDE(Row.InteriorType, EOutcomeInterior::Default, Row.InteriorComparison))
	{
		const bool bNegate = (Row.InteriorComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::Interior(Row.InteriorType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	if (FILTER_SHOULD_INCLUDE(Row.SpawnGroupType, EOutcomeSpawnGroup::Default, Row.SpawnGroupComparison))
	{
		const bool bNegate = (Row.SpawnGroupComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::SpawnGroup(Row.SpawnGroupType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	if (FILTER_SHOULD_INCLUDE(Row.WorldStateType, EWorldState::Default, Row.WorldStateComparison))
	{
		const bool bNegate = (Row.WorldStateComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::WorldState(Row.WorldStateType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	if (FILTER_SHOULD_INCLUDE(Row.ChoreType, EOutcomeChore::Default, Row.ChoreComparison))
	{
		const bool bNegate = (Row.ChoreComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::Chore(Row.ChoreType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	OutDescription = Parts.IsEmpty()
		? TEXT("Composite: all fields Default - matches everything")
		: FString::Join(Parts, TEXT(" AND "));

	return AndChain;
}

// Helper: build AND(OutcomeType == ExpectedType [, SubCategory op Value])
// SubCategory included when: value != Default OR comparison == NotEquals
// (Строит AND(OutcomeType == ExpectedType [, SubCategory op Value]))
// (SubCategory включается если: значение != Default ИЛИ сравнение == NotEquals)
template<typename TSubEnum>
static TSharedPtr<IOutcomeCondition> BuildCategoryCondition(
	EOutcomeType ExpectedType,
	TSubEnum SubValue,
	TSubEnum SubDefault,
	EConditionComparison SubComparison,
	TSharedPtr<IOutcomeCondition> SubCondition)
{
	TSharedPtr<FAndCondition> Chain = FOutcomeQueryBuilder::And();

	// Always check OutcomeType first
	// (Всегда сначала проверяем OutcomeType)
	Chain->Add(FOutcomeQueryBuilder::Type(ExpectedType));

	// Include subcategory when value != Default OR comparison is NotEquals
	// (Включаем подкатегорию если значение != Default ИЛИ сравнение NotEquals)
	if (FILTER_SHOULD_INCLUDE(SubValue, SubDefault, SubComparison))
	{
		Chain->Add(SubCondition);
	}

	return Chain;
}

void UOutcomeConditionAsset::CompileCondition()
{
	CompiledCondition.Reset();
	ConditionDescription = TEXT("Not compiled");

	switch (OperatorType)
	{
		case EConditionOperator::Composite:
		{
			CompiledCondition = BuildFromFilterRow(FilterRow, ConditionDescription);
			break;
		}

		// Simple operators: automatically AND(OutcomeType == X [, Subcategory op Y])
		// (Простые операторы: автоматически AND(OutcomeType == X [, Subcategory op Y]))

		case EConditionOperator::Mission:
		{
			const bool bNegate = (MissionComparison == EConditionComparison::NotEquals);
			CompiledCondition    = BuildCategoryCondition(
				EOutcomeType::Mission,
				MissionType, EOutcomeMission::Default, MissionComparison,
				FOutcomeQueryBuilder::Mission(MissionType, bNegate));
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::Actor:
		{
			const bool bNegate = (ActorComparison == EConditionComparison::NotEquals);
			CompiledCondition    = BuildCategoryCondition(
				EOutcomeType::Actor,
				ActorType, EOutcomeActor::Default, ActorComparison,
				FOutcomeQueryBuilder::Actor(ActorType, bNegate));
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::Object:
		{
			const bool bNegate = (ObjectComparison == EConditionComparison::NotEquals);
			CompiledCondition    = BuildCategoryCondition(
				EOutcomeType::Inventory,
				ObjectType, EOutcomeInventory::Default, ObjectComparison,
				FOutcomeQueryBuilder::Object(ObjectType, bNegate));
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::Terminal:
		{
			const bool bNegate = (TerminalComparison == EConditionComparison::NotEquals);
			CompiledCondition    = BuildCategoryCondition(
				EOutcomeType::Terminal,
				TerminalType, EOutcomeTerminal::Default, TerminalComparison,
				FOutcomeQueryBuilder::Terminal(TerminalType, bNegate));
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::Interior:
		{
			const bool bNegate = (InteriorComparison == EConditionComparison::NotEquals);
			CompiledCondition    = BuildCategoryCondition(
				EOutcomeType::Interior,
				InteriorType, EOutcomeInterior::Default, InteriorComparison,
				FOutcomeQueryBuilder::Interior(InteriorType, bNegate));
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::SpawnGroup:
		{
			const bool bNegate = (SpawnGroupComparison == EConditionComparison::NotEquals);
			CompiledCondition    = BuildCategoryCondition(
				EOutcomeType::SpawnGroup,
				SpawnGroupType, EOutcomeSpawnGroup::Default, SpawnGroupComparison,
				FOutcomeQueryBuilder::SpawnGroup(SpawnGroupType, bNegate));
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::WorldState:
		{
			const bool bNegate = (WorldStateComparison == EConditionComparison::NotEquals);
			CompiledCondition    = BuildCategoryCondition(
				EOutcomeType::WorldState,
				WorldStateType, EWorldState::Default, WorldStateComparison,
				FOutcomeQueryBuilder::WorldState(WorldStateType, bNegate));
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::And:
		{
			if (!FirstCondition || !SecondCondition)
			{
				ConditionDescription = TEXT("AND: First and Second conditions required!");
				UE_LOG(LogTemp, Warning, TEXT("OutcomeConditionAsset [%s]: AND requires both conditions"), *GetName());
				return;
			}
			FirstCondition->CompileCondition();
			SecondCondition->CompileCondition();
			if (!FirstCondition->GetCondition().IsValid() || !SecondCondition->GetCondition().IsValid())
			{
				ConditionDescription = TEXT("AND: child compile failed!");
				return;
			}
			CompiledCondition = FOutcomeQueryBuilder::And()
				->Add(FirstCondition->GetCondition())
				->Add(SecondCondition->GetCondition());
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::Or:
		{
			if (!FirstCondition || !SecondCondition)
			{
				ConditionDescription = TEXT("OR: First and Second conditions required!");
				UE_LOG(LogTemp, Warning, TEXT("OutcomeConditionAsset [%s]: OR requires both conditions"), *GetName());
				return;
			}
			FirstCondition->CompileCondition();
			SecondCondition->CompileCondition();
			if (!FirstCondition->GetCondition().IsValid() || !SecondCondition->GetCondition().IsValid())
			{
				ConditionDescription = TEXT("OR: child compile failed!");
				return;
			}
			CompiledCondition = FOutcomeQueryBuilder::Or()
				->Add(FirstCondition->GetCondition())
				->Add(SecondCondition->GetCondition());
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::Not:
		{
			if (!FirstCondition)
			{
				ConditionDescription = TEXT("NOT: First condition required!");
				UE_LOG(LogTemp, Warning, TEXT("OutcomeConditionAsset [%s]: NOT requires FirstCondition"), *GetName());
				return;
			}
			FirstCondition->CompileCondition();
			if (!FirstCondition->GetCondition().IsValid())
			{
				ConditionDescription = TEXT("NOT: child compile failed!");
				return;
			}
			CompiledCondition    = FOutcomeQueryBuilder::Not(FirstCondition->GetCondition());
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		default:
			ConditionDescription = TEXT("Unknown operator!");
			UE_LOG(LogTemp, Warning, TEXT("OutcomeConditionAsset [%s]: Unknown operator"), *GetName());
			return;
	}

	UE_LOG(LogTemp, Log, TEXT("OutcomeConditionAsset [%s]: Compiled -> %s"),
		*GetName(), *ConditionDescription);
}

void UOutcomeConditionAsset::ResetCondition()
{
	OperatorType         = EConditionOperator::Composite;
	FilterRow            = FOutcomeFilterRow();
	MissionType          = EOutcomeMission::Default;
	MissionComparison    = EConditionComparison::Equals;
	ActorType            = EOutcomeActor::Default;
	ActorComparison      = EConditionComparison::Equals;
	ObjectType           = EOutcomeInventory::Default;
	ObjectComparison     = EConditionComparison::Equals;
	TerminalType         = EOutcomeTerminal::Default;
	TerminalComparison   = EConditionComparison::Equals;
	InteriorType         = EOutcomeInterior::Default;
	InteriorComparison   = EConditionComparison::Equals;
	SpawnGroupType       = EOutcomeSpawnGroup::Default;
	SpawnGroupComparison = EConditionComparison::Equals;
	WorldStateType       = EWorldState::Default;
	WorldStateComparison = EConditionComparison::Equals;
	FirstCondition       = nullptr;
	SecondCondition      = nullptr;
	ConditionDescription = TEXT("Reset");
	CompiledCondition.Reset();

	UE_LOG(LogTemp, Log, TEXT("OutcomeConditionAsset [%s]: Reset complete"), *GetName());
}