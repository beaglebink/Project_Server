#include "ShareLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/FileManagerGeneric.h"
#include "Misc/Paths.h"

void UShareLibrary::AnalyzeAssets()
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> AssetDataList;
    FARFilter Filter;
    Filter.bRecursiveClasses = true;

    // Добавляем дополнительные пути к фильтру
    Filter.PackagePaths.Add(TEXT("/Game"));
    Filter.PackagePaths.Add(TEXT("/Game/MultiplayerFPS"));
    Filter.PackagePaths.Add(TEXT("/Game/MultiplayerFPS/Game"));
    Filter.PackagePaths.Add(TEXT("/Game/RefactoredALS"));

    // Вызов ScanPathsSynchronous для загрузки ассетов
    AssetRegistry.ScanPathsSynchronous({ TEXT("/Game"), TEXT("/Game/MultiplayerFPS"), TEXT("/Game/MultiplayerFPS/Game"), TEXT("/Game/RefactoredALS") }, true);

    UE_LOG(LogTemp, Warning, TEXT("Scanning assets in specified directories..."));

    AssetRegistry.GetAssets(Filter, AssetDataList);

    UE_LOG(LogTemp, Warning, TEXT("Number of assets found: %d"), AssetDataList.Num());

    UE_LOG(LogTemp, Warning, TEXT("Analyzing assets for hard references..."));

    for (const FAssetData& AssetData : AssetDataList)
    {
        TArray<FAssetIdentifier> HardReferences;
        AssetRegistry.GetReferencers(AssetData.PackageName, HardReferences, UE::AssetRegistry::EDependencyCategory::Package, UE::AssetRegistry::FDependencyQuery(UE::AssetRegistry::EDependencyQuery::Hard));

        if (HardReferences.Num() > 0)
        {
            // Получение пути к файлу и проверка его размера
            FString AssetPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + AssetData.PackageName.ToString().Replace(TEXT("/Game/"), TEXT("")) + TEXT(".uasset"));
            int64 AssetSize = IFileManager::Get().FileSize(*AssetPath);

            if (AssetSize != INDEX_NONE)
            {
                UE_LOG(LogTemp, Warning, TEXT("Asset %s (Size: %lld bytes) has hard references:"), *AssetData.PackageName.ToString(), AssetSize);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to get size for asset %s"), *AssetData.PackageName.ToString());
            }

            for (const FAssetIdentifier& Reference : HardReferences)
            {
                UE_LOG(LogTemp, Warning, TEXT("\t%s"), *Reference.ToString());
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("Asset analysis complete."));
}
