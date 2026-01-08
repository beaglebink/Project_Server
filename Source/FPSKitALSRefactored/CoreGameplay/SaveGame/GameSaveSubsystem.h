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
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable)
    void SaveGame();

    UFUNCTION(BlueprintCallable)
    void LoadGame();

    UFUNCTION(BlueprintCallable)
    void DeleteSaveSlot(const FString& SlotName);

    UFUNCTION(BlueprintCallable)
    void SetActiveSlot(const FString& SlotName);

    UFUNCTION(BlueprintCallable)
    FString GetActiveSlot() const { return CurrentSlot; }

    UFUNCTION(BlueprintCallable)
    TArray<FString> GetAvailableSlots() const;

    UFUNCTION(BlueprintCallable)
    TArray<FSaveSlotInfo> GetSaveSlotsForUI() const;

public:
    UPROPERTY()
    FString CurrentSlot;
};