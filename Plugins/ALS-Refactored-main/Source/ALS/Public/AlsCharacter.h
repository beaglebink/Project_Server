#pragma once

#include "CoreMinimal.h"
#include "Delegates/DelegateCombinations.h"
#include "GameFramework/Character.h"
#include "State/AlsLocomotionState.h"
#include "State/AlsMantlingState.h"
#include "State/AlsMovementBaseState.h"
#include "State/AlsRagdollingState.h"
#include "State/AlsRollingState.h"
#include "State/AlsViewState.h"
#include "Utility/AlsGameplayTags.h"
#include "AlsCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStartMantling, float, AnimationDuration, EAlsMantlingType, MantlingType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStartRolling, float, AnimationDuration);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, Health, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, Stamina, float, MaxStamina);

struct FAlsMantlingParameters;
struct FAlsMantlingTraceSettings;
class UAlsCharacterMovementComponent;
class UAlsCharacterSettings;
class UAlsMovementSettings;
class UAlsAnimationInstance;
class UAlsMantlingSettings;
class UBlindnessWidget;

UCLASS(AutoExpandCategories = ("Settings|Als Character", "Settings|Als Character|Desired State", "State|Als Character"))
class ALS_API AAlsCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Als Character")
	TObjectPtr<UAlsCharacterMovementComponent> AlsCharacterMovement;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character")
	TObjectPtr<UAlsCharacterSettings> Settings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character")
	TObjectPtr<UAlsMovementSettings> MovementSettings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character", meta = (ClampMin = "0.5", ClampMax = "1.0"))
	float MovementBackwardSpeedMultiplier = 0.7f;

	float SpeedMultiplier = 1.0f;
	float PrevSpeedMultiplier = 1.0f;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float WeaponMovementPenalty = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character", meta = (ClampMin = "0.01", ClampMax = "5.0"))
	float DelayCrouchInOut = 0.01f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character|Desired State",
		ReplicatedUsing = "OnReplicated_DesiredAiming")
	uint8 bDesiredAiming : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character|Desired State", Replicated)
	FGameplayTag DesiredRotationMode{ AlsRotationModeTags::ViewDirection };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character|Desired State", Replicated)
	FGameplayTag DesiredStance{ AlsStanceTags::Standing };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character|Desired State", Replicated)
	FGameplayTag DesiredGait{ AlsGaitTags::Running };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character|Desired State", Replicated)
	FGameplayTag ViewMode{ AlsViewModeTags::ThirdPerson };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character|Desired State",
		ReplicatedUsing = "OnReplicated_OverlayMode")
	FGameplayTag OverlayMode{ AlsOverlayModeTags::Default };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Meta = (ShowInnerProperties))
	TWeakObjectPtr<UAlsAnimationInstance> AnimationInstance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient)
	FGameplayTag LocomotionMode{ AlsLocomotionModeTags::Grounded };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient)
	FGameplayTag RotationMode{ AlsRotationModeTags::ViewDirection };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient)
	FGameplayTag Stance{ AlsStanceTags::Standing };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient)
	FGameplayTag Gait{ AlsGaitTags::Walking };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient)
	FGameplayTag LocomotionAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient)
	FAlsMovementBaseState MovementBase;

	// Replicated raw view rotation. Depending on the context, this rotation can be in world space, or in movement
	// base space. In most cases, it is better to use FAlsViewState::Rotation to take advantage of network smoothing.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient,
		ReplicatedUsing = "OnReplicated_ReplicatedViewRotation")
	FRotator ReplicatedViewRotation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient)
	FAlsViewState ViewState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Replicated)
	FVector_NetQuantizeNormal InputDirection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character",
		Transient, Replicated, Meta = (ClampMin = -180, ClampMax = 180, ForceUnits = "deg"))
	float DesiredVelocityYawAngle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient)
	FAlsLocomotionState LocomotionState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient)
	FAlsMantlingState MantlingState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient, Replicated)
	FVector_NetQuantize RagdollTargetLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient)
	FAlsRagdollingState RagdollingState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State|Als Character", Transient)
	FAlsRollingState RollingState;

	FTimerHandle BrakingFrictionFactorResetTimer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool IsFirstJumpClick = true;


	UPROPERTY(EditAnywhere, BlueprintAssignable)
	FOnStartMantling OnStartmantling;

	UPROPERTY(EditAnywhere, BlueprintAssignable)
	FOnStartRolling OnStartRolling;

