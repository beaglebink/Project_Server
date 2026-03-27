#pragma once
#include "OutcomeEventBase.h"
#include "ActorStateOutcome.generated.h"

USTRUCT(BlueprintType)
struct FActorStateOutcome : public FOutcomeEventBase
{
    GENERATED_BODY()

    // ID актёра, для которого произошло событие
    UPROPERTY(BlueprintReadWrite)
    FGuid ActorId;

    // Тип изменения состояния (например: "DialogueStarted", "DialogueEnded", "EmotionChanged")
    UPROPERTY(BlueprintReadWrite)
    FString StateChangeType;

    // Дополнительный контекст — например ID строки диалога или ключ эмоции
    UPROPERTY(BlueprintReadWrite)
    FString ContextId;

    // Произвольные данные для расширения (например уровень эмоции, имя анимации)
    UPROPERTY(BlueprintReadWrite)
    TMap<FString, FString> Payload;
};
