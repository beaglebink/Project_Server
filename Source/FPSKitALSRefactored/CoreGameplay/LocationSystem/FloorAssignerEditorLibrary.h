#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FloorAssignerEditorLibrary.generated.h"

class UWorldMapAsset;

/**
 * Editor-only helper functions for Floor assignment.
 * Functions are exposed to Blueprint (CallInEditor) and are available in Editor Utility Widgets.
 */
UCLASS()
class FPSKITALSREFACTORED_API UFloorAssignerEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	// Возвращает карту WorldMap GUID -> DisplayName
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
	static TMap<FGuid, FText> GetWorldMaps();

	// Возвращает карту Region GUID -> DisplayName для выбранного WorldMap
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
	static TMap<FGuid, FText> GetRegions(const FGuid& WorldMapGuid);

	// Возвращает карту Street GUID -> DisplayName для выбранного Region
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
	static TMap<FGuid, FText> GetStreets(const FGuid& WorldRegionGuid);

	// Возвращает карту InteriorSet GUID -> DisplayName для выбранной Street
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
	static TMap<FGuid, FText> GetInteriorSets(const FGuid& StreetGuid);

	// Возвращает карту Floor GUID -> "Index — Name" для выбранного InteriorSet
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
	static TMap<FGuid, FText> GetFloors(const FGuid& InteriorSetGuid);

	// Применяет FloorId и InteriorSetId ко всем выделенным актёрам (записывает в FloorAssignmentComponent)
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
	static int32 ApplyFloorToSelectedActors(const FGuid& FloorGuid, const FGuid& InteriorSetGuid);
#endif
};