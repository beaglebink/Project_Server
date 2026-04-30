#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MissionEnvelopeTypes.h"
#include "MissionAsset.generated.h"

class UMissionController; // Forward declaration (Предварительное объявление)

// UMissionAsset data asset for describing a single mission.
// Contains an Envelope with persistence settings.
// The mission identifier at runtime is the asset name FName (GetFName()).
// The designer never works with the GUID directly.
// PrimaryDataAsset для описания одной миссии.
// Содержит Envelope с настройками persistence.
// Идентификатор миссии в рантайме — FName имени ассета (GetFName()).
// Дизайнер никогда не работает с GUID напрямую.
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // Human-readable mission name for UI and debugging.
    // Читаемое название миссии для UI и дебага.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Identity")
    FText DisplayName;

    // Short description for the journal and UI.
    // Краткое описание для журнала и UI.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Identity")
    FText Description;

    // Envelope contains the full persistence configuration.
    // If the Envelope is empty (JobSpacePolicy = None, Scope is empty), the world uses default behavior.
    // The Envelope array contains all configurations of the mission steps.
	// Массив Envelope содержит все конфигурации шагов миссии.
    // Envelope содержит всю конфигурацию persistence.
    // Если Envelope пуст (JobSpacePolicy = None, Scope пуст), используется поведение мира по умолчанию.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Envelope")
    TArray<FMissionEnvelope> Envelopes;

    // Priority for resolving conflicts between several active missions.
    // Duplicates Envelope.Priority for easier lookup in the subsystem.
    // Приоритет разрешения конфликтов между несколькими активными миссиями.
    // Дублирует Envelope.Priority для удобства поиска в подсистеме.
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Priority")
    //int32 Priority = 0;

    // Specify the mission controller BP class inherited from UMissionController.
    // Укажите BP-класс контроллера миссии, наследник UMissionController.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Controller")
    TSubclassOf<UMissionController> ControllerClass;

    // Returns a stable FName identifier for runtime use.
    // Возвращает стабильный FName-идентификатор для использования в рантайме.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|Identity")
    FName GetMissionId() const { return DisplayName.IsEmpty() ? GetFName() : FName(*DisplayName.ToString()); }

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Mission", GetFName());
    }

#if WITH_EDITOR
    // Synchronizes Priority into Envelope when the DataAsset is changed.
    // Синхронизирует Priority в Envelope при изменении DataAsset.
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
    {
        Super::PostEditChangeProperty(PropertyChangedEvent);
        //Envelope.Priority = Priority;
    }
#endif
};