public:
	explicit AAlsCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* Property) const override;
#endif

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreRegisterAllComponents() override;

	virtual void PostRegisterAllComponents() override;

	virtual void PostInitializeComponents() override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void PostNetReceiveLocationAndRotation() override;

	virtual void OnRep_ReplicatedBasedMovement() override;

	virtual void Tick(float DeltaTime) override;

	virtual void PossessedBy(AController* NewController) override;

	virtual void Restart() override;

private:
	void RefreshMeshProperties() const;

	void RefreshMovementBase();

	// View Mode

public:
	const FGameplayTag& GetViewMode() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Character", Meta = (AutoCreateRefTerm = "NewViewMode"))
	void SetViewMode(const FGameplayTag& NewViewMode);

private:
	void SetViewMode(const FGameplayTag& NewViewMode, bool bSendRpc);

	UFUNCTION(Client, Reliable)
	void ClientSetViewMode(const FGameplayTag& NewViewMode);

	UFUNCTION(Server, Reliable)
	void ServerSetViewMode(const FGameplayTag& NewViewMode);

	// Locomotion Mode

public:
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode = 0) override;

public:
	const FGameplayTag& GetLocomotionMode() const;

protected:
	void SetLocomotionMode(const FGameplayTag& NewLocomotionMode);

	virtual void NotifyLocomotionModeChanged(const FGameplayTag& PreviousLocomotionMode);

	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	void OnLocomotionModeChanged(const FGameplayTag& PreviousLocomotionMode);

	// Desired Aiming

public:
	bool IsDesiredAiming() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Character")
	void SetDesiredAiming(bool bNewDesiredAiming);

