#include "TeleportingSubsystem.h"
#include "SceneDataProvider.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"

void UTeleportingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UGameInstance* GI = GetGameInstance();
    if (!GI)
    {
        UE_LOG(LogTemp, Warning, TEXT("TeleportingSubsystem: GameInstance not found"));
        return;
    }

    if (!GI->Implements<USceneDataProvider>())
    {
        UE_LOG(LogTemp, Warning, TEXT("GameInstance does not implement SceneDataProvider"));
        return;
    }

    TSoftObjectPtr<UDataTable> TablePtr = ISceneDataProvider::Execute_GetLevelDataTable(GI);
    if (!TablePtr.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("Scene DataTable is not loaded, loading synchronously"));
        TablePtr.LoadSynchronous();
    }

    LoadedSceneTable = TablePtr.Get();
    if (LoadedSceneTable)
    {
        UE_LOG(LogTemp, Log, TEXT("Scene DataTable loaded: %s"), *LoadedSceneTable->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load Scene DataTable"));
    }

}

void UTeleportingSubsystem::Deinitialize()
{
    //LoadedTeleportTable = nullptr;
    Super::Deinitialize();
}

void UTeleportingSubsystem::RegistrationTeleportingDestination(AActor* Destination)
{
    if (Destination && !TeleportingDestinations.Contains(Destination))
    {
        TeleportingDestinations.Add(Destination);
    }
}

void UTeleportingSubsystem::UnregistrationTeleportingDestination(AActor* Destination)
{
    if (Destination && TeleportingDestinations.Contains(Destination))
    {
        TeleportingDestinations.Remove(Destination);
    }
}

/*
FTeleportTableRow UTeleportingSubsystem::GetTeleportRowByID(FName RowID) const
{
    if (!LoadedTeleportTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("TeleportingSubsystem: Teleport DataTable not available"));
        return FTeleportTableRow();
    }

    const FTeleportTableRow* Row = LoadedTeleportTable->FindRow<FTeleportTableRow>(RowID, TEXT("TeleportingSubsystem Row Lookup"));
    return Row ? *Row : FTeleportTableRow();
}


*/