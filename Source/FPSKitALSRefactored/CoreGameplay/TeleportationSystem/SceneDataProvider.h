// SceneDataProvider.h

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Engine/DataTable.h"
#include "SceneDataProvider.generated.h"

UINTERFACE(BlueprintType)
class FPSKITALSREFACTORED_API USceneDataProvider : public UInterface
{
    GENERATED_BODY()
};

class FPSKITALSREFACTORED_API ISceneDataProvider
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Teleporting system")
    UDataTable* TeleportDataTable() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Dictionary system")
    UDataTable* DictionaryDataTable() const;
};
