#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MissionEnvelopeTypes.h"
#include "MissionAsset.generated.h"

class UMissionController; // forward declaration

// ??? UMissionAsset ????????????????????????????????????????????????????????????
// PrimaryDataAsset для описания одной миссии.
// Содержит Envelope с настройками persistence.
// Идентификатор миссии в рантайме — FName имени ассета (GetFName()).
// Дизайнер никогда не работает с GUID напрямую.
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // Читаемое название миссии для UI и дебага
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Identity")
    FText DisplayName;

    // Краткое описание (для журнала, UI)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Identity")
    FText Description;

    // Envelope — вся конфигурация persistence.
    // Если Envelope пуст (JobSpacePolicy = None, Scope пуст) — поведение мира по умолчанию.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Envelope")
    FMissionEnvelope Envelope;

    // Приоритет разрешения конфликтов между несколькими активными миссиями.
    // Дублирует Envelope.Priority для удобства поиска в подсистеме.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Priority")
    int32 Priority = 0;

    // Укажите BP-класс контроллера миссии (наследник UMissionController).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Controller")
    TSubclassOf<UMissionController> ControllerClass;

    // Возвращает стабильный FName-идентификатор для использования в рантайме
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|Identity")
    FName GetMissionId() const { return GetFName(); }

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Mission", GetFName());
    }

#if WITH_EDITOR
    // Синхронизируем Priority в Envelope при изменении в DataAsset
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
    {
        Super::PostEditChangeProperty(PropertyChangedEvent);
        Envelope.Priority = Priority;
    }
#endif
};
