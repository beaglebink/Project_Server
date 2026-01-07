#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ThisSaveGame.h"
#include "GameSaveSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FSaveSlotInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString SlotName;

    UPROPERTY(BlueprintReadOnly)
    FDateTime SaveTime;

    UPROPERTY(BlueprintReadOnly)
    int32 FileSize;
};

UCLASS()
class FPSKITALSREFACTORED_API UGameSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FString CurrentSlot;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    void SaveGame();
    void LoadGame();

    void SetActiveSlot(const FString& SlotName);
    FString GetActiveSlot() const { return CurrentSlot; }

    TArray<FString> GetAvailableSlots() const;
    TArray<FSaveSlotInfo> GetSaveSlotsForUI() const;
};