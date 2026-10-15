#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
//#include <ScenarioManagerSubsystem/PayloadBatchEntry.h>
#include "OutcomePayload.generated.h"

struct FCommandSpec;
class UPayloadBatchEntry;

// Base class for all outcome payload objects
// Both C++ and Blueprint inherit from this to attach extra data to events
// (Базовый класс для всех объектов payload событий)
// (C++ и Blueprint наследуют его для передачи дополнительных данных)

UCLASS(BlueprintType, Blueprintable) 
class FPSKITALSREFACTORED_API UOutcomePayload : public UObject
{
    GENERATED_BODY()

public:
    // Вызывается перед публикацией; по умолчанию ничего не делает.
    virtual void OnBeforePublish(const FCommandSpec* Spec, const UPayloadBatchEntry* Entry) {}
};