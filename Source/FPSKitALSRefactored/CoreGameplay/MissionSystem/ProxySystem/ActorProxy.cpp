#include "ActorProxy.h"
#include "GameFramework/Actor.h"

void UActorProxy::SetProperty(FName PropertyName, const FString& Value)
{
    FProxyProperty Prop;
    Prop.Name = PropertyName;
    Prop.Value = Value;
    PendingPropertyValues.Add(Prop);
}

void UActorProxy::CallFunction(FName FunctionName, const TArray<FString>& Args)
{
    FProxyFunctionCall Call;
    Call.FunctionName = FunctionName;
    Call.Args = Args;
    PendingCalls.Add(Call);
}

void UActorProxy::ApplyToActor(AActor* Target)
{
    if (!Target) return;

    // Применяем свойства
    for (const FProxyProperty& Prop : PendingPropertyValues)
    {
        FProperty* Property = Target->GetClass()->FindPropertyByName(Prop.Name);
        if (Property)
        {
            void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Target);
            Property->ImportText(*Prop.Value, ValuePtr, PPF_None, Target);
        }
    }

    // Вызываем функции
    for (const FProxyFunctionCall& Call : PendingCalls)
    {
        UFunction* Func = Target->FindFunction(Call.FunctionName);
        if (Func)
        {
            // если нет параметров, передаём nullptr
            Target->ProcessEvent(Func, nullptr);
        }
    }

    PendingPropertyValues.Empty();
    PendingCalls.Empty();
}
