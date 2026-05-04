#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_GetActorByClass.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UK2Node_GetActorByClass : public UK2Node
{
    GENERATED_BODY()

public:
    virtual void AllocateDefaultPins() override;
    virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
    virtual FText GetTooltipText() const override;
    virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
    virtual bool IsNodePure() const override { return false; }
    virtual FText GetMenuCategory() const override;

    // Вызывается при изменении пинов
    virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;

    // Обновляет тип выходного пина в зависимости от ActorClass
    void UpdateOutputPinType();

private:
    TSubclassOf<AActor> GetActorClassFromPin() const;
};