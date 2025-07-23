#pragma once

#include "CoreMinimal.h"
#include "NiagaraDataInterface.h"
#include "NiagaraDataInterface_RibbonLinks.generated.h"

USTRUCT(BlueprintType)
struct FNiagaraRibbonLink
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StartIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EndIndex;
};

UCLASS(EditInlineNew, BlueprintType, Category = "Custom", meta = (DisplayName = "Ribbon Links"))
class FPSKITALSREFACTORED_API UNiagaraDataInterface_RibbonLinks : public UNiagaraDataInterface
{
    GENERATED_BODY()

public:
    UNiagaraDataInterface_RibbonLinks();

    // 🧩 Храним связи как массив пар индексов
    UPROPERTY(EditAnywhere, Category = "Data")
    TArray<FNiagaraRibbonLink> RibbonLinks;

    // 🌀 Niagara VM интерфейс
    virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions) override;
    virtual void GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData, FVMExternalFunction& OutFunc) override;

    // 💡 Функция доступа к связям из Niagara
    void VMGetRibbonLink(FVectorVMExternalFunctionContextProxy& Context);
};