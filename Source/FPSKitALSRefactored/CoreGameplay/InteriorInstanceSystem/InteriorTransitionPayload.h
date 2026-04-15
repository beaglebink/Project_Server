#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "../LocationSystem/FloorAsset.h"
#include "InteriorTransitionPayload.generated.h"

// Payload для событий переходов интерьера (SeamlessTravel + позиционирование в якорь)
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UInteriorTransitionPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	// Целевой этаж — передаётся напрямую как ассет
	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	TObjectPtr<UFloorAsset> TargetFloor = nullptr;

	// ID точки перехода (FLocationTransitionPoint::TransitionPointID) — используется при наличии
	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	FGuid TransitionPointId;

	// Альтернативный способ (удобен в Blueprint): индекс якоря в массиве Anchors целевого FloorAsset
	// Используется если TransitionPointId не задан
	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	int32 AnchorIndex = -1;

	// Альтернативный способ (удобен в Blueprint): имя/тег якоря (сравнивается с DisplayName или с GameplayTag)
	// Пример: "NorthEntry" — в Blueprint можно задать как Name
	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	FName AnchorName = NAME_None;

	// Основной Setup: ассет + опциональный TransitionPointId
	UFUNCTION(BlueprintCallable, Category = "InteriorTransition")
	UInteriorTransitionPayload* SetupWithTransitionPoint(
		UFloorAsset*  InTargetFloor,
		const FGuid&  InTransitionPointId = FGuid())
	{
		TargetFloor       = InTargetFloor;
		TransitionPointId = InTransitionPointId;
		AnchorIndex       = -1;
		AnchorName        = NAME_None;
		return this;
	}

	// Setup по индексу якоря
	UFUNCTION(BlueprintCallable, Category = "InteriorTransition")
	UInteriorTransitionPayload* SetupWithAnchorIndex(UFloorAsset* InTargetFloor, int32 InAnchorIndex)
	{
		TargetFloor       = InTargetFloor;
		TransitionPointId.Invalidate();
		AnchorIndex       = InAnchorIndex;
		AnchorName        = NAME_None;
		return this;
	}

	// Setup по имени/тегу якоря
	UFUNCTION(BlueprintCallable, Category = "InteriorTransition")
	UInteriorTransitionPayload* SetupWithAnchorName(UFloorAsset* InTargetFloor, FName InAnchorName)
	{
		TargetFloor       = InTargetFloor;
		TransitionPointId.Invalidate();
		AnchorIndex       = -1;
		AnchorName        = InAnchorName;
		return this;
	}

	// Getters
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorTransition")
	UFloorAsset* GetTargetFloor() const { return TargetFloor; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorTransition")
	FGuid GetTransitionPointId() const { return TransitionPointId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorTransition")
	int32 GetAnchorIndex() const { return AnchorIndex; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorTransition")
	FName GetAnchorName() const { return AnchorName; }
};