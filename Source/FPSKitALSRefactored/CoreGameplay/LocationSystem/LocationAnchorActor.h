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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Owner")
    ELocationContextType OwnerContextType = ELocationContextType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Owner",
        meta = (EditCondition = "OwnerContextType == ELocationContextType::Street"))
    TSoftObjectPtr<UWorldRegionAsset> OwnerRegion;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Owner",
        meta = (EditCondition = "OwnerContextType == ELocationContextType::Street"))
    TSoftObjectPtr<UStreetAsset> OwnerStreet;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Owner",
        meta = (EditCondition = "OwnerContextType == ELocationContextType::Street"))
    TSoftObjectPtr<UInteriorSetAsset> OwnerInteriorSet;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Owner",
        meta = (EditCondition = "OwnerContextType == ELocationContextType::Floor"))
    TSoftObjectPtr<UFloorAsset> OwnerFloor;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Destination")
    FLocationAnchorLink DestinationLink;

    // Runtime UFUNCTION — доступны во всех конфигурациях, реализованы вне #if WITH_EDITOR
    UFUNCTION(BlueprintCallable, Category = "Anchor|Transition")
    FLocationAnchorLink GetDestinationLink() const { return DestinationLink; }

    UFUNCTION(BlueprintCallable, Category = "Anchor|Transition")
    FInteriorTransitionDescriptor MakeTransitionDescriptor() const;

    UFUNCTION(BlueprintCallable, Category = "Anchor|Transition")
    UInteriorTransitionPayload* CreateTransitionPayload() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anchor|Components")
    USceneComponent* AnchorRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anchor|Components")
    UBillboardComponent* EditorSprite;

    // Editor-only UFUNCTION: CallInEditor — защищены #if WITH_EDITOR, UHT не генерирует exec в Shipping
#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Anchor|Editor")
    void SyncToAsset();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Anchor|Editor")
    void RefreshLinkDisplayName();
#endif

    virtual void PostActorCreated() override;
    virtual void PostLoad() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void PostEditMove(bool bFinished) override;
#endif

#if WITH_EDITOR
protected:
    bool TryAutoAssignOwnerFromWorld();
#endif

public:
    // Runtime UFUNCTION — доступна во всех конфигурациях
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Anchor|Editor")
    UObject* GetOwnerRegistrationAsset() const;
};