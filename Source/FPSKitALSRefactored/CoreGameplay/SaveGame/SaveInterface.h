#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "SaveInterface.generated.h"   
UINTERFACE(MinimalAPI)
class USaveInterface : public UInterface
{
    GENERATED_BODY()
};

class ISaveInterface
{
    GENERATED_BODY()

public:
    virtual FString SaveToJson() = 0;
    virtual void LoadFromJson(const FString& JsonString) = 0;
    virtual FString GetSaveID() const = 0;

    virtual void OnPreSave() {}
    virtual void OnPostLoad() {}
};