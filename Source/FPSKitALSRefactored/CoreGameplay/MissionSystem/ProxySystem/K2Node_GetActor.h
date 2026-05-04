#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_GetActor.generated.h"

/**
 * Узел Blueprint для получения прокси-актора из сцены.
 */
UCLASS()
class FPSKITALSREFACTORED_API UK2Node_GetActor : public UK2Node
{
    GENERATED_BODY()

public:
    // Создание пинов
    virtual void AllocateDefaultPins() override;

    // Заголовок узла
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

    // Подсказка
    virtual FText GetTooltipText() const override;

    // Разворачивание узла в вызов функции ProxyManager
    virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;

#if WITH_EDITOR
    // Формирование выпадающего списка
    //virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
#endif
};
