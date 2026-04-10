#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FloorAssignerEditorLibrary.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UFloorAssignerEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	// Возвращает карту WorldMap GUID -> DisplayName
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
	static TMap<FGuid, FText> GetWorldMaps();

	// Публикует FloorPlacementUnregistered для всех выделенных акторов с FloorAssignmentComponent
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
	static int32 UnregisterSelectedActors();

	// Выделяет в редакторе все акторы, у которых FloorAssignmentComponent.FloorId == FloorGuid
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
	static int32 SelectActorsByFloor(const FGuid& FloorGuid);

	// Остальные editor-only функции...
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
	static TMap<FGuid, FText> GetRegions(const FGuid& WorldMapGuid);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
	static TMap<FGuid, FText> GetStreets(const FGuid& WorldRegionGuid);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
	static TMap<FGuid, FText> GetInteriorSets(const FGuid& StreetGuid);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
	static TMap<FGuid, FText> GetFloors(const FGuid& InteriorSetGuid);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
	static int32 ApplyFloorToSelectedActors(const FGuid& FloorGuid, const FGuid& InteriorSetGuid);
#endif
};