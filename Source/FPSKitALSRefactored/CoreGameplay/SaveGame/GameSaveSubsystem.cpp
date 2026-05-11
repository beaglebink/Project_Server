#include "GameSaveSubsystem.h"
#include "ThisSaveGame.h"
#include "SaveInterface.h"
//#include "SaveGameHelper.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "EngineUtils.h"

void UGameSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    TArray<FString> Slots = GetAvailableSlots();
    CurrentSlot = Slots.Num() > 0 ? Slots[0] : TEXT("SaveGame1");
}

void UGameSaveSubsystem::RegisterSaveableSubsystem(ISaveableSubsystem* Subsystem)
{
    if (Subsystem && !SaveableSubsystems.Contains(Subsystem))
    {
        SaveableSubsystems.Add(Subsystem);
        UE_LOG(LogTemp, Log,
            TEXT("GameSaveSubsystem: Registered subsystem '%s'"),
            *Subsystem->GetSaveSubsystemName());
    }
}

void UGameSaveSubsystem::UnregisterSaveableSubsystem(ISaveableSubsystem* Subsystem)
{
    SaveableSubsystems.Remove(Subsystem);
}

void UGameSaveSubsystem::SetActiveSlot(const FString& SlotName)
{
    CurrentSlot = SlotName;
}

TArray<FString> UGameSaveSubsystem::GetAvailableSlots() const
{
    TArray<FString> FoundSlots;
    FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    TArray<FString> Files;
    IFileManager::Get().FindFiles(Files, *SaveDir, TEXT("sav"));

    for (const FString& File : Files)
    {
        FoundSlots.Add(FPaths::GetBaseFilename(File));
    }
    return FoundSlots;
}

TArray<FSaveSlotInfo> UGameSaveSubsystem::GetSaveSlotsForUI() const
{
    TArray<FSaveSlotInfo> SlotsInfo;
    FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    TArray<FString> Files;
    IFileManager::Get().FindFiles(Files, *SaveDir, TEXT("sav"));

    for (const FString& File : Files)
    {
        FString FullPath = SaveDir / File;
        FSaveSlotInfo Info;
        Info.SlotName = FPaths::GetBaseFilename(File);
        Info.SaveTime = IFileManager::Get().GetTimeStamp(*FullPath);
        int64 Size = IFileManager::Get().FileSize(*FullPath);
        Info.FileSize = (Size >= 0) ? static_cast<int32>(Size) : 0;
        SlotsInfo.Add(Info);
    }
    return SlotsInfo;
}

void UGameSaveSubsystem::SaveGame()
{
    UThisSaveGame* SaveGameObject = Cast<UThisSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UThisSaveGame::StaticClass()));
    if (!SaveGameObject) return;

    // Собираем данные у всех зарегистрированных подсистем
    for (ISaveableSubsystem* Subsystem : SaveableSubsystems)
    {
        if (!Subsystem) continue;
        FSubsystemSaveData Data;
        Subsystem->CollectSaveData(Data);
        if (!Data.SubsystemName.IsEmpty())
        {
            SaveGameObject->SavedSubsystems.Add(Data);
        }
    }

    // Генерируем имя слота
    FString TimeStamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
    FString BaseSlotName = FString::Printf(TEXT("Save_%s"), *TimeStamp);

    FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    FString FullPath = SaveDir / (BaseSlotName + TEXT(".sav"));

    int32 Counter = 1;
    FString FinalSlotName = BaseSlotName;

    while (IFileManager::Get().FileExists(*FullPath))
    {
        FinalSlotName = FString::Printf(TEXT("%s_%d"), *BaseSlotName, Counter);
        FullPath = SaveDir / (FinalSlotName + TEXT(".sav"));
        Counter++;
    }

    CurrentSlot = FinalSlotName;

    UGameplayStatics::SaveGameToSlot(SaveGameObject, CurrentSlot, 0);

    UE_LOG(LogTemp, Log, TEXT("GameSaveSubsystem: Saved to slot '%s' (%d subsystems)"),
        *CurrentSlot, SaveableSubsystems.Num());
}

void UGameSaveSubsystem::LoadGame()
{
    UThisSaveGame* SaveGameObject = Cast<UThisSaveGame>(
        UGameplayStatics::LoadGameFromSlot(CurrentSlot, 0));
    if (!SaveGameObject)
    {
        UE_LOG(LogTemp, Warning, TEXT("GameSaveSubsystem: Cannot load slot '%s'"), *CurrentSlot);
        return;
    }

    // Раздаём данные зарегистрированным подсистемам по имени
    for (const FSubsystemSaveData& Data : SaveGameObject->SavedSubsystems)
    {
        for (ISaveableSubsystem* Subsystem : SaveableSubsystems)
        {
            if (Subsystem && Subsystem->GetSaveSubsystemName() == Data.SubsystemName)
            {
                Subsystem->ApplySaveData(Data);
                break;
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("GameSaveSubsystem: Loaded from slot '%s' (%d subsystem blocks)"),
        *CurrentSlot, SaveGameObject->SavedSubsystems.Num());
}

void UGameSaveSubsystem::DeleteSaveSlot(const FString& SlotName)
{
    FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    FString FullPath = SaveDir / (SlotName + TEXT(".sav"));

    if (IFileManager::Get().FileExists(*FullPath))
    {
        bool bDeleted = IFileManager::Get().Delete(*FullPath);
        if (bDeleted)
        {
            UE_LOG(LogTemp, Log, TEXT("GameSaveSubsystem: Slot '%s' deleted"), *SlotName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("GameSaveSubsystem: Failed to delete slot '%s'"), *SlotName);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GameSaveSubsystem: Slot '%s' not found"), *SlotName);
    }
}
