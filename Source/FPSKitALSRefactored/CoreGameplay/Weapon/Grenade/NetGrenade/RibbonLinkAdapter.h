#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SimulationTypes.h" // ← содержит FNode и FNodeLink
#include "NiagaraDataInterface_RibbonLinks.h"
#include "RibbonLinkAdapter.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API URibbonLinkAdapter : public UObject
{
    GENERATED_BODY()

public:
    // 🔗 Генерация и передача связей
    void UpdateRibbonLinks(const TArray<FNode>& Nodes, UNiagaraDataInterface_RibbonLinks* DataInterface);
};