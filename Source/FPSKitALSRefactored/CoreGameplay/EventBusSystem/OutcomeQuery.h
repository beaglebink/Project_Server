#pragma once

#include "CoreMinimal.h"
#include "Outcome.h"
#include "OutcomeEventBase.h"

// Base condition interface (Базовый интерфейс условия)
class IOutcomeCondition
{
public:
	virtual ~IOutcomeCondition() = default;
	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const = 0;
	virtual FString Describe() const = 0;
};

// Template for simple enum field conditions with Equals/NotEquals
// (Шаблон для простых условий по полям enum с поддержкой Equals/NotEquals)
template<typename TEnumType>
class TFieldCondition : public IOutcomeCondition
{
public:
	using FieldSelector = TEnumType(*)(const FOutcomeEventBase&);

	TFieldCondition(FieldSelector InSelector, TEnumType InValue, bool bInNegate, const TCHAR* InFieldName)
		: Selector(InSelector), Value(InValue), bNegate(bInNegate), FieldName(InFieldName)
	{
	}

	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
	{
		const bool bMatch = Selector(Outcome) == Value;
		return bNegate ? !bMatch : bMatch;
	}

	virtual FString Describe() const override
	{
		return FString::Printf(TEXT("%s %s %s"),
			FieldName,
			bNegate ? TEXT("!=") : TEXT("=="),
			*UEnum::GetValueAsString(Value));
	}

private:
	FieldSelector Selector;
	TEnumType Value;
	bool bNegate;
	const TCHAR* FieldName;
};

// ===== LOGICAL CONDITIONS =====

// AND condition (Условие И)
class FAndCondition : public IOutcomeCondition, public TSharedFromThis<FAndCondition>
{
public:
	TSharedRef<FAndCondition> Add(TSharedPtr<IOutcomeCondition> Condition)
	{
		if (Condition.IsValid()) { Conditions.Add(Condition); }
		return SharedThis(this);
	}

	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
	{
		for (const auto& C : Conditions)
		{
			if (!C->Evaluate(Outcome)) { return false; }
		}
		return true;
	}

	virtual FString Describe() const override
	{
		TArray<FString> Parts;
		for (const auto& C : Conditions) { Parts.Add(C->Describe()); }
		return FString::Printf(TEXT("(%s)"), *FString::Join(Parts, TEXT(" AND ")));
	}

private:
	TArray<TSharedPtr<IOutcomeCondition>> Conditions;
};

// OR condition (Условие ИЛИ)
class FOrCondition : public IOutcomeCondition, public TSharedFromThis<FOrCondition>
{
public:
	TSharedRef<FOrCondition> Add(TSharedPtr<IOutcomeCondition> Condition)
	{
		if (Condition.IsValid()) { Conditions.Add(Condition); }
		return SharedThis(this);
	}

	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
	{
		for (const auto& C : Conditions)
		{
			if (C->Evaluate(Outcome)) { return true; }
		}
		return false;
	}

	virtual FString Describe() const override
	{
		TArray<FString> Parts;
		for (const auto& C : Conditions) { Parts.Add(C->Describe()); }
		return FString::Printf(TEXT("(%s)"), *FString::Join(Parts, TEXT(" OR ")));
	}

private:
	TArray<TSharedPtr<IOutcomeCondition>> Conditions;
};

// NOT condition (Условие НЕ)
class FNotCondition : public IOutcomeCondition
{
public:
	FNotCondition(TSharedPtr<IOutcomeCondition> InCondition) : Condition(InCondition) {}

	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
	{
		return Condition.IsValid() ? !Condition->Evaluate(Outcome) : true;
	}

	virtual FString Describe() const override
	{
		return FString::Printf(TEXT("NOT(%s)"),
			Condition.IsValid() ? *Condition->Describe() : TEXT("?"));
	}

private:
	TSharedPtr<IOutcomeCondition> Condition;
};

// ===== BUILDER =====

struct FOutcomeQueryBuilder
{
	static TSharedPtr<IOutcomeCondition> Mission(EOutcomeMission Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EOutcomeMission>>(
			[](const FOutcomeEventBase& O) { return O.OutcomeMission; },
			Value, bNegate, TEXT("Mission"));
	}

	static TSharedPtr<IOutcomeCondition> Actor(EOutcomeActor Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EOutcomeActor>>(
			[](const FOutcomeEventBase& O) { return O.OutcomeActor; },
			Value, bNegate, TEXT("Actor"));
	}

	static TSharedPtr<IOutcomeCondition> Object(EOutcomeObject Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EOutcomeObject>>(
			[](const FOutcomeEventBase& O) { return O.OutcomeObject; },
			Value, bNegate, TEXT("Object"));
	}

	// Terminal events (Терминальные события)
	static TSharedPtr<IOutcomeCondition> Terminal(EOutcomeTerminal Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EOutcomeTerminal>>(
			[](const FOutcomeEventBase& O) { return O.OutcomeTerminal; },
			Value, bNegate, TEXT("Terminal"));
	}

	static TSharedPtr<IOutcomeCondition> Interior(EOutcomeInterior Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EOutcomeInterior>>(
			[](const FOutcomeEventBase& O) { return O.OutcomeInterior; },
			Value, bNegate, TEXT("Interior"));
	}

	static TSharedPtr<IOutcomeCondition> SpawnGroup(EOutcomeSpawnGroup Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EOutcomeSpawnGroup>>(
			[](const FOutcomeEventBase& O) { return O.OutcomeSpawnGroup; },
			Value, bNegate, TEXT("SpawnGroup"));
	}

	// World state changes (Изменения мирового состояния)
	static TSharedPtr<IOutcomeCondition> WorldState(EWorldState Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EWorldState>>(
			[](const FOutcomeEventBase& O) { return O.WorldState; },
			Value, bNegate, TEXT("WorldState"));
	}

	static TSharedPtr<FAndCondition> And()  { return MakeShared<FAndCondition>(); }
	static TSharedPtr<FOrCondition>  Or()   { return MakeShared<FOrCondition>(); }

	static TSharedPtr<IOutcomeCondition> Not(TSharedPtr<IOutcomeCondition> Condition)
	{
		return MakeShared<FNotCondition>(Condition);
	}
};