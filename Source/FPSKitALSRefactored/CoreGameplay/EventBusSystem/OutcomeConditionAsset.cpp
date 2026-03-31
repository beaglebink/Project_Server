#include "OutcomeConditionAsset.h"

// Build AND chain from a FilterRow - skips Default values
// (Строит цепочку AND из FilterRow - пропускает значения Default)
static TSharedPtr<IOutcomeCondition> BuildFromFilterRow(const FOutcomeFilterRow& Row,
	FString& OutDescription)
{
	TSharedPtr<FAndCondition> AndChain = FOutcomeQueryBuilder::And();
	TArray<FString> Parts;

	if (Row.MissionType != EOutcomeMission::Default)
	{
		const bool bNegate = (Row.MissionComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::Mission(Row.MissionType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	if (Row.ActorType != EOutcomeActor::Default)
	{
		const bool bNegate = (Row.ActorComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::Actor(Row.ActorType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	if (Row.ObjectType != EOutcomeObject::Default)
	{
		const bool bNegate = (Row.ObjectComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::Object(Row.ObjectType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	if (Row.TerminalType != EOutcomeTerminal::Default)
	{
		const bool bNegate = (Row.TerminalComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::Terminal(Row.TerminalType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	if (Row.InteriorType != EOutcomeInterior::Default)
	{
		const bool bNegate = (Row.InteriorComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::Interior(Row.InteriorType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	if (Row.SpawnGroupType != EOutcomeSpawnGroup::Default)
	{
		const bool bNegate = (Row.SpawnGroupComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::SpawnGroup(Row.SpawnGroupType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	if (Row.WorldStateType != EWorldState::Default)
	{
		const bool bNegate = (Row.WorldStateComparison == EConditionComparison::NotEquals);
		TSharedPtr<IOutcomeCondition> C = FOutcomeQueryBuilder::WorldState(Row.WorldStateType, bNegate);
		AndChain->Add(C);
		Parts.Add(C->Describe());
	}

	OutDescription = Parts.IsEmpty()
		? TEXT("Composite: all fields Default - matches everything")
		: FString::Join(Parts, TEXT(" AND "));

	return AndChain;
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

		case EConditionOperator::Mission:
		{
			const bool bNegate = (MissionComparison == EConditionComparison::NotEquals);
			CompiledCondition = FOutcomeQueryBuilder::Mission(MissionType, bNegate);
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::Actor:
		{
			const bool bNegate = (ActorComparison == EConditionComparison::NotEquals);
			CompiledCondition = FOutcomeQueryBuilder::Actor(ActorType, bNegate);
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::Object:
		{
			const bool bNegate = (ObjectComparison == EConditionComparison::NotEquals);
			CompiledCondition = FOutcomeQueryBuilder::Object(ObjectType, bNegate);
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::Terminal:
		{
			const bool bNegate = (TerminalComparison == EConditionComparison::NotEquals);
			CompiledCondition = FOutcomeQueryBuilder::Terminal(TerminalType, bNegate);
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::Interior:
		{
			const bool bNegate = (InteriorComparison == EConditionComparison::NotEquals);
			CompiledCondition = FOutcomeQueryBuilder::Interior(InteriorType, bNegate);
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::SpawnGroup:
		{
			const bool bNegate = (SpawnGroupComparison == EConditionComparison::NotEquals);
			CompiledCondition = FOutcomeQueryBuilder::SpawnGroup(SpawnGroupType, bNegate);
			ConditionDescription = CompiledCondition->Describe();
			break;
		}

		case EConditionOperator::WorldState:
		{
			const bool bNegate = (WorldStateComparison == EConditionComparison::NotEquals);
			CompiledCondition = FOutcomeQueryBuilder::WorldState(WorldStateType, bNegate);
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
			CompiledCondition = FOutcomeQueryBuilder::Not(FirstCondition->GetCondition());
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
	OperatorType = EConditionOperator::Composite;
	FilterRow = FOutcomeFilterRow();
	MissionType = EOutcomeMission::Default;
	MissionComparison = EConditionComparison::Equals;
	ActorType = EOutcomeActor::Default;
	ActorComparison = EConditionComparison::Equals;
	ObjectType = EOutcomeObject::Default;
	ObjectComparison = EConditionComparison::Equals;
	TerminalType = EOutcomeTerminal::Default;
	TerminalComparison = EConditionComparison::Equals;
	InteriorType = EOutcomeInterior::Default;
	InteriorComparison = EConditionComparison::Equals;
	SpawnGroupType = EOutcomeSpawnGroup::Default;
	SpawnGroupComparison = EConditionComparison::Equals;
	WorldStateType = EWorldState::Default;
	WorldStateComparison = EConditionComparison::Equals;
	FirstCondition = nullptr;
	SecondCondition = nullptr;
	ConditionDescription = TEXT("Reset");
	CompiledCondition.Reset();

	UE_LOG(LogTemp, Log, TEXT("OutcomeConditionAsset [%s]: Reset complete"), *GetName());
}