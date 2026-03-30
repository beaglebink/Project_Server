#include "OutcomeConditionAsset.h"
#include "OutcomeQuery.h"

// Helper to get enum display name as string (Вспомогательная функция для получения имени значения enum)
static FString GetOutcomeTypeName(EOutcomeType Value)
{
	switch (Value)
	{
		case EOutcomeType::Default:               return TEXT("Default");
		case EOutcomeType::GhostCleared:          return TEXT("GhostCleared");
		case EOutcomeType::DialogueStarted:       return TEXT("DialogueStarted");
		case EOutcomeType::TerminalTaskCompleted: return TEXT("TerminalTaskCompleted");
		case EOutcomeType::MissionCompleted:      return TEXT("MissionCompleted");
		case EOutcomeType::ItemAcquired:          return TEXT("ItemAcquired");
		default:                                  return TEXT("Unknown");
	}
}

static FString GetComparisonSign(EConditionComparison Comparison)
{
	return Comparison == EConditionComparison::NotEquals ? TEXT("!=") : TEXT("==");
}

void UOutcomeConditionAsset::CompileCondition()
{
	// Reset previous condition (Сбросить предыдущее условие)
	CompiledCondition.Reset();
	CurrentState = EConditionState::Empty;
	ConditionDescription = TEXT("Empty");

	// Build condition based on OperatorType (Построить условие на основе OperatorType)
	switch (OperatorType)
	{
		case EConditionOperator::Type:
		{
			CompiledCondition = FOutcomeQueryBuilder::Type(EventType);
			ConditionDescription = FString::Printf(TEXT("Type == %s"),
				*GetOutcomeTypeName(EventType));
			CurrentState = EConditionState::Simple;
			break;
		}
		case EConditionOperator::Mission:
		{
			TSharedPtr<IOutcomeCondition> Condition = FOutcomeQueryBuilder::Mission(MissionType);
			if (MissionComparison == EConditionComparison::NotEquals)
			{
				Condition = FOutcomeQueryBuilder::Not(Condition);
			}
			ConditionDescription = FString::Printf(TEXT("Mission %s Default"),
				*GetComparisonSign(MissionComparison));
			CompiledCondition = Condition;
			CurrentState = EConditionState::Simple;
			break;
		}
		case EConditionOperator::Actor:
		{
			TSharedPtr<IOutcomeCondition> Condition = FOutcomeQueryBuilder::Actor(ActorType);
			if (ActorComparison == EConditionComparison::NotEquals)
			{
				Condition = FOutcomeQueryBuilder::Not(Condition);
			}
			ConditionDescription = FString::Printf(TEXT("Actor %s Default"),
				*GetComparisonSign(ActorComparison));
			CompiledCondition = Condition;
			CurrentState = EConditionState::Simple;
			break;
		}
		case EConditionOperator::Object:
		{
			TSharedPtr<IOutcomeCondition> Condition = FOutcomeQueryBuilder::Object(ObjectType);
			if (ObjectComparison == EConditionComparison::NotEquals)
			{
				Condition = FOutcomeQueryBuilder::Not(Condition);
			}
			ConditionDescription = FString::Printf(TEXT("Object %s Default"),
				*GetComparisonSign(ObjectComparison));
			CompiledCondition = Condition;
			CurrentState = EConditionState::Simple;
			break;
		}
		case EConditionOperator::Interior:
		{
			TSharedPtr<IOutcomeCondition> Condition = FOutcomeQueryBuilder::Interior(InteriorType);
			if (InteriorComparison == EConditionComparison::NotEquals)
			{
				Condition = FOutcomeQueryBuilder::Not(Condition);
			}
			ConditionDescription = FString::Printf(TEXT("Interior %s Default"),
				*GetComparisonSign(InteriorComparison));
			CompiledCondition = Condition;
			CurrentState = EConditionState::Simple;
			break;
		}
		case EConditionOperator::SpawnGroup:
		{
			TSharedPtr<IOutcomeCondition> Condition = FOutcomeQueryBuilder::SpawnGroup(SpawnGroupType);
			if (SpawnGroupComparison == EConditionComparison::NotEquals)
			{
				Condition = FOutcomeQueryBuilder::Not(Condition);
			}
			ConditionDescription = FString::Printf(TEXT("SpawnGroup %s Default"),
				*GetComparisonSign(SpawnGroupComparison));
			CompiledCondition = Condition;
			CurrentState = EConditionState::Simple;
			break;
		}
		case EConditionOperator::And:
		{
			if (FirstCondition && SecondCondition)
			{
				// Compile child conditions first (Сначала компилируем дочерние условия)
				FirstCondition->CompileCondition();
				SecondCondition->CompileCondition();

				auto AndQuery = FOutcomeQueryBuilder::And();
				AndQuery->Add(FirstCondition->GetCondition());
				AndQuery->Add(SecondCondition->GetCondition());
				CompiledCondition = AndQuery;
				ConditionDescription = FString::Printf(TEXT("(%s) AND (%s)"),
					*FirstCondition->GetConditionDescription(),
					*SecondCondition->GetConditionDescription());
				CurrentState = EConditionState::Complex;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("OutcomeConditionAsset: AND requires both FirstCondition and SecondCondition"));
				ConditionDescription = TEXT("AND: missing conditions!");
			}
			break;
		}
		case EConditionOperator::Or:
		{
			if (FirstCondition && SecondCondition)
			{
				// Compile child conditions first (Сначала компилируем дочерние условия)
				FirstCondition->CompileCondition();
				SecondCondition->CompileCondition();

				auto OrQuery = FOutcomeQueryBuilder::Or();
				OrQuery->Add(FirstCondition->GetCondition());
				OrQuery->Add(SecondCondition->GetCondition());
				CompiledCondition = OrQuery;
				ConditionDescription = FString::Printf(TEXT("(%s) OR (%s)"),
					*FirstCondition->GetConditionDescription(),
					*SecondCondition->GetConditionDescription());
				CurrentState = EConditionState::Complex;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("OutcomeConditionAsset: OR requires both FirstCondition and SecondCondition"));
				ConditionDescription = TEXT("OR: missing conditions!");
			}
			break;
		}
		case EConditionOperator::Not:
		{
			if (FirstCondition)
			{
				// Compile child condition first (Сначала компилируем дочернее условие)
				FirstCondition->CompileCondition();

				CompiledCondition = FOutcomeQueryBuilder::Not(FirstCondition->GetCondition());
				ConditionDescription = FString::Printf(TEXT("NOT (%s)"),
					*FirstCondition->GetConditionDescription());
				CurrentState = EConditionState::Complex;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("OutcomeConditionAsset: NOT requires FirstCondition"));
				ConditionDescription = TEXT("NOT: missing condition!");
			}
			break;
		}
		default:
		{
			UE_LOG(LogTemp, Warning, TEXT("OutcomeConditionAsset: Unknown operator type"));
			break;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("OutcomeConditionAsset: Compiled - %s"), *ConditionDescription);
}

FString UOutcomeConditionAsset::GetConditionDescription() const
{
	return ConditionDescription;
}

void UOutcomeConditionAsset::ResetCondition()
{
	CompiledCondition.Reset();
	ConditionDescription = TEXT("Empty");
	CurrentState = EConditionState::Empty;
	OperatorType = EConditionOperator::Type;
	EventType = EOutcomeType::Default;
	MissionType = EOutcomeMission::Default;
	MissionComparison = EConditionComparison::Equals;
	ActorType = EOutcomeActor::Default;
	ActorComparison = EConditionComparison::Equals;
	ObjectType = EOutcomeObject::Default;
	ObjectComparison = EConditionComparison::Equals;
	InteriorType = EOutcomeInterior::Default;
	InteriorComparison = EConditionComparison::Equals;
	SpawnGroupType = EOutcomeSpawnGroup::Default;
	SpawnGroupComparison = EConditionComparison::Equals;
	FirstCondition = nullptr;
	SecondCondition = nullptr;

	UE_LOG(LogTemp, Log, TEXT("OutcomeConditionAsset: Reset complete"));
}