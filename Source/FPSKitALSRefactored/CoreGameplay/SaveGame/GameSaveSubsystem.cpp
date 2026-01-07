#include "GameSaveSubsystem.h"
#include "ThisSaveGame.h"
#include "SaveInterface.h"
#include "SaveGameHelper.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "EngineUtils.h"

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
    UThisSaveGame* SaveGameObject = Cast<UThisSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UThisSaveGame::StaticClass()));

    UWorld* World = GetWorld();
    if (!World) return;

    // Сохраняем акторы
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor) continue;

        FActorSaveData Data;
        Data.ClassName = Actor->GetClass()->GetPathName();
        Data.Transform = Actor->GetActorTransform();

        if (Actor->Implements<USaveInterface>())
        {
            ISaveInterface* Saveable = Cast<ISaveInterface>(Actor);
            Saveable->OnPreSave();
            Data.UniqueID = Saveable->GetSaveID();
            Data.SerializedData = Saveable->SaveToJson();
        }
        else
        {
            Data.UniqueID = Actor->GetName();
            Data.SerializedData = USaveGameHelper::SerializeActor(Actor);
        }

        Data.SavedComponents = USaveGameHelper::SerializeComponents(Actor);
        SaveGameObject->SavedActors.Add(Data);
    }

    if (auto* BookfaceSubsystem = GetGameInstance()->GetSubsystem<UBookfaceSubsystem>())
    {
        FSubsystemSaveData Data;
        Data.SubsystemName = BookfaceSubsystem->GetClass()->GetName();
        // stub
        Data.SerializedData = TEXT("{}");
        SaveGameObject->SavedSubsystems.Add(Data);
    }

    // Сохраняем InstantMessengerSubsystem
    if (auto* MessengerSubsystem = GetGameInstance()->GetSubsystem<UInstantMessengerSubsystem>())
    {
        FSubsystemSaveData Data;
        Data.SubsystemName = MessengerSubsystem->GetClass()->GetName();
        // stub
        Data.SerializedData = TEXT("{}");
        SaveGameObject->SavedSubsystems.Add(Data);
    }

    UGameplayStatics::SaveGameToSlot(SaveGameObject, CurrentSlot, 0);
}

void UGameSaveSubsystem::LoadGame()
{
    UThisSaveGame* SaveGameObject = Cast<UThisSaveGame>(
        UGameplayStatics::LoadGameFromSlot(CurrentSlot, 0));
    if (!SaveGameObject) return;

    UWorld* World = GetWorld();
    if (!World) return;

    // Восстанавливаем акторы
    for (const FActorSaveData& Data : SaveGameObject->SavedActors)
    {
        AActor* Actor = nullptr;

        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->GetName() == Data.UniqueID)
            {
                Actor = *It;
                break;
            }
        }

        if (!Actor)
        {
            UClass* ActorClass = LoadObject<UClass>(nullptr, *Data.ClassName);
            if (ActorClass)
            {
                FActorSpawnParameters Params;
                Actor = World->SpawnActor<AActor>(ActorClass, Data.Transform, Params);
            }
        }

        if (Actor)
        {
            if (Actor->Implements<USaveInterface>())
            {
                ISaveInterface* Saveable = Cast<ISaveInterface>(Actor);
                Saveable->LoadFromJson(Data.SerializedData);
                Saveable->OnPostLoad();
            }
            else
            {
                USaveGameHelper::DeserializeActor(Actor, Data.SerializedData);
            }

            Actor->SetActorTransform(Data.Transform);
            USaveGameHelper::DeserializeComponents(Actor, Data.SavedComponents);
        }
    }

    // restore BookfaceSubsystem
    if (auto* BookfaceSubsystem = GetGameInstance()->GetSubsystem<UBookfaceSubsystem>())
    {
        for (const FSubsystemSaveData& Data : SaveGameObject->SavedSubsystems)
        {
            if (Data.SubsystemName == BookfaceSubsystem->GetClass()->GetName())
            {
                // stub
                // BookfaceSubsystem->LoadFromJson(Data.SerializedData);
            }
        }
    }

    // restore InstantMessengerSubsystem
    if (auto* MessengerSubsystem = GetGameInstance()->GetSubsystem<UInstantMessengerSubsystem>())
    {
        for (const FSubsystemSaveData& Data : SaveGameObject->SavedSubsystems)
        {
            if (Data.SubsystemName == MessengerSubsystem->GetClass()->GetName())
            {
                // stub
                // MessengerSubsystem->LoadFromJson(Data.SerializedData);
            }
        }
    }
}