private:
	void SetDesiredAiming(bool bNewDesiredAiming, bool bSendRpc);

	UFUNCTION(Client, Reliable)
	void ClientSetDesiredAiming(bool bNewDesiredAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetDesiredAiming(bool bNewDesiredAiming);

	UFUNCTION()
	void OnReplicated_DesiredAiming(bool bPreviousDesiredAiming);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	void OnDesiredAimingChanged(bool bPreviousDesiredAiming);

	UFUNCTION(BlueprintImplementableEvent, Category = "Aim input BP")
	void StartStopAim(bool bIsAim);

	// Desired Rotation Mode

public:
	const FGameplayTag& GetDesiredRotationMode() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Character", Meta = (AutoCreateRefTerm = "NewDesiredRotationMode"))
	void SetDesiredRotationMode(const FGameplayTag& NewDesiredRotationMode);

private:
	void SetDesiredRotationMode(const FGameplayTag& NewDesiredRotationMode, bool bSendRpc);

	UFUNCTION(Client, Reliable)
	void ClientSetDesiredRotationMode(const FGameplayTag& NewDesiredRotationMode);

	UFUNCTION(Server, Reliable)
	void ServerSetDesiredRotationMode(const FGameplayTag& NewDesiredRotationMode);

	// Rotation Mode

public:
	const FGameplayTag& GetRotationMode() const;

protected:
	void SetRotationMode(const FGameplayTag& NewRotationMode);

	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	void OnRotationModeChanged(const FGameplayTag& PreviousRotationMode);

	void RefreshRotationMode();

	// Desired Stance

public:
	const FGameplayTag& GetDesiredStance() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Character", Meta = (AutoCreateRefTerm = "NewDesiredStance"))
	void SetDesiredStance(const FGameplayTag& NewDesiredStance);

private:
	void SetDesiredStance(const FGameplayTag& NewDesiredStance, bool bSendRpc);

	UFUNCTION(Client, Reliable)
	void ClientSetDesiredStance(const FGameplayTag& NewDesiredStance);

	UFUNCTION(Server, Reliable)
	void ServerSetDesiredStance(const FGameplayTag& NewDesiredStance);

protected:
	virtual void ApplyDesiredStance();

	// Stance

public:
	virtual bool CanCrouch() const override;

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

public:
	const FGameplayTag& GetStance() const;

protected:
	void SetStance(const FGameplayTag& NewStance);

	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	void OnStanceChanged(const FGameplayTag& PreviousStance);

	// Desired Gait

public:
	const FGameplayTag& GetDesiredGait() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Character", Meta = (AutoCreateRefTerm = "NewDesiredGait"))
	void SetDesiredGait(const FGameplayTag& NewDesiredGait);

private:
	void SetDesiredGait(const FGameplayTag& NewDesiredGait, bool bSendRpc);

	UFUNCTION(Client, Reliable)
	void ClientSetDesiredGait(const FGameplayTag& NewDesiredGait);

	UFUNCTION(Server, Reliable)
	void ServerSetDesiredGait(const FGameplayTag& NewDesiredGait);

	// Gait

public:
	const FGameplayTag& GetGait() const;

protected:
	void SetGait(const FGameplayTag& NewGait);

	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	void OnGaitChanged(const FGameplayTag& PreviousGait);

	void CalculateBackwardAndStrafeMoveReducement();

private:
	void RefreshGait();

	FGameplayTag CalculateMaxAllowedGait() const;

	FGameplayTag CalculateActualGait(const FGameplayTag& MaxAllowedGait) const;

	bool CanSprint() const;

	// Overlay Mode

public:
	const FGameplayTag& GetOverlayMode() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Character", Meta = (AutoCreateRefTerm = "NewOverlayMode"))
	void SetOverlayMode(const FGameplayTag& NewOverlayMode);

private:
	void SetOverlayMode(const FGameplayTag& NewOverlayMode, bool bSendRpc);

	UFUNCTION(Client, Reliable)
	void ClientSetOverlayMode(const FGameplayTag& NewOverlayMode);

	UFUNCTION(Server, Reliable)
	void ServerSetOverlayMode(const FGameplayTag& NewOverlayMode);

	UFUNCTION()
	void OnReplicated_OverlayMode(const FGameplayTag& PreviousOverlayMode);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	void OnOverlayModeChanged(const FGameplayTag& PreviousOverlayMode);

	// Locomotion Action

public:
	const FGameplayTag& GetLocomotionAction() const;

	void SetLocomotionAction(const FGameplayTag& NewLocomotionAction);

protected:
	virtual void NotifyLocomotionActionChanged(const FGameplayTag& PreviousLocomotionAction);

	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	void OnLocomotionActionChanged(const FGameplayTag& PreviousLocomotionAction);

	// Input

public:
	const FVector& GetInputDirection() const;

protected:
	void SetInputDirection(FVector NewInputDirection);

	virtual void RefreshInput(float DeltaTime);

	// View

public:
	virtual FRotator GetViewRotation() const override;

private:
	void SetReplicatedViewRotation(const FRotator& NewViewRotation, bool bSendRpc);

	UFUNCTION(Server, Unreliable)
	void ServerSetReplicatedViewRotation(const FRotator& NewViewRotation);

	UFUNCTION()
	void OnReplicated_ReplicatedViewRotation();

public:
	void CorrectViewNetworkSmoothing(const FRotator& NewTargetRotation, bool bRelativeTargetRotation);

public:
	const FAlsViewState& GetViewState() const;

private:
	void RefreshView(float DeltaTime);

	void RefreshViewNetworkSmoothing(float DeltaTime);

	// Locomotion

public:
	const FAlsLocomotionState& GetLocomotionState() const;

private:
	void SetDesiredVelocityYawAngle(float NewDesiredVelocityYawAngle);

	void RefreshLocomotionLocationAndRotation();

	void RefreshLocomotionEarly();

	void RefreshLocomotion(float DeltaTime);

	void RefreshLocomotionLate(float DeltaTime);

	// Jumping

public:
	virtual void Jump() override;

	virtual void OnJumped_Implementation() override;

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnJumpedNetworked();

	void OnJumpedNetworked();

protected:
	void CalculateFallDistanceToCountStunAndDamage();

	// Rotation

public:
	virtual void FaceRotation(FRotator Rotation, float DeltaTime) override final;

	void CharacterMovement_OnPhysicsRotation(float DeltaTime);

private:
	void RefreshGroundedRotation(float DeltaTime);

protected:
	virtual bool RefreshCustomGroundedMovingRotation(float DeltaTime);

	virtual bool RefreshCustomGroundedNotMovingRotation(float DeltaTime);

	float CalculateGroundedMovingRotationInterpolationSpeed() const;

	void RefreshGroundedAimingRotation(float DeltaTime);

	bool RefreshConstrainedAimingRotation(float DeltaTime, bool bApplySecondaryConstraint = false);

private:
	void ApplyRotationYawSpeedAnimationCurve(float DeltaTime);

	void RefreshInAirRotation(float DeltaTime);

protected:
	virtual bool RefreshCustomInAirRotation(float DeltaTime);

	void RefreshInAirAimingRotation(float DeltaTime);

	void RefreshRotation(float TargetYawAngle, float DeltaTime, float RotationInterpolationSpeed);

	void RefreshRotationExtraSmooth(float TargetYawAngle, float DeltaTime,
		float RotationInterpolationSpeed, float TargetYawAngleRotationSpeed);

	void RefreshRotationInstant(float TargetYawAngle, ETeleportType Teleport = ETeleportType::None);

	void RefreshTargetYawAngleUsingLocomotionRotation();

	void RefreshTargetYawAngle(float TargetYawAngle);

	void RefreshViewRelativeTargetYawAngle();

	// Rolling

public:
	UFUNCTION(BlueprintCallable, Category = "ALS|Character")
	void StartRolling(float PlayRate = 1.0f);

	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	UAnimMontage* SelectRollMontage();

	bool IsRollingAllowedToStart(const UAnimMontage* Montage) const;

private:
	void StartRolling(float PlayRate, float TargetYawAngle);

	UFUNCTION(Server, Reliable)
	void ServerStartRolling(UAnimMontage* Montage, float PlayRate, float InitialYawAngle, float TargetYawAngle);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartRolling(UAnimMontage* Montage, float PlayRate, float InitialYawAngle, float TargetYawAngle);

	void StartRollingImplementation(UAnimMontage* Montage, float PlayRate, float InitialYawAngle, float TargetYawAngle);

	void RefreshRolling(float DeltaTime);

	void RefreshRollingPhysics(float DeltaTime);

	// Mantling

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	bool IsMantlingAllowedToStart() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Character", Meta = (ReturnDisplayName = "Success"))
	bool StartMantlingGrounded();

private:
	bool StartMantlingInAir();

	bool StartMantling(const FAlsMantlingTraceSettings& TraceSettings);

	UFUNCTION(Server, Reliable)
	void ServerStartMantling(const FAlsMantlingParameters& Parameters);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartMantling(const FAlsMantlingParameters& Parameters);

	void StartMantlingImplementation(const FAlsMantlingParameters& Parameters);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	UAlsMantlingSettings* SelectMantlingSettings(EAlsMantlingType MantlingType);

	float CalculateMantlingStartTime(const UAlsMantlingSettings* MantlingSettings, float MantlingHeight) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	void OnMantlingStarted(const FAlsMantlingParameters& Parameters);

private:
	void RefreshMantling();

	void StopMantling(bool bStopMontage = false);

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	void OnMantlingEnded();

	// Ragdolling

public:
	const FAlsRagdollingState& GetRagdollingState() const;

	bool IsRagdollingAllowedToStart() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Character")
	void StartRagdolling();

private:
	UFUNCTION(Server, Reliable)
	void ServerStartRagdolling();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartRagdolling();

	void StartRagdollingImplementation();

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	void OnRagdollingStarted();

public:
	bool IsRagdollingAllowedToStop() const;

	UFUNCTION(BlueprintCallable, Category = "ALS|Character", Meta = (ReturnDisplayName = "Success"))
	bool StopRagdolling();

private:
	UFUNCTION(Server, Reliable)
	void ServerStopRagdolling();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopRagdolling();

	void StopRagdollingImplementation();

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	UAnimMontage* SelectGetUpMontage(bool bRagdollFacingUpward);

	UFUNCTION(BlueprintNativeEvent, Category = "Als Character")
	void OnRagdollingEnded();

private:
	void SetRagdollTargetLocation(const FVector& NewTargetLocation);

	UFUNCTION(Server, Unreliable)
	void ServerSetRagdollTargetLocation(const FVector_NetQuantize& NewTargetLocation);

	void RefreshRagdolling(float DeltaTime);

	FVector RagdollTraceGround(bool& bGrounded) const;

	void LimitRagdollSpeed() const;

	// Debug

public:
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& Unused, float& VerticalLocation) override;

