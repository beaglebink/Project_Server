#pragma once
#include "OutcomeEventBase.h"
#include "ActorStateOutcome.generated.h"

// Outcome structure for actor state changes (Структура результата для изменений состояния актера)
USTRUCT(BlueprintType)
struct FActorStateOutcome : public FOutcomeEventBase
{
	GENERATED_BODY()

	// Actor identifier for which the event occurred (ID актёра, для которого произошло событие)
	UPROPERTY(BlueprintReadWrite)
	FGuid ActorId;

	// Type of state change (e.g., "DialogueStarted", "DialogueEnded", "EmotionChanged") (Тип изменения состояния (например: "DialogueStarted", "DialogueEnded", "EmotionChanged"))
	UPROPERTY(BlueprintReadWrite)
	FString StateChangeType;

	// Additional context — for example dialogue string ID or emotion key (Дополнительный контекст — например ID строки диалога или ключ эмоции)
	UPROPERTY(BlueprintReadWrite)
	FString ContextId;

	// Arbitrary data for extension (e.g., emotion level, animation name) (Произвольные данные для расширения (например уровень эмоции, имя анимации))
	UPROPERTY(BlueprintReadWrite)
	TMap<FString, FString> Payload;
};
