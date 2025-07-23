#include "NiagaraDataInterface_RibbonLinks.h"
#include "NiagaraTypes.h"
#include "VectorVM.h"
#include "NiagaraSystemInstance.h"

UNiagaraDataInterface_RibbonLinks::UNiagaraDataInterface_RibbonLinks()
{
}

void UNiagaraDataInterface_RibbonLinks::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
    FNiagaraFunctionSignature Sig;
    Sig.Name = TEXT("GetRibbonLink");
    Sig.bMemberFunction = true;
    Sig.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Index")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Start")));
    Sig.Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("End")));
    Sig.SetDescription(FText::FromString(TEXT("Returns Start and End indices of RibbonLinks[Index].")));
    OutFunctions.Add(Sig);
}

void UNiagaraDataInterface_RibbonLinks::GetVMExternalFunction(
    const FVMExternalFunctionBindingInfo& BindingInfo,
    void* InstanceData,
    FVMExternalFunction& OutFunc)
{
    if (BindingInfo.Name == TEXT("GetRibbonLink"))
    {
        OutFunc = FVMExternalFunction::CreateUObject(this, &UNiagaraDataInterface_RibbonLinks::VMGetRibbonLink);
    }
}

void UNiagaraDataInterface_RibbonLinks::VMGetRibbonLink(FVectorVMExternalFunctionContextProxy& Context)
{
    const int32 NumInstances = Context.GetNumInstances();
    VectorVM::FExternalFuncInputHandler<int32> IndexParam(Context);
    VectorVM::FExternalFuncRegisterHandler<int32> OutStart(Context);
    VectorVM::FExternalFuncRegisterHandler<int32> OutEnd(Context);

    for (int32 i = 0; i < Context.GetNumInstances(); ++i)
    {
        int32 Index = IndexParam.GetAndAdvance();
        const FNiagaraRibbonLink& Link = RibbonLinks.IsValidIndex(Index) ? RibbonLinks[Index] : FNiagaraRibbonLink{ 0, 0 };

        *OutStart.GetDest() = Link.StartIndex;
        *OutEnd.GetDest() = Link.EndIndex;

        OutStart.Advance();
        OutEnd.Advance();
    }
}