private:
	static void DisplayDebugHeader(const UCanvas* Canvas, const FText& HeaderText, const FLinearColor& HeaderColor,
		float Scale, float HorizontalLocation, float& VerticalLocation);

	void DisplayDebugCurves(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;

	void DisplayDebugState(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;

	void DisplayDebugShapes(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;

	void DisplayDebugTraces(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;

	void DisplayDebugMantling(const UCanvas* Canvas, float Scale, float HorizontalLocation, float& VerticalLocation) const;

	// Attributes
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0", ClampMax = "1000.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0", ClampMax = "1000.0"))
	float Health = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0", ClampMax = "1000.0"))
	float MaxStamina = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes", meta = (ClampMin = "0.0", ClampMax = "1000.0"))
	float Stamina = 100.0f;

public:
	UPROPERTY(EditAnywhere, BlueprintAssignable)
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(EditAnywhere, BlueprintAssignable)
	FOnStaminaChanged OnStaminaChanged;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attributes")
	float GetMaxHealth();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attributes")
	float GetHealth();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attributes")
	float GetMaxStamina();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attributes")
	float GetStamina();

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void SetMaxHealth(float NewMaxHealth);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void SetHealth(float NewHealth);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void SetMaxStamina(float NewMaxStamina);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void SetStamina(float NewStamina);

	// What does stamina affect
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attributes|Stamina affects", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float SprintStaminaDrainRate = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attributes|Stamina affects", meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float JumpStaminaCost = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attributes|Stamina affects", meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float RollStaminaCost = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attributes|Stamina affects", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float StaminaRegenerationRate = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Attributes|Stamina affects", meta = (ClampMin = "0.0", ClampMax = "10.0", Units = "s"))
	float	ExhaustionPenaltyDuration = 5.0f;

	uint8 AbleToSprint : 1{true};

	//Fall damage
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character|Effects|FallEffect", meta = (ClampMin = "0.0", ClampMax = "1000.0"))
	float MinFallHeightWithoutDamageAndStun = 200.0f;

private:
	float FallDistanceToCountStunAndDamage = 0.0f;
	float PrevZLocation = 0.0f;
	float ZLocation = 0.0f;
	float FallDamage = 0.0f;

	//Stun effect
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character|Effects|StunEffect", meta = (ClampMin = "0.0", ClampMax = "10.0", ToolTip = "Stun time from being heavy attack"))
	float StunTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character|Effects|StunEffect", meta = (ClampMin = "0.0", ClampMax = "10.0", ToolTip = "Stun recovery time"))
	float StunRecoveryTime = 5.0f;

	uint8 bIsStunned : 1 = false;

	UFUNCTION(BlueprintCallable, Category = "Stun effect")
	void StunEffect(float Time);

	float StunRecoveryMultiplier = 1.0f;

private:
	FTimerHandle StunTimerHandle;

private:
	void StunRecovery();

	//Damage slowdown
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character",
		meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "The bigger value - the bigger movement slowing in depends on health left"))
	float HealthMovementPenalty_01 = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character",
		meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "0 - no slow effect, 1 - max slow effect"))
	float DamageSlowdownEffect = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character",
		meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "How long slow effect lasts. 0 - no slow effect, 1 - max time slow effect"))
	float DamageSlowdownTime = 0.0f;

	float DamageSlowdownMultiplier = 1.0f;

