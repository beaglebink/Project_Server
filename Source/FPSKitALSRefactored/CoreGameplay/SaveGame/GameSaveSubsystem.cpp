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
    UThisSaveGame* SaveGameObject = Cast<UThisSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UThisSaveGame::StaticClass()));

    UWorld* World = GetWorld();
    if (!World) return;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor) continue;
        if (!USaveGameHelper::IsActorEligibleForSave(Actor)) continue;

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

        if (AActor* Parent = Actor->GetAttachParentActor())
        {
            Data.AttachParentID = Parent->GetName();
            if (USceneComponent* RootComp = Actor->GetRootComponent())
            {
                Data.AttachSocketName = RootComp->GetAttachSocketName();
            }
        }

        SaveGameObject->SavedActors.Add(Data);
    }

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

    UE_LOG(LogTemp, Log, TEXT("Сохранение выполнено в слот: %s"), *CurrentSlot);
}

void UGameSaveSubsystem::LoadGame()
{
    UThisSaveGame* SaveGameObject = Cast<UThisSaveGame>(
        UGameplayStatics::LoadGameFromSlot(CurrentSlot, 0));
    if (!SaveGameObject) return;

    UWorld* World = GetWorld();
    if (!World) return;

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
            if (ActorClass && USaveGameHelper::IsActorEligibleForSave(ActorClass->GetDefaultObject<AActor>()))
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

            if (!Data.AttachParentID.IsEmpty())
            {
                for (TActorIterator<AActor> It(World); It; ++It)
                {
                    if (It->GetName() == Data.AttachParentID)
                    {
                        AActor* Parent = *It;
                        if (USceneComponent* ParentComp = Parent->GetRootComponent())
                        {
                            Actor->AttachToComponent(
                                ParentComp,
                                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                                Data.AttachSocketName
                            );
                        }
                        break;
                    }
                }
            }
        }
    }
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