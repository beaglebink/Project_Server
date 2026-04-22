// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LocationSpatialTypes.h"
#include "../InteriorInstanceSystem/InteriorTransitionPayload.h"
#include "LocationAnchorActor.generated.h"

class USceneComponent;
class UBillboardComponent;
class UWorldRegionAsset;
class UStreetAsset;
class UInteriorSetAsset;
class UFloorAsset;

/**
 * Актор-якорь, размещаемый на карте.
 * Хранит ссылку на свой FLocationAnchor (по AnchorID) в ассете
 * и FLocationAnchorLink — куда ведёт этот переход.
 */
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API ALocationAnchorActor : public AActor
{
    GENERATED_BODY()

public:
    ALocationAnchorActor();

    // Идентификация
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Identity")
    FGuid AnchorID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Classification")
    ELocationTransitionType TransitionType = ELocationTransitionType::Door;

    // Владелец (контекст)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Owner")
    ELocationContextType OwnerContextType = ELocationContextType::None;

    // Регион (район) — для карты района/фоллов
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Owner",
        meta = (EditCondition = "OwnerContextType == ELocationContextType::Street"))
    TSoftObjectPtr<UWorldRegionAsset> OwnerRegion;

    // Улица — для карты района: обязательно указывать при регистрации
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Owner",
        meta = (EditCondition = "OwnerContextType == ELocationContextType::Street"))
    TSoftObjectPtr<UStreetAsset> OwnerStreet;

    // Здание (InteriorSet) — если задано, Registration идёт в InteriorSet
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Owner",
        meta = (EditCondition = "OwnerContextType == ELocationContextType::Street"))
    TSoftObjectPtr<UInteriorSetAsset> OwnerInteriorSet;

    // Этаж — если актор на карте этажа
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Owner",
        meta = (EditCondition = "OwnerContextType == ELocationContextType::Floor"))
    TSoftObjectPtr<UFloorAsset> OwnerFloor;

    // Куда ведёт этот якорь
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Destination")
    FLocationAnchorLink DestinationLink;

    // Возвращает готовую ссылку назначения (Region/Street/InteriorSet/Floor + TargetAnchorID/DisplayName)
    UFUNCTION(BlueprintCallable, Category = "Anchor|Transition")
    FLocationAnchorLink GetDestinationLink() const { return DestinationLink; }

    // Возвращает дескриптор перехода, сформированный из полей DestinationLink.
    UFUNCTION(BlueprintCallable, Category = "Anchor|Transition")
    FInteriorTransitionDescriptor MakeTransitionDescriptor() const;

    // Создаёт новый UInteriorTransitionPayload и заполняет его данными из этого актора (удобно для Blueprint)
    UFUNCTION(BlueprintCallable, Category = "Anchor|Transition")
    UInteriorTransitionPayload* CreateTransitionPayload() const;

    // Компоненты
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anchor|Components")
    USceneComponent* AnchorRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anchor|Components")
    UBillboardComponent* EditorSprite;

    // Editor / utilities
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Anchor|Editor")
    void SyncToAsset();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Anchor|Editor")
    void RefreshLinkDisplayName();

    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

    // Важные жизненные методы объявляем всегда (не внутри #if WITH_EDITOR),
    // чтобы сигнатуры были одинаковы во всех TU и не было LNK ошибок.
    virtual void PostActorCreated() override;
    virtual void PostLoad() override;
    virtual void PostEditMove(bool bFinished) override;

#if WITH_EDITOR
protected:
    // Editor-only helper: пытается автоматически назначить OwnerRegion/OwnerFloor
    bool TryAutoAssignOwnerFromWorld();
#endif

public:
    // Возвращает ассет (Floor / InteriorSet / Street/Region) куда регистрировать TransitionPoint
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Anchor|Editor")
    UObject* GetOwnerRegistrationAsset() const;
};