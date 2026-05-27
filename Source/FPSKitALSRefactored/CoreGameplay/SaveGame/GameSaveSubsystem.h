#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ThisSaveGame.h"
#include "ISaveableSubsystem.h"
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
    int32 FileSize = 0;
};

// ??? UGameSaveSubsystem ????????????????????????????????????????????????????????
// Отвечает ТОЛЬКО за сериализацию/десериализацию данных подсистем на диск.
// Не знает о содержимом данных — собирает блоки через ISaveableSubsystem.
// MissionSubsystem, InteriorSubsystem, WorldStateSubsystem и др. регистрируются
// вызовом RegisterSaveableSubsystem() при инициализации.
UCLASS()
class FPSKITALSREFACTORED_API UGameSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // Регистрация подсистем 
    // Подсистема вызывает это сама при инициализации (или её вызывает GameMode)
    void RegisterSaveableSubsystem(ISaveableSubsystem* Subsystem);
    void UnregisterSaveableSubsystem(ISaveableSubsystem* Subsystem);

    // Сохранение / Загрузка 

    // Собирает данные у всех зарегистрированных подсистем и сохраняет на диск.
    // Создаёт новый слот с timestamp-именем.
    UFUNCTION(BlueprintCallable, Category = "SaveGame")
    void SaveGame();

    // Читает файл текущего слота и раздаёт данные зарегистрированным подсистемам.
    UFUNCTION(BlueprintCallable, Category = "SaveGame")
    void LoadGame();

    UFUNCTION(BlueprintCallable, Category = "SaveGame")
    void DeleteSaveSlot(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category = "SaveGame")
    void SetActiveSlot(const FString& SlotName);

    UFUNCTION(BlueprintCallable, Category = "SaveGame")
    FString GetActiveSlot() const { return CurrentSlot; }

    UFUNCTION(BlueprintCallable, Category = "SaveGame")
    TArray<FString> GetAvailableSlots() const;

    UFUNCTION(BlueprintCallable, Category = "SaveGame")
    TArray<FSaveSlotInfo> GetSaveSlotsForUI() const;

public:
    UPROPERTY()
    FString CurrentSlot;

private:
    // Зарегистрированные подсистемы (raw pointers — UObject подсистемы живут дольше)
    TArray<ISaveableSubsystem*> SaveableSubsystems;
    
    UPROPERTY()
    FTimerHandle TimerHandle;
};