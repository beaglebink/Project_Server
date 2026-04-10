#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LocationEditorUtils.generated.h"

class UWorldMapAsset;
class UWorldRegionAsset;
class UStreetAsset;
class UInteriorSetAsset;
class UFloorAsset;

UCLASS()
class FPSKITALSREFACTORED_API ULocationEditorUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Авто-установка parent-ссылок и ParentLocationID/ParentContext для всей иерархии WorldMap.
    // Возвращает количество изменённых ассетов.
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationSystem|Editor")
    static int32 AutoFillParentsForWorldMap(UWorldMapAsset* WorldMap);

    // Вспомогательная: проходит одну улицу и ставит parent у InteriorSet'ов и spatial-элементов.
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationSystem|Editor")
    static int32 AutoFillParentsForStreet(UStreetAsset* Street);

    // Вспомогательная: проход по InteriorSet
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationSystem|Editor")
    static int32 AutoFillParentsForInteriorSet(UInteriorSetAsset* InteriorSet);

#if WITH_EDITOR
	/** Возвращает все ассеты типа UWorldMapAsset в проекте (editor-only). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationEditor|Query")
	static TArray<UWorldMapAsset*> GetAllWorldMapAssets();
#endif
};