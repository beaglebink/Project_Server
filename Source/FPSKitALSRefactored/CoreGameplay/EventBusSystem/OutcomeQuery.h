#pragma once

#include "CoreMinimal.h"
#include "Outcome.h"
#include "OutcomeEventBase.h"

/**
 * Система условных фильтров для событий
 * Позволяет создавать сложные логические условия И/ИЛИ для комбинаций перечислений
 */

// Базовый интерфейс условия
class IOutcomeCondition
{
public:
	virtual ~IOutcomeCondition() = default;
	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const = 0;
};

// Объявление базового шаблона (без определения)
template<typename EnumType>
class TOutcomeCondition;

// Специализация для EOutcomeType
template<>
class TOutcomeCondition<EOutcomeType> : public IOutcomeCondition
{
public:
	TOutcomeCondition(EOutcomeType Value) : TargetValue(Value) {}

	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
	{
		return Outcome.OutcomeType == TargetValue;
	}

private:
	EOutcomeType TargetValue;
};

// Специализация для EOutcomeMission
template<>
class TOutcomeCondition<EOutcomeMission> : public IOutcomeCondition
{
public:
	TOutcomeCondition(EOutcomeMission Value) : TargetValue(Value) {}

	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
	{
		return Outcome.OutcomeMission == TargetValue;
	}

private:
	EOutcomeMission TargetValue;
};

// Специализация для EOutcomeActor
template<>
class TOutcomeCondition<EOutcomeActor> : public IOutcomeCondition
{
public:
	TOutcomeCondition(EOutcomeActor Value) : TargetValue(Value) {}

	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
	{
		return Outcome.OutcomeActor == TargetValue;
	}

private:
	EOutcomeActor TargetValue;
};

// Специализация для EOutcomeObject
template<>
class TOutcomeCondition<EOutcomeObject> : public IOutcomeCondition
{
public:
	TOutcomeCondition(EOutcomeObject Value) : TargetValue(Value) {}

	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
	{
		return Outcome.OutcomeObject == TargetValue;
	}

private:
	EOutcomeObject TargetValue;
};

// Specialization for EOutcomeInterior (Специализация для EOutcomeInterior)
template<>
class TOutcomeCondition<EOutcomeInterior> : public IOutcomeCondition
{
public:
	TOutcomeCondition(EOutcomeInterior Value) : TargetValue(Value) {}

	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
	{
		return Outcome.OutcomeInterior == TargetValue;
	}

private:
	EOutcomeInterior TargetValue;
};

// Specialization for EOutcomeSpawnGroup (Специализация для EOutcomeSpawnGroup)
template<>
class TOutcomeCondition<EOutcomeSpawnGroup> : public IOutcomeCondition
{
public:
	TOutcomeCondition(EOutcomeSpawnGroup Value) : TargetValue(Value) {}

	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
	{
		return Outcome.OutcomeSpawnGroup == TargetValue;
	}

private:
	EOutcomeSpawnGroup TargetValue;
};

// Логическое И (все условия должны быть истинны)
class FAndCondition : public IOutcomeCondition, public TSharedFromThis<FAndCondition>
{
public:
	FAndCondition() {}

	TSharedPtr<FAndCondition> Add(TSharedPtr<IOutcomeCondition> Condition)
	{
		if (Condition)
		{
			Conditions.Add(Condition);
		}
		return SharedThis(this);
	}

	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
	{
		if (Conditions.IsEmpty())
		{
			return true;
		}

		for (const auto& Condition : Conditions)
		{
			if (!Condition->Evaluate(Outcome))
			{
				return false;
			}
		}
		return true;
	}

private:
	TArray<TSharedPtr<IOutcomeCondition>> Conditions;
};

// Логическое ИЛИ (хотя бы одно условие должно быть истинно)
class FOrCondition : public IOutcomeCondition, public TSharedFromThis<FOrCondition>
{
public:
	FOrCondition() {}

	TSharedPtr<FOrCondition> Add(TSharedPtr<IOutcomeCondition> Condition)
	{
		if (Condition)
		{
			Conditions.Add(Condition);
		}
		return SharedThis(this);
	}

	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
	{
		if (Conditions.IsEmpty())
		{
			return false;
		}

		for (const auto& Condition : Conditions)
		{
			if (Condition->Evaluate(Outcome))
			{
				return true;
			}
		}
		return false;
	}

private:
	TArray<TSharedPtr<IOutcomeCondition>> Conditions;
};

// Логическое НЕ (инверсия условия)
class FNotCondition : public IOutcomeCondition
{
public:
	FNotCondition(TSharedPtr<IOutcomeCondition> InCondition) : Condition(InCondition) {}

	virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
	{
		return Condition ? !Condition->Evaluate(Outcome) : true;
	}

private:
	TSharedPtr<IOutcomeCondition> Condition;
};

// Построитель запросов (Fluent API)
class FOutcomeQueryBuilder
{
public:
	// Начать новый запрос с И-условия
	static TSharedPtr<FAndCondition> And()
	{
		return MakeShared<FAndCondition>();
	}

	// Начать новый запрос с ИЛИ-условия
	static TSharedPtr<FOrCondition> Or()
	{
		return MakeShared<FOrCondition>();
	}

	// Условие для типа события
	static TSharedPtr<IOutcomeCondition> Type(EOutcomeType Value)
	{
		return MakeShared<TOutcomeCondition<EOutcomeType>>(Value);
	}

	// Условие для миссии
	static TSharedPtr<IOutcomeCondition> Mission(EOutcomeMission Value)
	{
		return MakeShared<TOutcomeCondition<EOutcomeMission>>(Value);
	}

	// Условие для актёра
	static TSharedPtr<IOutcomeCondition> Actor(EOutcomeActor Value)
	{
		return MakeShared<TOutcomeCondition<EOutcomeActor>>(Value);
	}

	// Условие для объекта
	static TSharedPtr<IOutcomeCondition> Object(EOutcomeObject Value)
	{
		return MakeShared<TOutcomeCondition<EOutcomeObject>>(Value);
	}

	// Условие для интерьера
	static TSharedPtr<IOutcomeCondition> Interior(EOutcomeInterior Value)
	{
		return MakeShared<TOutcomeCondition<EOutcomeInterior>>(Value);
	}

	// Условие для группы спауна
	static TSharedPtr<IOutcomeCondition> SpawnGroup(EOutcomeSpawnGroup Value)
	{
		return MakeShared<TOutcomeCondition<EOutcomeSpawnGroup>>(Value);
	}

	// Логическое НЕ
	static TSharedPtr<FNotCondition> Not(TSharedPtr<IOutcomeCondition> Condition)
	{
		return MakeShared<FNotCondition>(Condition);
	}
};