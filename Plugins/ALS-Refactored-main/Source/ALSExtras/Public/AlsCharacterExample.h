#pragma once

#include "AlsCharacter.h"
#include "Enums/EnumLoopStates.h"
#include <PhysicsEngine/PhysicsConstraintActor.h>
#include "AlsCharacterExample_I.h"
#include "Interfaces/I_PortalInteraction.h"
#include "Utility/AlsGameplayTags.h"
#include "AlsCharacterExample.generated.h"

struct FInputActionValue;
class UAlsCameraComponent;
class UInputMappingContext;
class UInputAction;
class UAttributesWidget;
class UAC_Inventory;
class USceneCaptureComponent2D;

UENUM(BlueprintType)
enum class EMovementDirection : uint8
{
	Forward_Back,
	Right_Left
};

struct FEffectTimer
{
	FTimerHandle EffectTimerHandle;
	FTimerDelegate EffectDelegate;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMovementInputEvent, EMovementDirection, MovementDirection, float, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetParalyse, AActor*, NetReason);

UCLASS(AutoExpandCategories = ("Settings|Als Character Example", "State|Als Character Example"))
class ALSEXTRAS_API AAlsCharacterExample : public AAlsCharacter, public IAlsCharacter_I, public II_PortalInteraction
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Als Character Example")
	TObjectPtr<UAlsCameraComponent> Camera;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Als Character Example")
	USceneCaptureComponent2D* SceneCaptureComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	UAC_Inventory* InventoryComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintAssignable)
	FOnNetParalyse OnNetParalyse;
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LookMouseAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> WalkAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> RagdollAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> RollAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> RotationModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> ViewModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> SwitchShoulderAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> SwitchWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> RemoveSticknessAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> ThrowAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> MeleeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> FireModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> NVGsAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LeanAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> ScoreBoardAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character Example", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> GrappleRemoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character Example", Meta = (ClampMin = 0, ForceUnits = "x"))
	float LookUpMouseSensitivity{ 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character Example", Meta = (ClampMin = 0, ForceUnits = "x"))
	float LookRightMouseSensitivity{ 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character Example",
		Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float LookUpRate{ 90.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character Example",
		Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float LookRightRate{ 240.0f };

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interact|DragActor")
	//UPhysicsConstraintComponent* PhysicsConstraint;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|DragActor")
	//UStaticMeshComponent* AttachmentPoint;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Movement|Input")
	FOnMovementInputEvent OnMovementInputEvent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character Example")
	float DoubleSpaceTime = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character Example")
	bool IsImplementingAIM = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character Example")
	float NetGrenadeParalyseTime = 5.0f;

private:
	FTimerHandle JumpTimerHandle;

	UObject* Target = nullptr;

	AActor* FocusActor = nullptr;

	AController* TestController = nullptr;

	AActor* ReasonParalyse = nullptr;

public:
	AAlsCharacterExample();

	void BeginPlay() override;

	virtual void NotifyControllerChanged() override;

	virtual float GetNetGrenadeParalyseTime() const;

	virtual void ParalyzeNPC(AActor* Reason, float Time);

	void EndStun();

	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& ViewInfo) override;

	// Input

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;

private:
	void Input_OnLookMouse(const FInputActionValue& ActionValue);

	void Input_OnLook(const FInputActionValue& ActionValue);

	void Input_OnMove(const FInputActionValue& ActionValue);
	void Input_OnMove_Released();

	void Input_OnSprint(const FInputActionValue& ActionValue);

	void Input_OnWalk();

	void Input_OnCrouch();

	void Input_OnJump(const FInputActionValue& ActionValue);

	void Input_OnAim(const FInputActionValue& ActionValue);

	void Input_OnRagdoll();

	void Input_OnRoll();

	void Input_OnRotationMode();

	void Input_OnViewMode();

	void Input_OnSwitchShoulder();

	void ContinueJump();

	void Input_OnSwitchWeapon();

	void Input_OnRemoveStickness();

	void Input_OnRemoveGrapple(const FInputActionValue& ActionValue);

protected:
	UFUNCTION(BlueprintImplementableEvent)
	void SwitchWeaponHandle();

	// Debug

public:
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& Unused, float& VerticalLocation) override;

	UFUNCTION(BlueprintCallable, Category = "Interact|DragActor")
	void GrabExistingObject(AActor* ExistingActor);

	UFUNCTION(BlueprintCallable, Category = "Interact|DragActor")
	void ReleaseObject();

	void Tick(float DeltaTime) override;

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	// UI
protected:
	UPROPERTY(BlueprintReadWrite, Category = "Widgets")
	TObjectPtr<UAttributesWidget> AttributesWidget;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OnSetSprintMode(bool bSprintMode);

	//loop effect
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character|Effects|LoopEffect", meta = (ClampMin = 1, ClampMax = 10))
	uint8 HowManyLoops = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character|Effects|LoopEffect", meta = (ClampMin = 0.0f, ClampMax = 15.0f))
	float RepeatingPeaceDuration = 0.0f;

	UPROPERTY(BlueprintReadWrite, Category = "LoopEffect")
	uint8 bIsLooped : 1 = false;

	FLoopEffectFrame LoopEffectFrame;

	UFUNCTION(BlueprintCallable, Category = "LoopEffect")
	void SetLoopEffect(bool bIsSet);

private:
	TDoubleLinkedList<FLoopEffectFrame> FrameList;
	TDoubleLinkedList<FLoopEffectFrame>::TIterator FrameIt = TDoubleLinkedList<FLoopEffectFrame>::TIterator(FrameList.GetHead());

	uint8 FrameListSize = 0;

	uint8 LoopsCounter = 0;

	void LoopEffect();

	//Inventory
protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UAC_Inventory> Inventory;

public:
	UFUNCTION(BlueprintCallable, Category = "SceneRender")
	void SetSceneRenderComponents(AActor* Actor);

	//Portal interaction
private:
	virtual void PortalInteract_Implementation(const FHitResult& Hit, const FTransform& EnterTransform, const FTransform& ExitTransform) override;

	//Food effects
public:
	UFUNCTION(BlueprintCallable, Category = "FoodEffects")
	void FoodEffectByTag(const FGameplayTag& Tag, bool Apply);

private:
	TArray<FEffectTimer> EffectTimerHandles;

	void InitializeFoodEffectTimerDelegates();

	TMap<FGameplayTag, TFunction<void(bool)>> FoodEffectMap;

	void InitializeFoodEffectMap();

	void SetEffect_1(bool Apply = false);
	void SetEffect_2(bool Apply = false);
	void SetEffect_3(bool Apply = false);
	void SetEffect_4(bool Apply = false);
	void SetEffect_5(bool Apply = false);
	void SetEffect_6(bool Apply = false);
	void SetEffect_7(bool Apply = false);
	void SetEffect_8(bool Apply = false);
	void SetEffect_9(bool Apply = false);
	void SetEffect_10(bool Apply = false);
	void SetEffect_11(bool Apply = false);
	void SetEffect_12(bool Apply = false);
	void SetEffect_13(bool Apply = false);
	void SetEffect_14(bool Apply = false);
	void SetEffect_15(bool Apply = false);
	void SetEffect_16(bool Apply = false);
	void SetEffect_17(bool Apply = false);
	void SetEffect_18(bool Apply = false);
	void SetEffect_19(bool Apply = false);
	void SetEffect_20(bool Apply = false);
	void SetEffect_21(bool Apply = false);
	void SetEffect_22(bool Apply = false);
	void SetEffect_23(bool Apply = false);
	void SetEffect_24(bool Apply = false);
	void SetEffect_25(bool Apply = false);
	void SetEffect_26(bool Apply = false);
	void SetEffect_27(bool Apply = false);
	void SetEffect_28(bool Apply = false);
	void SetEffect_29(bool Apply = false);
	void SetEffect_30(bool Apply = false);
	void SetEffect_31(bool Apply = false);
	void SetEffect_32(bool Apply = false);
	void SetEffect_33(bool Apply = false);
	void SetEffect_34(bool Apply = false);
	void SetEffect_35(bool Apply = false);
	void SetEffect_36(bool Apply = false);
	void SetEffect_37(bool Apply = false);
	void SetEffect_38(bool Apply = false);
	void SetEffect_39(bool Apply = false);
	void SetEffect_40(bool Apply = false);
	void SetEffect_41(bool Apply = false);
	void SetEffect_42(bool Apply = false);
	void SetEffect_43(bool Apply = false);
	void SetEffect_44(bool Apply = false);
	void SetEffect_45(bool Apply = false);
	void SetEffect_46(bool Apply = false);
	void SetEffect_47(bool Apply = false);
	void SetEffect_48(bool Apply = false);
	void SetEffect_49(bool Apply = false);
	void SetEffect_50(bool Apply = false);
	void SetEffect_51(bool Apply = false);
	void SetEffect_52(bool Apply = false);
	void SetEffect_53(bool Apply = false);
	void SetEffect_54(bool Apply = false);
	void SetEffect_55(bool Apply = false);

	//Clothes effects
public:
	UFUNCTION(BlueprintCallable, Category = "ClothesEffects")
	void ClothesEffectByTag(const FGameplayTag& Tag, bool Apply);

private:
	TMap<FGameplayTag, TFunction<void(bool)>> ClothesEffectMap;

	void InitializeClothesEffectMap();

	void AlphabetCoat_Effect(bool Apply = false);
	void ByteVest_Effect(bool Apply = false);
	void JanitorOveralls_Effect(bool Apply = false);
	void WasherOveralls_Effect(bool Apply = false);
	void CalculatorGoggles_Effect(bool Apply = false);
	void GladiatorOutfit_Effect(bool Apply = false);
	void PorcupineCoat_Effect(bool Apply = false);
	virtual void ForcefieldCoat_Effect(bool Apply = false) override;
	virtual void BugZapperCoat_Effect(bool Apply = false) override;
	void SimmonSweatpants_Effect(bool Apply = false);
	virtual void RebootVest_Effect(bool Apply = false) override;
	void NullAndVoidHat_Effect(bool Apply = false);
	void MagneticVest_Effect(bool Apply = false);
	void AmmoBeltVest_Effect(bool Apply = false);
	void MasterMinMooMooSlippers_Effect(bool Apply = false);
	void BounceHouseSuit_Effect(bool Apply = false);
	void SheriffOutfit_Effect(bool Apply = false);
	void ManillaOxfordAndSlacks_Effect(bool Apply = false);
	void HoareSweaterVest_Effect(bool Apply = false);
	void NuttySpectacles_Effect(bool Apply = false);
	void BugHunterUniform_Effect(bool Apply = false);
	void SniperFocusOnLongRange_Effect(bool Apply = false);
	void Boxer_Effect(bool Apply = false);
	void AdminPolo_Effect(bool Apply = false);
	void Effect_25(bool Apply = false);
	void CarlOvercoat_Effect(bool Apply = false);
	void Effect_27(bool Apply = false);
	void SimonSweater_Effect(bool Apply = false);
	void Effect_29(bool Apply = false);
	void LaoEddieNightRobe_Effect(bool Apply = false);
	void Effect_31(bool Apply = false);
	void Effect_32(bool Apply = false);
	void Effect_33(bool Apply = false);
	void Effect_34(bool Apply = false);
	void DesperadoPoncho_Effect(bool Apply = false);
	void Effect_36(bool Apply = false);
	void WW2Uniform_Effect(bool Apply = false);
	void GreenhouseOutfit_Effect(bool Apply = false);
	void HeartShapedSweater_Effect(bool Apply = false);
	void CableRepairOutfit_Effect(bool Apply = false);
	void Effect_41(bool Apply = false);
	void TroubleshooterJacket_Effect(bool Apply = false);
	void HotSwapPatch_Effect(bool Apply = false);
	void ChefApron_Effect(bool Apply = false);
	void MiddleAgedCyborgSamuraiTortoiseShell_Effect(bool Apply = false);
	void RastaRobe_Effect(bool Apply = false);
	void UndertakerCloak_Effect(bool Apply = false);
	void TranquilBlouse_Effect(bool Apply = false);
	void Effect_49(bool Apply = false);
	void KnuthOvercoat_Effect(bool Apply = false);
	void VcarSweatShirt_Effect(bool Apply = false);
	void Effect_52(bool Apply = false);
};
