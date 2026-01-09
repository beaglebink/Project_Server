#include "GameSaveSubsystem.h"
#include "ThisSaveGame.h"
#include "SaveInterface.h"
#include "SaveGameHelper.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "EngineUtils.h"
#include "Engine/ReflectionCapture.h"
#include "BookfaceSubsystem.h"
#include "InstantMessengerSubsystem.h"

void UGameSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    TArray<FString> Slots = GetAvailableSlots();
    CurrentSlot = Slots.Num() > 0 ? Slots[0] : TEXT("SaveGame1");
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
    UWorld* World = GetWorld();
    if (!World) return;

    // 1. Создаём объект сохранения
    UThisSaveGame* SaveGameObject = Cast<UThisSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UThisSaveGame::StaticClass()));
    if (!SaveGameObject) return;

    // 2. Собираем данные из мира через хелпер
    SaveGameObject->SavedActors = USaveGameHelper::SerializeWorld(World);

    // 3. Генерируем имя слота
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

    // 4. Сохраняем в слот
    UGameplayStatics::SaveGameToSlot(SaveGameObject, CurrentSlot, 0);

    UE_LOG(LogTemp, Log, TEXT("Сохранение выполнено в слот: %s"), *CurrentSlot);
}

void UGameSaveSubsystem::LoadGame()
{
    // 1. Загружаем объект сохранения из текущего слота
    UThisSaveGame* SaveGameObject = Cast<UThisSaveGame>(
        UGameplayStatics::LoadGameFromSlot(CurrentSlot, 0));
    if (!SaveGameObject) return;

    // 2. Получаем мир
    UWorld* World = GetWorld();
    if (!World) return;

    USaveGameHelper::ClearWorld(World, SaveGameObject->SavedActors);

    // 3. Восстанавливаем акторы и их компоненты через хелпер
    USaveGameHelper::DeserializeWorld(World, SaveGameObject->SavedActors);

    UE_LOG(LogTemp, Log, TEXT("Загрузка выполнена из слота: %s"), *CurrentSlot);
}

void UGameSaveSubsystem::DeleteSaveSlot(const FString& SlotName)
{
    // Путь к файлу сохранения
    FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    FString FullPath = SaveDir / (SlotName + TEXT(".sav"));

    // Проверяем, существует ли файл
    if (IFileManager::Get().FileExists(*FullPath))
    {
        bool bDeleted = IFileManager::Get().Delete(*FullPath);
        if (bDeleted)
        {
            UE_LOG(LogTemp, Log, TEXT("Слот %s успешно удалён."), *SlotName);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Не удалось удалить слот %s."), *SlotName);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Слот %s не найден."), *SlotName);
    }
}