#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "../LocationSystem/LocationSpatialTypes.h"
#include "InteriorTransitionPayload.generated.h"

class UFloorAsset;
class UWorldRegionAsset;
class ALocationAnchorActor;
class UStreetAsset;
class UInteriorSetAsset;

/**
 * Descriptor — единая структура входных данных для Setup.
 * Заполняйте нужные поля (TargetFloor или TargetRegion и т.д.) — обработчик разберётся.
 */
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FInteriorTransitionDescriptor
{
	GENERATED_BODY()

	// ── Этаж источника (с которого уходим) ───────────────────────────────
	/** Ассет этажа, с которого выполняется переход. Используется для действий перед уходом. */
	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	TSoftObjectPtr<UFloorAsset> SourceFloor;

	// Иерархия назначения (любые из них — используйте те, что нужны)
	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	TSoftObjectPtr<UWorldRegionAsset> TargetRegion;

	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	TSoftObjectPtr<UStreetAsset> TargetStreet;

	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	TSoftObjectPtr<UInteriorSetAsset> TargetInteriorSet;

	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	TSoftObjectPtr<UFloorAsset> TargetFloor;

	// Идентификатор целевого якоря (GUID) — предпочтительный способ
	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	FGuid TargetAnchorID;

	// Альтернатива: TransitionPointId (если используется)
	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	FGuid TransitionPointId;

	// Альтернативы удобные для Blueprints
	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	int32 AnchorIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	FName AnchorName = NAME_None;
};

/**
 * Payload для переходов между картами (SeamlessTravel + позиционирование в якорь).
 * Хранит полную ссылку на точку назначения (FLocationAnchorLink).
 */
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UInteriorTransitionPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	// Полная ссылка на точку назначения (заполняется из дескриптора)
	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	FLocationAnchorLink DestinationLink;

	/** Ассет этажа, с которого выполняется переход (заполняется из дескриптора). */
	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	TSoftObjectPtr<UFloorAsset> SourceFloor;

	/** Простой Setup из уже собранной FLocationAnchorLink (совместимость) */
	UFUNCTION(BlueprintCallable, Category = "InteriorTransition")
	UInteriorTransitionPayload* Setup(const FLocationAnchorLink& InDestinationLink);

	/**
	 * Универсальный Setup: принимает дескриптор с любыми комбинациями данных.
	 * Копирует/преобразует поля в DestinationLink и пытается заполнить TargetAnchorDisplayName
	 * по данным ассетов (Floor/Region).
	 */
	UFUNCTION(BlueprintCallable, Category = "InteriorTransition")
	UInteriorTransitionPayload* SetupFromDescriptor(const FInteriorTransitionDescriptor& Descriptor);

	// ── Геттеры ───────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorTransition")
	FGuid GetTargetAnchorID() const { return DestinationLink.TargetAnchorID; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorTransition")
	FText GetTargetAnchorDisplayName() const { return DestinationLink.TargetAnchorDisplayName; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorTransition")
	bool IsValid() const { return DestinationLink.IsValid(); }

	/**
	 * Определяет путь к карте назначения (пакетный путь).
	 * Приоритет: FloorLevel > RegionLevel. Возвращает пустую строку если не найден.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorTransition")
	FString GetTargetLevelPackageName() const;
};