private:
	void CalculateDamageSlowdownDuration(float NewHealth);

	//Adjusts speed based on incline or decline angles.
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character",
		meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "The influence of surface slope on the movement speed. 0 - no effect, 1 - max effect"))
	float SurfaceSlopeEffect = 0.0f;

private:
	float SurfaceSlopeEffectMultiplier = 1.0f;

	void CalculateSpeedMultiplierOnGoingUpOrDown();

	//Sliding
public:
	UPROPERTY(BlueprintReadWrite, Category = "Movement|Sliding")
	uint8 bIsSliding : 1{false};

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Sliding")
	float AlphaForLeanAnim;

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Sliding")
	FGameplayTag LastGaitTag;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character|Effects|SlidingEffect", meta = (ToolTip = "If true - turns on sliding logic in depends on surface physic friction"))
	uint8 SlidingTurnOnOff : 1 {false};

private:
	FVector2D PrevVelocity2D;
	FVector2D CurrentVelocity2D;
	FRotator PrevControlRotation;
	FRotator CurrentControlRotation;
	float PrevVelocityLength2D;
	float CurrentVelocityLength2D;
	FVector LastVelocity;
	FVector LastVelocityConsideringWind;
	float DeltaDistanceToGetToStopPoint;
	float SurfacePhysicFriction = 1.0f;
	float SlidingTime = 0.0f;

	bool SwitcherForSlidingLogic_OnSurfaceFriction();
	void CalculateStartStopSliding();

	//Wind influence
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character|Effects|WindEffect", meta = (ToolTip = "If true - turns on wind influence"))
	uint8 bDoesWindInfluence : 1 {false};

	UPROPERTY(BlueprintReadOnly, Category = "WindDirectionInfluence")
	float BackwardForward_WindAmount{ 0.0f };

	UPROPERTY(BlueprintReadOnly, Category = "WindDirectionInfluence")
	float LeftRight_WindAmount{ 0.0f };

	FVector2D WindDirectionAndSpeed;

