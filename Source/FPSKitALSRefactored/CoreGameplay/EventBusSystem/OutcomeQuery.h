#pragma once

#include "CoreMinimal.h"
#include "Outcome.h"
#include "OutcomeEventBase.h"

// Base condition interface
// (Базовый интерфейс условия)
class IOutcomeCondition
{
public:
	virtual ~IOutcomeCondition() = default;
	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const = 0;
	virtual FString Describe() const = 0;
};

// Template for simple enum field conditions with Equals/NotEquals support
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
	TEnumType     Value;
	bool          bNegate;
	const TCHAR*  FieldName;
};

// ===== LOGICAL CONDITIONS =====

// AND condition - all children must pass
// (AND условие - все дочерние условия должны выполниться)
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

// OR condition - at least one child must pass
// (OR условие - хотя бы одно дочернее условие должно выполниться)
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

// NOT condition - inverts a single child condition
// (NOT условие - инвертирует дочернее условие)
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
// Factory methods for building conditions from enum values
// (Фабричные методы для построения условий из значений enum)
struct FOutcomeQueryBuilder
{
	// Event type filter
	// (Фильтр по типу события)
	static TSharedPtr<IOutcomeCondition> Type(EOutcomeType Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EOutcomeType>>(
			[](const FOutcomeEventBase& O) { return O.OutcomeType; },
			Value, bNegate, TEXT("OutcomeType"));
	}

	// Mission category filter
	// (Фильтр по категории миссии)
	static TSharedPtr<IOutcomeCondition> Mission(EOutcomeMission Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EOutcomeMission>>(
			[](const FOutcomeEventBase& O) { return O.OutcomeMission; },
			Value, bNegate, TEXT("Mission"));
	}

	// Actor category filter
	// (Фильтр по категории актёра)
	static TSharedPtr<IOutcomeCondition> Actor(EOutcomeActor Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EOutcomeActor>>(
			[](const FOutcomeEventBase& O) { return O.OutcomeActor; },
			Value, bNegate, TEXT("Actor"));
	}

	// Object category filter
	// (Фильтр по категории объекта)
	static TSharedPtr<IOutcomeCondition> Object(EOutcomeInventory Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EOutcomeInventory>>(
			[](const FOutcomeEventBase& O) { return O.OutcomeInventory; },
			Value, bNegate, TEXT("Object"));
	}

	// Terminal category filter
	// (Фильтр по категории терминала)
	static TSharedPtr<IOutcomeCondition> Terminal(EOutcomeTerminal Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EOutcomeTerminal>>(
			[](const FOutcomeEventBase& O) { return O.OutcomeTerminal; },
			Value, bNegate, TEXT("Terminal"));
	}

	// Interior category filter
	// (Фильтр по категории интерьера)
	static TSharedPtr<IOutcomeCondition> Interior(EOutcomeInterior Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EOutcomeInterior>>(
			[](const FOutcomeEventBase& O) { return O.OutcomeInterior; },
			Value, bNegate, TEXT("Interior"));
	}

	// Spawn group category filter
	// (Фильтр по категории группы спавна)
	static TSharedPtr<IOutcomeCondition> SpawnGroup(EOutcomeSpawnGroup Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EOutcomeSpawnGroup>>(
			[](const FOutcomeEventBase& O) { return O.OutcomeSpawnGroup; },
			Value, bNegate, TEXT("SpawnGroup"));
	}

	// World state category filter
	// (Фильтр по категории состояния мира)
	static TSharedPtr<IOutcomeCondition> WorldState(EWorldState Value, bool bNegate = false)
	{
		return MakeShared<TFieldCondition<EWorldState>>(
			[](const FOutcomeEventBase& O) { return O.WorldState; },
			Value, bNegate, TEXT("WorldState"));
	}

	static TSharedPtr<FAndCondition> And() { return MakeShared<FAndCondition>(); }
	static TSharedPtr<FOrCondition>  Or()  { return MakeShared<FOrCondition>(); }

	static TSharedPtr<IOutcomeCondition> Not(TSharedPtr<IOutcomeCondition> Condition)
	{
		return MakeShared<FNotCondition>(Condition);
	}
};