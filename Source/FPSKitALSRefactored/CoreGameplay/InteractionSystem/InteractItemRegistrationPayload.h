#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "InteractItemRegistrationPayload.generated.h"

// Which subsystem manages this interactive item
// (Какая подсистема управляет этим интерактивным объектом)
UENUM(BlueprintType)
enum class EInteractiveSubsystem : uint8
{
	Terminal    UMETA(DisplayName = "Terminal"),
	ActorNPC    UMETA(DisplayName = "Actor (NPC)"),
	Inventory   UMETA(DisplayName = "Inventory"),
	Interior    UMETA(DisplayName = "Interior")
};

// Payload sent when an interactive item spawns or despawns
// Carries all data the player needs - no direct component reference required
// (Payload при спавне/деспавне интерактивного объекта)
// (Несёт все данные которые нужны плейеру - прямая ссылка на компонент не требуется)
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UInteractItemRegistrationPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	// Unique item ID generated at BeginPlay by the component
	// (Уникальный ID объекта генерируется компонентом в BeginPlay)
	UPROPERTY(BlueprintReadWrite, Category = "InteractItem")
	FGuid ItemId;

	// Which subsystem handles this item
	// (Какая подсистема управляет этим объектом)
	// По умолчанию считаем Interior — большинство интерактивных объектов сейчас относятся к интерьеру
	// (Default is Interior — most interactive items are interior)
	UPROPERTY(BlueprintReadWrite, Category = "InteractItem")
	EInteractiveSubsystem SubsystemType = EInteractiveSubsystem::Interior;

	// Interaction range in cm - player checks distance against this value
	// (Дистанция интеракции в см - плейер сравнивает расстояние с этим значением)
	UPROPERTY(BlueprintReadWrite, Category = "InteractItem")
	float InteractionRange = 200.f;

	// Default tooltip text set in Editor
	// (Подсказка по умолчанию заданная в редакторе)
	UPROPERTY(BlueprintReadWrite, Category = "InteractItem")
	FText DefaultTooltip;

	// Actor owning this component (weak ref - do NOT store long-term)
	// Player uses it only during the same tick for distance check or highlight
	// (Актор-владелец компонента - слабая ссылка - НЕ хранить долгосрочно)
	// (Плейер использует только в тот же тик для проверки расстояния или хайлайта)
	UPROPERTY(BlueprintReadWrite, Category = "InteractItem")
	TWeakObjectPtr<AActor> OwnerActor;

	UFUNCTION(BlueprintCallable, Category = "InteractItem")
	UInteractItemRegistrationPayload* Setup(
		const FGuid&             InItemId,
		EInteractiveSubsystem    InSubsystemType,
		float                    InRange,
		const FText&             InDefaultTooltip,
		AActor*                  InOwnerActor)
	{
		ItemId           = InItemId;
		SubsystemType    = InSubsystemType;
		InteractionRange = InRange;
		DefaultTooltip   = InDefaultTooltip;
		OwnerActor       = InOwnerActor;
		return this;
	}

	// Getters
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractItem")
	FGuid GetItemId() const { return ItemId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractItem")
	EInteractiveSubsystem GetSubsystemType() const { return SubsystemType; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractItem")
	float GetInteractionRange() const { return InteractionRange; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractItem")
	FText GetDefaultTooltip() const { return DefaultTooltip; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractItem")
	AActor* GetOwnerActor() const { return OwnerActor.Get(); }
};