private:
	float WindIfluenceEffect0_2{ 1.0f };

	void SetWindDirection();

	void CalculateWindInfluenceOnFalling();

	void CalculateWindInfluenceEffect();

	//Sticky surface
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character|Effects|SticknessEffect", meta = (ClampMin = 0.0f, ClampMax = 1.0f, ToolTip = "How faster slowdown on sticky surface"))
	float StickyStuckSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character|Effects|SticknessEffect", meta = (ClampMin = 0, ClampMax = 10, ToolTip = "How many times need to tap move button quickly and in a row to get rid of stickness"))
	uint8 HowManyTaps = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character|Effects|SticknessEffect", meta = (ClampMin = 0.0f, ClampMax = 1.0f, ForceUnits = "s", ToolTip = "How quick should push the button to escape"))
	float TimeBetweenTaps = 0.2f;

	UPROPERTY(BlueprintReadWrite, Category = "SticknessEffect")
	float StickyStuckMultiplier{ 1.0f };

	UFUNCTION(BlueprintCallable, Category = "SticknessEffect")
	bool IsStickySurface(FName Bone);

protected:
	uint8 bUsedMashToEscape : 1{false};

	uint8 bIsSticky : 1{false};

	uint8 bIsStickyStuck : 1{false};

	void RemoveSticknessByMash();

	FVector2D LastInputDirection;

