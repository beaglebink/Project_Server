#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "EditorServiceSubsystem.generated.h"

UCLASS()
class UEditorServiceSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    void OnNewActorsPlaced(UObject* Context, const TArray<AActor*>& NewActors);
};