private:
	float StickyMultiplier{ 1.0f };

	uint8 bTapInTime = false;

	uint8 TapCounter = 0;

	FTimerHandle TapInTimeTimerHandle;

	FVector2D PrevInputDirection;

	//stumble effect
public:
	UFUNCTION(BlueprintCallable, Category = "StumbleEffect")
	void StumbleEffect(FVector InstigatorLocation, float InstigatorPower);

	//knockdown effect
public:
	UFUNCTION(BlueprintCallable, Category = "KnockdownEffect")
	void KnockdownEffect(FVector InstigatorLocation, float InfluenceRadius);

	//shock effect
public:
	UPROPERTY(BlueprintReadWrite, Category = "ShockEffect")
	uint8 bIsShocked : 1{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character|Effects|ShockEffect", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float ShockEffectPower_01Range = 0.0f;

private:
	FTimerHandle LaunchTimerHandle;
	FTimerHandle CameraTimerHandle;
	FTimerHandle DiscreteTimerHandle;
	FTimerHandle RapidTimerHandle;

	float ShockSpeedMultiplier = 1.0f;
	float CameraPitchOffset = 0.0f;
	float CameraYawOffset = 0.0f;
	float CameraRapidPitchOffset = 0.0f;
	float CameraRapidYawOffset = 0.0f;
	float RapidFinalDistance = 0.0f;
	float RapidFinalDistanceTransition = 0.0f;

	void ShockEffect();

	//slowdown
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character|Effects|SlowdownEffect", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float Slowdown_01Range = 1.0f;

	//discombobulate Effect
public:
	UPROPERTY(BlueprintReadWrite, Category = "Discombobulate Effect")
	uint8 bIsDiscombobulated : 1{false};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character|Effects|DiscombobulateEffect", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float DiscombobulateEffectPower_01Range = 0.0f;

protected:
	float DiscombobulateTimeDelay = 0.0001f;

private:
	float CurrentDiscombobulateCameraPitchOffset = 0.0f;
	float CurrentDiscombobulateCameraYawOffset = 0.0f;

	float TargetDiscombobulateCameraPitchOffset = 0.0f;
	float TargetDiscombobulateCameraYawOffset = 0.0f;

	void DiscombobulateEffect();

	//blind effect
public:
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<UBlindnessWidget> BlindnessWidgetClass;

private:
	UBlindnessWidget* BlindnessWidget;

public:
	UFUNCTION(BlueprintCallable, Category = "BlindnessEffect")
	void SetRemoveBlindness(bool IsSet);

	//reversed input
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Settings|Als Character|Effects|ReverseEffect")
	uint8 bIsInputReversed : 1{false};

	//wire effect
public:
	UFUNCTION(BlueprintCallable, Category = "WireEffect")
	void SetRemoveWireEffect(bool bIsSet, UPARAM(meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0")) float EffectPower = 0.0f);

protected:
	uint8 bIsWired : 1{false};
	float WireEffectPower_01Range = 1.0f;

	void ShakeMouseRemoveEffect(FVector2D Value);

private:
	float PrevPrevMouseValueLength = 0.0f;
	float PrevMouseValueLength = 0.0f;
	float CurrentMouseValueLength = 0.0f;

	//grappling effect
protected:
	uint8 bIsGrappled : 1{false};
	float GrappleEffectSpeedMultiplier = 1.0f;

public:
	UFUNCTION(BlueprintCallable, Category = "WireEffect")
	void SetRemoveGrappleEffect(bool bIsSet);

protected:
	void PressTwoKeysRemoveGrappleEffect(bool bIsHold);
	uint8 bIsTwoKeysHold : 1{false};

	//magnetic effect
private:
	uint8 bIsMagnetic : 1 {false};

	FVector MagnetLocation;

	float MagneticEffectSpeedMultiplier = 1.0f;

	float MagneticEffectPower_Range01 = 1.0f;

	float MagneticSphereRadius = 0.0f;

	void MagneticEffect();

public:
	UFUNCTION(BlueprintCallable, Category = "Magnetic Effect")
	void SetRemoveMagneticEffect(bool bIsSet, float SphereRadius, float MagnetPower = 1.0f, FVector ActorLocation = FVector::ZeroVector);

	//ink effect
public:
	UPROPERTY(BlueprintReadOnly, Category = "Ink Effect")
	FRotator WeaponRotation_InkEffect;

protected:
	uint8 bIsInked : 1{false};

	uint8 bIsInkProcessed : 1{false};

	float InkEffectPower_01Range = 0.0f;

	float InkTimeDelay = 0.0001f;

private:
	float PrevLookSpeed = 0.0f;
	float CurrentLookSpeed = 0.0f;

	FRotator DeltaControlRotatiton;
	FRotator PrevControlRotation_Ink;
	FRotator CurrentControlRotation_Ink;

	FVector2D PrevLookDirection;
	FVector2D CurrentLookDirection;

public:
	UFUNCTION(BlueprintCallable, Category = "Ink effect")
	void SetRemoveInkEffect(bool bIsSet, UPARAM(meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0")) float EffectPower = 0.0f);

private:
	void CalculateInkEffect();

	// object virus effect
private:
	uint8 bIsRestored : 1{false};

	float CurrentDeltaSpeed = 0.0f;
	float CurrentDeltaJumpHeight = 0.0f;
	float CurrentDeltaHealth = 0.0;
	float CurrentDeltaStamina = 0.0;

	void Restore_Speed_JumpHeight_Health();

public:
	UFUNCTION(BlueprintCallable, Category = "Object Virus Effect")
	void Alter_Speed_JumpHeight_Health_Stamina(float DeltaSpeed, float DeltaJumpHeight, float DeltaHealth, float DeltaStamina, float TimeToRestore = 0.0f);

	//bubble effect
public:
	UPROPERTY(BlueprintReadWrite, Category = "Bubble effect")
	uint8 bIsBubbled : 1 {false};
};

inline const FGameplayTag& AAlsCharacter::GetViewMode() const
{
	return ViewMode;
}

inline const FGameplayTag& AAlsCharacter::GetLocomotionMode() const
{
	return LocomotionMode;
}

inline bool AAlsCharacter::IsDesiredAiming() const
{
	return bDesiredAiming;
}

inline const FGameplayTag& AAlsCharacter::GetDesiredRotationMode() const
{
	return DesiredRotationMode;
}

inline const FGameplayTag& AAlsCharacter::GetRotationMode() const
{
	return RotationMode;
}

inline const FGameplayTag& AAlsCharacter::GetDesiredStance() const
{
	return DesiredStance;
}

inline const FGameplayTag& AAlsCharacter::GetStance() const
{
	return Stance;
}

inline const FGameplayTag& AAlsCharacter::GetDesiredGait() const
{
	return DesiredGait;
}

inline const FGameplayTag& AAlsCharacter::GetGait() const
{
	return Gait;
}

inline const FGameplayTag& AAlsCharacter::GetOverlayMode() const
{
	return OverlayMode;
}

inline const FGameplayTag& AAlsCharacter::GetLocomotionAction() const
{
	return LocomotionAction;
}

inline const FVector& AAlsCharacter::GetInputDirection() const
{
	return InputDirection;
}

inline const FAlsViewState& AAlsCharacter::GetViewState() const
{
	return ViewState;
}

inline const FAlsLocomotionState& AAlsCharacter::GetLocomotionState() const
{
	return LocomotionState;
}

inline const FAlsRagdollingState& AAlsCharacter::GetRagdollingState() const
{
	return RagdollingState;
}

