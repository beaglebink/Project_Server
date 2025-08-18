#include "AlsCharacter.h"

#include "AlsAnimationInstance.h"
#include "AlsCharacterMovementComponent.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/GameNetworkManager.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Settings/AlsCharacterSettings.h"
#include "Utility/AlsConstants.h"
#include "Utility/AlsMacros.h"
#include "Utility/AlsUtility.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/WindDirectionalSource.h"
#include "Components/WindDirectionalSourceComponent.h"
#include "UI/BlindnessWidget.h"
#include "Animation/WidgetAnimation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AlsCharacter)

namespace AlsCharacterConstants
{
	constexpr auto TeleportDistanceThresholdSquared{ FMath::Square(50.0f) };
}

AAlsCharacter::AAlsCharacter(const FObjectInitializer& ObjectInitializer) : Super{
	ObjectInitializer.SetDefaultSubobjectClass<UAlsCharacterMovementComponent>(CharacterMovementComponentName)
}
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	bClientCheckEncroachmentOnNetUpdate = true; // Required for bSimGravityDisabled to be updated.

	GetCapsuleComponent()->InitCapsuleSize(30.0f, 90.0f);

	if (IsValid(GetMesh()))
	{
		GetMesh()->SetRelativeLocation_Direct({ 0.0f, 0.0f, -92.0f });
		GetMesh()->SetRelativeRotation_Direct({ 0.0f, -90.0f, 0.0f });

		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
		GetMesh()->bEnableUpdateRateOptimizations = false;
	}

	AlsCharacterMovement = Cast<UAlsCharacterMovementComponent>(GetCharacterMovement());

	// This will prevent the editor from combining component details with actor details.
	// Component details can still be accessed from the actor's component hierarchy.

#if WITH_EDITOR
	StaticClass()->FindPropertyByName(FName{ TEXTVIEW("Mesh") })->SetPropertyFlags(CPF_DisableEditOnInstance);
	StaticClass()->FindPropertyByName(FName{ TEXTVIEW("CapsuleComponent") })->SetPropertyFlags(CPF_DisableEditOnInstance);
	StaticClass()->FindPropertyByName(FName{ TEXTVIEW("CharacterMovement") })->SetPropertyFlags(CPF_DisableEditOnInstance);
#endif
}

#if WITH_EDITOR
bool AAlsCharacter::CanEditChange(const FProperty* Property) const
{
	return Super::CanEditChange(Property) &&
		Property->GetFName() != GET_MEMBER_NAME_CHECKED(ThisClass, bUseControllerRotationPitch) &&
		Property->GetFName() != GET_MEMBER_NAME_CHECKED(ThisClass, bUseControllerRotationYaw) &&
		Property->GetFName() != GET_MEMBER_NAME_CHECKED(ThisClass, bUseControllerRotationRoll);
}
#endif

void AAlsCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Parameters;
	Parameters.bIsPushBased = true;

	Parameters.Condition = COND_SkipOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredStance, Parameters)
		DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredGait, Parameters)
		DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, bDesiredAiming, Parameters)
		DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredRotationMode, Parameters)
		DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ViewMode, Parameters)
		DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, OverlayMode, Parameters)

		DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, ReplicatedViewRotation, Parameters)
		DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, InputDirection, Parameters)
		DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, DesiredVelocityYawAngle, Parameters)
		DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, RagdollTargetLocation, Parameters)
}

void AAlsCharacter::PreRegisterAllComponents()
{
	// Set some default values here so that the animation instance and the
	// camera component can read the most up-to-date values during initialization.

	RotationMode = bDesiredAiming ? AlsRotationModeTags::Aiming : DesiredRotationMode;
	Stance = DesiredStance;
	Gait = DesiredGait;

	Super::PreRegisterAllComponents();
}

void AAlsCharacter::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	SetReplicatedViewRotation(Super::GetViewRotation().GetNormalized(), false);

	ViewState.NetworkSmoothing.InitialRotation = ReplicatedViewRotation;
	ViewState.NetworkSmoothing.TargetRotation = ReplicatedViewRotation;
	ViewState.NetworkSmoothing.CurrentRotation = ReplicatedViewRotation;

	ViewState.Rotation = ReplicatedViewRotation;
	ViewState.PreviousYawAngle = UE_REAL_TO_FLOAT(ReplicatedViewRotation.Yaw);

	const auto& ActorTransform{ GetActorTransform() };

	LocomotionState.Location = ActorTransform.GetLocation();
	LocomotionState.RotationQuaternion = ActorTransform.GetRotation();
	LocomotionState.Rotation = GetActorRotation();
	LocomotionState.PreviousYawAngle = UE_REAL_TO_FLOAT(LocomotionState.Rotation.Yaw);

	RefreshTargetYawAngleUsingLocomotionRotation();

	LocomotionState.InputYawAngle = UE_REAL_TO_FLOAT(LocomotionState.Rotation.Yaw);
	LocomotionState.VelocityYawAngle = UE_REAL_TO_FLOAT(LocomotionState.Rotation.Yaw);
}

void AAlsCharacter::PostInitializeComponents()
{
	// Make sure the mesh and animation blueprint are ticking after the character so they can access the most up-to-date character state.

	GetMesh()->AddTickPrerequisiteActor(this);

	AlsCharacterMovement->OnPhysicsRotation.AddUObject(this, &ThisClass::CharacterMovement_OnPhysicsRotation);

	// Pass current movement settings to the movement component.

	AlsCharacterMovement->SetMovementSettings(MovementSettings);

	AnimationInstance = Cast<UAlsAnimationInstance>(GetMesh()->GetAnimInstance());

	Super::PostInitializeComponents();
}

void AAlsCharacter::BeginPlay()
{
	ALS_ENSURE(IsValid(Settings));
	ALS_ENSURE(IsValid(MovementSettings));
	ALS_ENSURE(AnimationInstance.IsValid());

	ALS_ENSURE_MESSAGE(!bUseControllerRotationPitch && !bUseControllerRotationYaw && !bUseControllerRotationRoll,
		TEXT("These settings are not allowed and must be turned off!"));

	Super::BeginPlay();

	if (GetLocalRole() >= ROLE_AutonomousProxy)
	{
		// Teleportation of simulated proxies is detected differently, see
		// AAlsCharacter::PostNetReceiveLocationAndRotation() and AAlsCharacter::OnRep_ReplicatedBasedMovement().

		GetCapsuleComponent()->TransformUpdated.AddWeakLambda(
			this, [this](USceneComponent*, const EUpdateTransformFlags, const ETeleportType TeleportType)
			{
				if (TeleportType != ETeleportType::None && AnimationInstance.IsValid())
				{
					AnimationInstance->MarkTeleported();
				}
			});
	}

	RefreshMeshProperties();

	ViewState.NetworkSmoothing.bEnabled |= IsValid(Settings) &&
		Settings->View.bEnableNetworkSmoothing && GetLocalRole() == ROLE_SimulatedProxy;

	// Update states to use the initial desired values.

	RefreshRotationMode();

	AlsCharacterMovement->SetRotationMode(RotationMode);

	ApplyDesiredStance();

	AlsCharacterMovement->SetStance(Stance);

	RefreshGait();

	OnOverlayModeChanged(OverlayMode);

	CurrentZVelocity = AlsCharacterMovement->JumpZVelocity;
}

void AAlsCharacter::PostNetReceiveLocationAndRotation()
{
	// AActor::PostNetReceiveLocationAndRotation() function is only called on simulated proxies, so there is no need to check roles here.

	const auto PreviousLocation{ GetActorLocation() };

	// Ignore server-replicated rotation on simulated proxies because ALS itself has full control over character rotation.

	GetReplicatedMovement_Mutable().Rotation = GetActorRotation();

	Super::PostNetReceiveLocationAndRotation();

	// Detect teleportation of simulated proxies.

	auto bTeleported{ static_cast<bool>(bSimGravityDisabled) };

	if (!bTeleported && !ReplicatedBasedMovement.HasRelativeLocation())
	{
		const auto NewLocation{ FRepMovement::RebaseOntoLocalOrigin(GetReplicatedMovement().Location, this) };

		bTeleported |= FVector::DistSquared(PreviousLocation, NewLocation) > AlsCharacterConstants::TeleportDistanceThresholdSquared;
	}

	if (bTeleported && AnimationInstance.IsValid())
	{
		AnimationInstance->MarkTeleported();
	}
}

void AAlsCharacter::OnRep_ReplicatedBasedMovement()
{
	// ACharacter::OnRep_ReplicatedBasedMovement() is only called on simulated proxies, so there is no need to check roles here.

	const auto PreviousLocation{ GetActorLocation() };

	// Ignore server-replicated rotation on simulated proxies because ALS itself has full control over character rotation.

	if (ReplicatedBasedMovement.HasRelativeRotation())
	{
		FVector MovementBaseLocation;
		FQuat MovementBaseRotation;

		MovementBaseUtility::GetMovementBaseTransform(ReplicatedBasedMovement.MovementBase, ReplicatedBasedMovement.BoneName,
			MovementBaseLocation, MovementBaseRotation);

		ReplicatedBasedMovement.Rotation = (MovementBaseRotation.Inverse() * GetActorQuat()).Rotator();
	}
	else
	{
		ReplicatedBasedMovement.Rotation = GetActorRotation();
	}

	Super::OnRep_ReplicatedBasedMovement();

	// Detect teleportation of simulated proxies.

	auto bTeleported{ static_cast<bool>(bSimGravityDisabled) };

	if (!bTeleported && BasedMovement.HasRelativeLocation())
	{
		const auto NewLocation{
			GetCharacterMovement()->OldBaseLocation + GetCharacterMovement()->OldBaseQuat.RotateVector(BasedMovement.Location)
		};

		bTeleported |= FVector::DistSquared(PreviousLocation, NewLocation) > AlsCharacterConstants::TeleportDistanceThresholdSquared;
	}

	if (bTeleported && AnimationInstance.IsValid())
	{
		AnimationInstance->MarkTeleported();
	}
}

void AAlsCharacter::Tick(const float DeltaTime)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("AAlsCharacter::Tick()"), STAT_AAlsCharacter_Tick, STATGROUP_Als)

	if (!IsValid(Settings) || !AnimationInstance.IsValid())
	{
		Super::Tick(DeltaTime);
		return;
	}

	RefreshMovementBase();

	RefreshMeshProperties();

	RefreshInput(DeltaTime);

	RefreshLocomotionEarly();

	RefreshView(DeltaTime);
	RefreshRotationMode();
	RefreshLocomotion(DeltaTime);
	RefreshGait();

	RefreshGroundedRotation(DeltaTime);
	RefreshInAirRotation(DeltaTime);

	if (!IsFirstJumpClick)
	{
		StartMantlingInAir();
	}

	RefreshMantling();
	RefreshRagdolling(DeltaTime);
	RefreshRolling(DeltaTime);

	Super::Tick(DeltaTime);

	RefreshLocomotionLate(DeltaTime);

	CalculateBackwardAndStrafeMoveReducement();

	CalculateFallDistanceToCountStunAndDamage();

	CalculateSpeedMultiplierOnGoingUpOrDown();

	CalculateStartStopSliding();

	CalculateWindInfluenceOnFalling();

	CalculateWindInfluenceEffect();

	StunRecovery();

	ShockEffect();

	DiscombobulateEffect();

	MagneticEffect();

	CalculateInkEffect();

	Restore_Speed_JumpHeight_Health();

	//HealthRecovery();

	RefreshStaminaHealthStandingMultiplier();

	RefreshStaminaHealthRunningMultiplier();

	StaminaRecovery();

	RefreshRecoil();

	RefreshAimAccuracy();

	RefreshDamageAmountOnMovingOrOnStanding();

	RefreshDamage();

	CheckIfShouldIncreaseWalkAndRunSpeed();

	CheckIfShouldDecreaseWalkRunSpeedAnDamage();

	IncreaseHealth_30_20c();
}

void AAlsCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	RefreshMeshProperties();

	// Enable view network smoothing on the listen server here because the remote role may not be valid yet during begin play.

	ViewState.NetworkSmoothing.bEnabled |= IsValid(Settings) && Settings->View.bEnableListenServerNetworkSmoothing &&
		IsNetMode(NM_ListenServer) && GetRemoteRole() == ROLE_AutonomousProxy;

	SetWindDirection();
}

void AAlsCharacter::Restart()
{
	Super::Restart();

	ApplyDesiredStance();
}

void AAlsCharacter::RefreshMeshProperties() const
{
	const auto bStandalone{ IsNetMode(NM_Standalone) };
	const auto bDedicatedServer{ IsNetMode(NM_DedicatedServer) };
	const auto bListenServer{ IsNetMode(NM_ListenServer) };

	const auto bAuthority{ GetLocalRole() >= ROLE_Authority };
	const auto bRemoteAutonomousProxy{ GetRemoteRole() == ROLE_AutonomousProxy };
	const auto bLocallyControlled{ IsLocallyControlled() };

	// Make sure that the pose is always ticked on the server when the character is controlled
	// by a remote client, otherwise some problems may arise (such as jitter when rolling).

	const auto DefaultTickOption{ GetClass()->GetDefaultObject<ThisClass>()->GetMesh()->VisibilityBasedAnimTickOption };

	const auto TargetTickOption{
		!bStandalone && bAuthority && bRemoteAutonomousProxy
			? EVisibilityBasedAnimTickOption::AlwaysTickPose
			: EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered
	};

	// Keep the default tick option, at least if the target tick option is not required by the plugin to work properly.

	GetMesh()->VisibilityBasedAnimTickOption = FMath::Min(TargetTickOption, DefaultTickOption);

	const auto bMeshIsTicking{
		GetMesh()->bRecentlyRendered || GetMesh()->VisibilityBasedAnimTickOption <= EVisibilityBasedAnimTickOption::AlwaysTickPose
	};

	// Use absolute mesh rotation to be able to precisely synchronize character rotation
	// with animations by manually updating the mesh rotation from the animation instance.

	// This is necessary in cases where the character and the animation instance are ticking
	// at different frequencies, which leads to desynchronization of rotation animations
	// with the character rotation, as well as foot sliding when the foot lock is active.

	// To save performance, use this only when really necessary, such as
	// when URO is enabled, or for autonomous proxies on the listen server.

	const auto bUROActive{ GetMesh()->AnimUpdateRateParams != nullptr && GetMesh()->AnimUpdateRateParams->UpdateRate > 1 };
	const auto bAutonomousProxyOnListenServer{ bListenServer && bRemoteAutonomousProxy };

	// Can't use absolute mesh rotation when the character is standing on a rotating object, as it
	// causes constant rotation jitter. Be careful: although it eliminates jitter in this case, not
	// using absolute mesh rotation can cause jitter when rotating in place or turning in place.

	const auto bStandingOnRotatingObject{ MovementBase.bHasRelativeRotation };

	const auto bUseAbsoluteRotation{
		bMeshIsTicking && !bDedicatedServer && !bLocallyControlled && !bStandingOnRotatingObject &&
		(bUROActive || bAutonomousProxyOnListenServer)
	};

	if (GetMesh()->IsUsingAbsoluteRotation() != bUseAbsoluteRotation)
	{
		GetMesh()->SetUsingAbsoluteRotation(bUseAbsoluteRotation);

		// Instantly update the relative mesh rotation, otherwise it will be incorrect during this tick.

		if (bUseAbsoluteRotation || !IsValid(GetMesh()->GetAttachParent()))
		{
			GetMesh()->SetRelativeRotation_Direct(
				GetMesh()->GetRelativeRotationCache().QuatToRotator(GetMesh()->GetComponentQuat()));
		}
		else
		{
			GetMesh()->SetRelativeRotation_Direct(
				GetMesh()->GetRelativeRotationCache().QuatToRotator(GetActorQuat().Inverse() * GetMesh()->GetComponentQuat()));
		}
	}

	if (!bMeshIsTicking)
	{
		AnimationInstance->MarkPendingUpdate();
	}
}

void AAlsCharacter::RefreshMovementBase()
{
	if (BasedMovement.MovementBase != MovementBase.Primitive || BasedMovement.BoneName != MovementBase.BoneName)
	{
		MovementBase.Primitive = BasedMovement.MovementBase;
		MovementBase.BoneName = BasedMovement.BoneName;
		MovementBase.bBaseChanged = true;
	}
	else
	{
		MovementBase.bBaseChanged = false;
	}

	MovementBase.bHasRelativeLocation = BasedMovement.HasRelativeLocation();
	MovementBase.bHasRelativeRotation = MovementBase.bHasRelativeLocation && BasedMovement.bRelativeRotation;

	const auto PreviousRotation{ MovementBase.Rotation };

	MovementBaseUtility::GetMovementBaseTransform(BasedMovement.MovementBase, BasedMovement.BoneName,
		MovementBase.Location, MovementBase.Rotation);

	MovementBase.DeltaRotation = MovementBase.bHasRelativeLocation && !MovementBase.bBaseChanged
		? (MovementBase.Rotation * PreviousRotation.Inverse()).Rotator()
		: FRotator::ZeroRotator;
}

void AAlsCharacter::SetViewMode(const FGameplayTag& NewViewMode)
{
	SetViewMode(NewViewMode, true);
}

void AAlsCharacter::SetViewMode(const FGameplayTag& NewViewMode, const bool bSendRpc)
{
	if (ViewMode == NewViewMode || GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	ViewMode = NewViewMode;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ViewMode, this)

		if (bSendRpc)
		{
			if (GetLocalRole() >= ROLE_Authority)
			{
				ClientSetViewMode(ViewMode);
			}
			else
			{
				ServerSetViewMode(ViewMode);
			}
		}
}

void AAlsCharacter::ClientSetViewMode_Implementation(const FGameplayTag& NewViewMode)
{
	SetViewMode(NewViewMode, false);
}

void AAlsCharacter::ServerSetViewMode_Implementation(const FGameplayTag& NewViewMode)
{
	SetViewMode(NewViewMode, false);
}

void AAlsCharacter::OnMovementModeChanged(const EMovementMode PreviousMovementMode, const uint8 PreviousCustomMode)
{
	// Use the character movement mode to set the locomotion mode to the right value. This allows you to have a
	// custom set of movement modes but still use the functionality of the default character movement component.

	switch (GetCharacterMovement()->MovementMode)
	{
	case MOVE_Walking:
	case MOVE_NavWalking:
		SetLocomotionMode(AlsLocomotionModeTags::Grounded);
		break;

	case MOVE_Falling:
		SetLocomotionMode(AlsLocomotionModeTags::InAir);
		break;

	default:
		SetLocomotionMode(FGameplayTag::EmptyTag);
		break;
	}

	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
}

void AAlsCharacter::SetLocomotionMode(const FGameplayTag& NewLocomotionMode)
{
	if (LocomotionMode != NewLocomotionMode)
	{
		const auto PreviousLocomotionMode{ LocomotionMode };

		LocomotionMode = NewLocomotionMode;

		NotifyLocomotionModeChanged(PreviousLocomotionMode);
	}
}

void AAlsCharacter::NotifyLocomotionModeChanged(const FGameplayTag& PreviousLocomotionMode)
{
	ApplyDesiredStance();

	if (LocomotionMode == AlsLocomotionModeTags::Grounded &&
		PreviousLocomotionMode == AlsLocomotionModeTags::InAir)
	{
		if (Settings->Ragdolling.bStartRagdollingOnLand &&
			LocomotionState.Velocity.Z <= -Settings->Ragdolling.RagdollingOnLandSpeedThreshold)
		{
			StartRagdolling();
		}
		else if (Settings->Rolling.bStartRollingOnLand &&
			LocomotionState.Velocity.Z <= -Settings->Rolling.RollingOnLandSpeedThreshold)
		{
			static constexpr auto PlayRate{ 1.3f };

			StartRolling(PlayRate, LocomotionState.bHasSpeed
				? LocomotionState.VelocityYawAngle
				: UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(GetActorRotation().Yaw)));
		}
		else
		{
			static constexpr auto HasInputBrakingFrictionFactor{ 0.5f };
			static constexpr auto NoInputBrakingFrictionFactor{ 3.0f };

			GetCharacterMovement()->BrakingFrictionFactor = LocomotionState.bHasInput
				? HasInputBrakingFrictionFactor
				: NoInputBrakingFrictionFactor;

			static constexpr auto ResetDelay{ 0.5f };

			GetWorldTimerManager().SetTimer(BrakingFrictionFactorResetTimer,
				FTimerDelegate::CreateWeakLambda(this, [this]
					{
						GetCharacterMovement()->BrakingFrictionFactor = 0.0f;
					}), ResetDelay, false);

			// Block character rotation towards the last input direction after landing to
			// prevent legs from twisting into a spiral while the landing animation is playing.

			LocomotionState.bRotationTowardsLastInputDirectionBlocked = true;
		}
	}
	else if (LocomotionMode == AlsLocomotionModeTags::InAir &&
		LocomotionAction == AlsLocomotionActionTags::Rolling &&
		Settings->Rolling.bInterruptRollingWhenInAir)
	{
		// If the character is currently rolling, then enable ragdolling.

		StartRagdolling();
	}

	OnLocomotionModeChanged(PreviousLocomotionMode);
}

void AAlsCharacter::OnLocomotionModeChanged_Implementation(const FGameplayTag& PreviousLocomotionMode) {}

void AAlsCharacter::SetDesiredAiming(const bool bNewDesiredAiming)
{
	SetDesiredAiming(bNewDesiredAiming, true);
	StartStopAim(bNewDesiredAiming);
}

void AAlsCharacter::SetDesiredAiming(const bool bNewDesiredAiming, const bool bSendRpc)
{
	if (bDesiredAiming == bNewDesiredAiming || GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	bDesiredAiming = bNewDesiredAiming;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, bDesiredAiming, this)

		OnDesiredAimingChanged(!bDesiredAiming);

	if (bSendRpc)
	{
		if (GetLocalRole() >= ROLE_Authority)
		{
			ClientSetDesiredAiming(bDesiredAiming);
		}
		else
		{
			ServerSetDesiredAiming(bDesiredAiming);
		}
	}
}

void AAlsCharacter::ClientSetDesiredAiming_Implementation(const bool bNewDesiredAiming)
{
	SetDesiredAiming(bNewDesiredAiming, false);
}

void AAlsCharacter::ServerSetDesiredAiming_Implementation(const bool bNewDesiredAiming)
{
	SetDesiredAiming(bNewDesiredAiming, false);
}

void AAlsCharacter::OnReplicated_DesiredAiming(const bool bPreviousDesiredAiming)
{
	OnDesiredAimingChanged(bPreviousDesiredAiming);
}

void AAlsCharacter::OnDesiredAimingChanged_Implementation(const bool bPreviousDesiredAiming) {}

void AAlsCharacter::SetDesiredRotationMode(const FGameplayTag& NewDesiredRotationMode)
{
	SetDesiredRotationMode(NewDesiredRotationMode, true);
}

void AAlsCharacter::SetDesiredRotationMode(const FGameplayTag& NewDesiredRotationMode, const bool bSendRpc)
{
	if (DesiredRotationMode == NewDesiredRotationMode || GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	DesiredRotationMode = NewDesiredRotationMode;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DesiredRotationMode, this)

		if (bSendRpc)
		{
			if (GetLocalRole() >= ROLE_Authority)
			{
				ClientSetDesiredRotationMode(DesiredRotationMode);
			}
			else
			{
				ServerSetDesiredRotationMode(DesiredRotationMode);
			}
		}
}

void AAlsCharacter::ClientSetDesiredRotationMode_Implementation(const FGameplayTag& NewDesiredRotationMode)
{
	SetDesiredRotationMode(NewDesiredRotationMode, false);
}

void AAlsCharacter::ServerSetDesiredRotationMode_Implementation(const FGameplayTag& NewDesiredRotationMode)
{
	SetDesiredRotationMode(NewDesiredRotationMode, false);
}

void AAlsCharacter::SetRotationMode(const FGameplayTag& NewRotationMode)
{
	AlsCharacterMovement->SetRotationMode(NewRotationMode);

	if (RotationMode != NewRotationMode)
	{
		const auto PreviousRotationMode{ RotationMode };

		RotationMode = NewRotationMode;

		OnRotationModeChanged(PreviousRotationMode);
	}
}

void AAlsCharacter::OnRotationModeChanged_Implementation(const FGameplayTag& PreviousRotationMode) {}

void AAlsCharacter::RefreshRotationMode()
{
	const auto bSprinting{ Gait == AlsGaitTags::Sprinting };
	const auto bAiming{ bDesiredAiming || DesiredRotationMode == AlsRotationModeTags::Aiming };

	if (ViewMode == AlsViewModeTags::FirstPerson)
	{
		if (LocomotionMode == AlsLocomotionModeTags::InAir)
		{
			if (bAiming && Settings->bAllowAimingWhenInAir)
			{
				SetRotationMode(AlsRotationModeTags::Aiming);
			}
			else
			{
				SetRotationMode(AlsRotationModeTags::ViewDirection);
			}

			return;
		}

		// Grounded and other locomotion modes.

		if (bAiming && (!bSprinting || !Settings->bSprintHasPriorityOverAiming))
		{
			SetRotationMode(AlsRotationModeTags::Aiming);
		}
		else
		{
			SetRotationMode(AlsRotationModeTags::ViewDirection);
		}

		return;
	}

	// Third person and other view modes.

	if (LocomotionMode == AlsLocomotionModeTags::InAir)
	{
		if (bAiming && Settings->bAllowAimingWhenInAir)
		{
			SetRotationMode(AlsRotationModeTags::Aiming);
		}
		else if (bAiming)
		{
			SetRotationMode(AlsRotationModeTags::ViewDirection);
		}
		else
		{
			SetRotationMode(DesiredRotationMode);
		}

		return;
	}

	// Grounded and other locomotion modes.

	if (bSprinting)
	{
		if (bAiming && !Settings->bSprintHasPriorityOverAiming)
		{
			SetRotationMode(AlsRotationModeTags::Aiming);
		}
		else if (Settings->bRotateToVelocityWhenSprinting)
		{
			SetRotationMode(AlsRotationModeTags::VelocityDirection);
		}
		else if (bAiming)
		{
			SetRotationMode(AlsRotationModeTags::ViewDirection);
		}
		else
		{
			SetRotationMode(DesiredRotationMode);
		}
	}
	else // Not sprinting.
	{
		if (bAiming)
		{
			SetRotationMode(AlsRotationModeTags::Aiming);
		}
		else
		{
			SetRotationMode(DesiredRotationMode);
		}
	}
}

void AAlsCharacter::SetDesiredStance(const FGameplayTag& NewDesiredStance)
{
	SetDesiredStance(NewDesiredStance, true);
}

void AAlsCharacter::SetDesiredStance(const FGameplayTag& NewDesiredStance, const bool bSendRpc)
{
	if (DesiredStance == NewDesiredStance || GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	DesiredStance = NewDesiredStance;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DesiredStance, this)

		if (bSendRpc)
		{
			if (GetLocalRole() >= ROLE_Authority)
			{
				ClientSetDesiredStance(DesiredStance);
			}
			else
			{
				ServerSetDesiredStance(DesiredStance);
			}
		}

	ApplyDesiredStance();
}

void AAlsCharacter::ClientSetDesiredStance_Implementation(const FGameplayTag& NewDesiredStance)
{
	SetDesiredStance(NewDesiredStance, false);
}

void AAlsCharacter::ServerSetDesiredStance_Implementation(const FGameplayTag& NewDesiredStance)
{
	SetDesiredStance(NewDesiredStance, false);
}

void AAlsCharacter::ApplyDesiredStance()
{
	if (!LocomotionAction.IsValid())
	{
		if (LocomotionMode == AlsLocomotionModeTags::Grounded)
		{
			if (DesiredStance == AlsStanceTags::Standing)
			{
				UnCrouch();
			}
			else if (DesiredStance == AlsStanceTags::Crouching)
			{
				Crouch();
			}
		}
		else if (LocomotionMode == AlsLocomotionModeTags::InAir)
		{
			UnCrouch();
		}
	}
	else if (LocomotionAction == AlsLocomotionActionTags::Rolling && Settings->Rolling.bCrouchOnStart)
	{
		Crouch();
	}
}

bool AAlsCharacter::CanCrouch() const
{
	// This allows the ACharacter::Crouch() function to execute properly when bIsCrouched is true.
	// TODO Wait for https://github.com/EpicGames/UnrealEngine/pull/9558 to be merged into the engine.

	return bIsCrouched || Super::CanCrouch();
}

void AAlsCharacter::OnStartCrouch(const float HalfHeightAdjust, const float ScaledHalfHeightAdjust)
{
	auto* PredictionData{ GetCharacterMovement()->GetPredictionData_Client_Character() };

	if (PredictionData != nullptr && GetLocalRole() <= ROLE_SimulatedProxy &&
		ScaledHalfHeightAdjust > 0.0f && IsPlayingNetworkedRootMotionMontage())
	{
		// The code below essentially undoes the changes that will be made later at the end of the UCharacterMovementComponent::Crouch()
		// function because they literally break network smoothing when crouching while the root motion montage is playing, causing the
		// mesh to take an incorrect location for a while.

		// TODO Check the need for this fix in future engine versions.

		PredictionData->MeshTranslationOffset.Z += ScaledHalfHeightAdjust;
		PredictionData->OriginalMeshTranslationOffset = PredictionData->MeshTranslationOffset;
	}

	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	SetStance(AlsStanceTags::Crouching);
}

void AAlsCharacter::OnEndCrouch(const float HalfHeightAdjust, const float ScaledHalfHeightAdjust)
{
	auto* PredictionData{ GetCharacterMovement()->GetPredictionData_Client_Character() };

	if (PredictionData != nullptr && GetLocalRole() <= ROLE_SimulatedProxy &&
		ScaledHalfHeightAdjust > 0.0f && IsPlayingNetworkedRootMotionMontage())
	{
		// Same fix as in AAlsCharacter::OnStartCrouch().

		PredictionData->MeshTranslationOffset.Z -= ScaledHalfHeightAdjust;
		PredictionData->OriginalMeshTranslationOffset = PredictionData->MeshTranslationOffset;
	}

	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);

	SetStance(AlsStanceTags::Standing);
}

void AAlsCharacter::SetStance(const FGameplayTag& NewStance)
{
	AlsCharacterMovement->SetStance(NewStance);

	if (Stance != NewStance)
	{
		const auto PreviousStance{ Stance };

		Stance = NewStance;

		OnStanceChanged(PreviousStance);
	}
}

void AAlsCharacter::OnStanceChanged_Implementation(const FGameplayTag& PreviousStance) {}

void AAlsCharacter::SetDesiredGait(const FGameplayTag& NewDesiredGait)
{
	SetDesiredGait(NewDesiredGait, true);
}

void AAlsCharacter::SetDesiredGait(const FGameplayTag& NewDesiredGait, const bool bSendRpc)
{
	if (DesiredGait == NewDesiredGait || GetLocalRole() < ROLE_AutonomousProxy)
	{
		return;
	}

	DesiredGait = NewDesiredGait;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, DesiredGait, this)

		if (bSendRpc)
		{
			if (GetLocalRole() >= ROLE_Authority)
			{
				ClientSetDesiredGait(DesiredGait);
			}
			else
			{
				ServerSetDesiredGait(DesiredGait);
			}
		}
}

void AAlsCharacter::ClientSetDesiredGait_Implementation(const FGameplayTag& NewDesiredGait)
{
	SetDesiredGait(NewDesiredGait, false);
}

void AAlsCharacter::ServerSetDesiredGait_Implementation(const FGameplayTag& NewDesiredGait)
{
	SetDesiredGait(NewDesiredGait, false);
}

void AAlsCharacter::SetGait(const FGameplayTag& NewGait)
{
	if (Gait != NewGait)
	{
		const auto PreviousGait{ Gait };

		Gait = NewGait;

		OnGaitChanged(PreviousGait);
	}
}

void AAlsCharacter::OnGaitChanged_Implementation(const FGameplayTag& PreviousGait) {}

void AAlsCharacter::RefreshGait()
{
	if (LocomotionMode != AlsLocomotionModeTags::Grounded)
	{
		return;
	}

	const auto MaxAllowedGait{ CalculateMaxAllowedGait() };

	// Update the character max walk speed to the configured speeds based on the currently max allowed gait.

	AlsCharacterMovement->SetMaxAllowedGait(MaxAllowedGait);

	SetGait(CalculateActualGait(MaxAllowedGait));
}

FGameplayTag AAlsCharacter::CalculateMaxAllowedGait() const
{
	// Calculate the max allowed gait. This represents the maximum gait the character is currently allowed
	// to be in and can be determined by the desired gait, the rotation mode, the stance, etc. For example,
	// if you wanted to force the character into a walking state while indoors, this could be done here.

	if (DesiredGait != AlsGaitTags::Sprinting)
	{
		return DesiredGait;
	}

	if (CanSprint())
	{
		return AlsGaitTags::Sprinting;
	}

	return AlsGaitTags::Running;
}

FGameplayTag AAlsCharacter::CalculateActualGait(const FGameplayTag& MaxAllowedGait) const
{
	// Calculate the new gait. This is calculated by the actual movement of the character and so it can be
	// different from the desired gait or max allowed gait. For instance, if the max allowed gait becomes
	// walking, the new gait will still be running until the character decelerates to the walking speed.

	if (LocomotionState.Speed < AlsCharacterMovement->GetGaitSettings().WalkSpeed + 10.0f)
	{
		return AlsGaitTags::Walking;
	}

	if (LocomotionState.Speed < AlsCharacterMovement->GetGaitSettings().RunSpeed + 10.0f || MaxAllowedGait != AlsGaitTags::Sprinting)
	{
		return AlsGaitTags::Running;
	}

	return AlsGaitTags::Sprinting;
}

bool AAlsCharacter::CanSprint() const
{
	// Determine if the character can sprint based on the rotation mode and input direction.
	// If the character is in view direction rotation mode, only allow sprinting if there is
	// input and if the input direction is aligned with the view direction within 50 degrees.

	if (!LocomotionState.bHasInput || Stance != AlsStanceTags::Standing ||
		(RotationMode == AlsRotationModeTags::Aiming && !Settings->bSprintHasPriorityOverAiming))
	{
		return false;
	}

	if (ViewMode != AlsViewModeTags::FirstPerson &&
		(DesiredRotationMode == AlsRotationModeTags::VelocityDirection || Settings->bRotateToVelocityWhenSprinting))
	{
		return true;
	}

	static constexpr auto ViewRelativeAngleThreshold{ 50.0f };

	if (FMath::Abs(FRotator3f::NormalizeAxis(UE_REAL_TO_FLOAT(
		LocomotionState.InputYawAngle - ViewState.Rotation.Yaw))) < ViewRelativeAngleThreshold)
	{
		return true;
	}

	return false;
}

void AAlsCharacter::SetOverlayMode(const FGameplayTag& NewOverlayMode)
{
	SetOverlayMode(NewOverlayMode, true);
}

void AAlsCharacter::SetOverlayMode(const FGameplayTag& NewOverlayMode, const bool bSendRpc)
{
	if (OverlayMode == NewOverlayMode || GetLocalRole() <= ROLE_SimulatedProxy)
	{
		return;
	}

	const auto PreviousOverlayMode{ OverlayMode };

	OverlayMode = NewOverlayMode;

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, OverlayMode, this)

		OnOverlayModeChanged(PreviousOverlayMode);

	if (bSendRpc)
	{
		if (GetLocalRole() >= ROLE_Authority)
		{
			ClientSetOverlayMode(OverlayMode);
		}
		else
		{
			ServerSetOverlayMode(OverlayMode);
		}
	}
}

void AAlsCharacter::ClientSetOverlayMode_Implementation(const FGameplayTag& NewOverlayMode)
{
	SetOverlayMode(NewOverlayMode, false);
}

void AAlsCharacter::ServerSetOverlayMode_Implementation(const FGameplayTag& NewOverlayMode)
{
	SetOverlayMode(NewOverlayMode, false);
}

void AAlsCharacter::OnReplicated_OverlayMode(const FGameplayTag& PreviousOverlayMode)
{
	OnOverlayModeChanged(PreviousOverlayMode);
}

void AAlsCharacter::OnOverlayModeChanged_Implementation(const FGameplayTag& PreviousOverlayMode) {}

void AAlsCharacter::SetLocomotionAction(const FGameplayTag& NewLocomotionAction)
{
	if (LocomotionAction != NewLocomotionAction)
	{
		const auto PreviousLocomotionAction{ LocomotionAction };

		LocomotionAction = NewLocomotionAction;

		NotifyLocomotionActionChanged(PreviousLocomotionAction);
	}
}

void AAlsCharacter::NotifyLocomotionActionChanged(const FGameplayTag& PreviousLocomotionAction)
{
	if (!LocomotionAction.IsValid())
	{
		AlsCharacterMovement->SetInputBlocked(false);
	}

	ApplyDesiredStance();

	OnLocomotionActionChanged(PreviousLocomotionAction);
}

void AAlsCharacter::OnLocomotionActionChanged_Implementation(const FGameplayTag& PreviousLocomotionAction) {}

FRotator AAlsCharacter::GetViewRotation() const
{
	return ViewState.Rotation;
}

void AAlsCharacter::SetInputDirection(FVector NewInputDirection)
{
	NewInputDirection = NewInputDirection.GetSafeNormal();

	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, InputDirection, NewInputDirection, this);
}

void AAlsCharacter::RefreshInput(const float DeltaTime)
{
	if (GetLocalRole() >= ROLE_AutonomousProxy)
	{
		SetInputDirection(GetCharacterMovement()->GetCurrentAcceleration() / GetCharacterMovement()->GetMaxAcceleration());
	}

	LocomotionState.bHasInput = InputDirection.SizeSquared() > UE_KINDA_SMALL_NUMBER;

	if (LocomotionState.bHasInput)
	{
		LocomotionState.InputYawAngle = UE_REAL_TO_FLOAT(UAlsMath::DirectionToAngleXY(InputDirection));
	}
}

void AAlsCharacter::SetReplicatedViewRotation(const FRotator& NewViewRotation, const bool bSendRpc)
{
	if (!ReplicatedViewRotation.Equals(NewViewRotation))
	{
		ReplicatedViewRotation = NewViewRotation;

		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ReplicatedViewRotation, this)

			if (bSendRpc && GetLocalRole() == ROLE_AutonomousProxy)
			{
				ServerSetReplicatedViewRotation(ReplicatedViewRotation);
			}
	}
}

void AAlsCharacter::ServerSetReplicatedViewRotation_Implementation(const FRotator& NewViewRotation)
{
	SetReplicatedViewRotation(NewViewRotation, false);
}

void AAlsCharacter::OnReplicated_ReplicatedViewRotation()
{
	CorrectViewNetworkSmoothing(ReplicatedViewRotation, MovementBase.bHasRelativeRotation);
}

void AAlsCharacter::CorrectViewNetworkSmoothing(const FRotator& NewTargetRotation, const bool bRelativeTargetRotation)
{
	// Based on UCharacterMovementComponent::SmoothCorrection().

	auto& NetworkSmoothing{ ViewState.NetworkSmoothing };

	NetworkSmoothing.TargetRotation = bRelativeTargetRotation
		? (MovementBase.Rotation * NewTargetRotation.Quaternion()).Rotator()
		: NewTargetRotation.GetNormalized();

	if (!NetworkSmoothing.bEnabled)
	{
		NetworkSmoothing.InitialRotation = NetworkSmoothing.TargetRotation;
		NetworkSmoothing.CurrentRotation = NetworkSmoothing.TargetRotation;
		return;
	}

	const auto bListenServer{ IsNetMode(NM_ListenServer) };

	const auto NewNetworkSmoothingServerTime{
		bListenServer
			? GetCharacterMovement()->GetServerLastTransformUpdateTimeStamp()
			: GetReplicatedServerLastTransformUpdateTimeStamp()
	};

	if (NewNetworkSmoothingServerTime <= 0.0f)
	{
		return;
	}

	NetworkSmoothing.InitialRotation = NetworkSmoothing.CurrentRotation;

	// Using server time lets us know how much time elapsed, regardless of packet lag variance.

	const auto ServerDeltaTime{ NewNetworkSmoothingServerTime - NetworkSmoothing.ServerTime };

	NetworkSmoothing.ServerTime = NewNetworkSmoothingServerTime;

	// Don't let the client fall too far behind or run ahead of new server time.

	const auto MaxServerDeltaTime{ GetDefault<AGameNetworkManager>()->MaxClientSmoothingDeltaTime };

	const auto MinServerDeltaTime{
		FMath::Min(MaxServerDeltaTime, bListenServer
										   ? GetCharacterMovement()->ListenServerNetworkSimulatedSmoothLocationTime
										   : GetCharacterMovement()->NetworkSimulatedSmoothLocationTime)
	};

	// Calculate how far behind we can be after receiving a new server time.

	const auto MinClientDeltaTime{ FMath::Clamp(ServerDeltaTime * 1.25f, MinServerDeltaTime, MaxServerDeltaTime) };

	NetworkSmoothing.ClientTime = FMath::Clamp(NetworkSmoothing.ClientTime,
		NetworkSmoothing.ServerTime - MinClientDeltaTime,
		NetworkSmoothing.ServerTime);

	// Compute actual delta between new server time and client simulation.

	NetworkSmoothing.Duration = NetworkSmoothing.ServerTime - NetworkSmoothing.ClientTime;
}

void AAlsCharacter::RefreshView(const float DeltaTime)
{
	if (MovementBase.bHasRelativeRotation)
	{
		// Offset the rotations to keep them relative to the movement base.

		ViewState.Rotation.Pitch += MovementBase.DeltaRotation.Pitch;
		ViewState.Rotation.Yaw += MovementBase.DeltaRotation.Yaw;
		ViewState.Rotation.Normalize();
	}

	ViewState.PreviousYawAngle = UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw);

	if (MovementBase.bHasRelativeRotation)
	{
		if (IsLocallyControlled())
		{
			// We can't depend on the view rotation sent by the character movement component
			// since it's in world space, so in this case we always send it ourselves.

			SetReplicatedViewRotation((MovementBase.Rotation.Inverse() * Super::GetViewRotation().Quaternion()).Rotator(), true);
		}
	}
	else
	{
		if (IsLocallyControlled() || (IsReplicatingMovement() && GetLocalRole() >= ROLE_Authority && IsValid(GetController())))
		{
			// The character movement component already sends the view rotation to the
			// server if movement is replicated, so we don't have to do this ourselves.

			SetReplicatedViewRotation(Super::GetViewRotation().GetNormalized(), !IsReplicatingMovement());
		}
	}

	RefreshViewNetworkSmoothing(DeltaTime);

	ViewState.Rotation = ViewState.NetworkSmoothing.CurrentRotation;

	// Set the yaw speed by comparing the current and previous view yaw angle, divided by
	// delta seconds. This represents the speed the camera is rotating from left to right.

	if (DeltaTime > UE_SMALL_NUMBER)
	{
		ViewState.YawSpeed = FMath::Abs(UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw - ViewState.PreviousYawAngle)) / DeltaTime;
	}
}

void AAlsCharacter::RefreshViewNetworkSmoothing(const float DeltaTime)
{
	// Based on UCharacterMovementComponent::SmoothClientPosition_Interpolate()
	// and UCharacterMovementComponent::SmoothClientPosition_UpdateVisuals().

	auto& NetworkSmoothing{ ViewState.NetworkSmoothing };

	if (!NetworkSmoothing.bEnabled ||
		NetworkSmoothing.ClientTime >= NetworkSmoothing.ServerTime ||
		NetworkSmoothing.Duration <= UE_SMALL_NUMBER ||
		(MovementBase.bHasRelativeRotation && IsNetMode(NM_ListenServer)))
	{
		// Can't use network smoothing on the listen server when the character
		// is standing on a rotating object, as it causes constant rotation jitter.

		NetworkSmoothing.InitialRotation = MovementBase.bHasRelativeRotation
			? (MovementBase.Rotation * ReplicatedViewRotation.Quaternion()).Rotator()
			: ReplicatedViewRotation;

		NetworkSmoothing.TargetRotation = NetworkSmoothing.InitialRotation;
		NetworkSmoothing.CurrentRotation = NetworkSmoothing.InitialRotation;

		return;
	}

	if (MovementBase.bHasRelativeRotation)
	{
		// Offset the rotations to keep them relative to the movement base.

		NetworkSmoothing.InitialRotation.Pitch += MovementBase.DeltaRotation.Pitch;
		NetworkSmoothing.InitialRotation.Yaw += MovementBase.DeltaRotation.Yaw;
		NetworkSmoothing.InitialRotation.Normalize();

		NetworkSmoothing.TargetRotation.Pitch += MovementBase.DeltaRotation.Pitch;
		NetworkSmoothing.TargetRotation.Yaw += MovementBase.DeltaRotation.Yaw;
		NetworkSmoothing.TargetRotation.Normalize();

		NetworkSmoothing.CurrentRotation.Pitch += MovementBase.DeltaRotation.Pitch;
		NetworkSmoothing.CurrentRotation.Yaw += MovementBase.DeltaRotation.Yaw;
		NetworkSmoothing.CurrentRotation.Normalize();
	}

	NetworkSmoothing.ClientTime += DeltaTime;

	const auto InterpolationAmount{
		UAlsMath::Clamp01(1.0f - (NetworkSmoothing.ServerTime - NetworkSmoothing.ClientTime) / NetworkSmoothing.Duration)
	};

	if (!FAnimWeight::IsFullWeight(InterpolationAmount))
	{
		NetworkSmoothing.CurrentRotation = UAlsMath::LerpRotator(NetworkSmoothing.InitialRotation,
			NetworkSmoothing.TargetRotation,
			InterpolationAmount);
	}
	else
	{
		NetworkSmoothing.ClientTime = NetworkSmoothing.ServerTime;
		NetworkSmoothing.CurrentRotation = NetworkSmoothing.TargetRotation;
	}
}

void AAlsCharacter::SetDesiredVelocityYawAngle(const float NewDesiredVelocityYawAngle)
{
	COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, DesiredVelocityYawAngle, NewDesiredVelocityYawAngle, this);
}

void AAlsCharacter::RefreshLocomotionLocationAndRotation()
{
	const auto& ActorTransform{ GetActorTransform() };

	// If network smoothing is disabled, then return regular actor transform.

	if (GetCharacterMovement()->NetworkSmoothingMode == ENetworkSmoothingMode::Disabled)
	{
		LocomotionState.Location = ActorTransform.GetLocation();
		LocomotionState.RotationQuaternion = ActorTransform.GetRotation();
		LocomotionState.Rotation = GetActorRotation();
	}
	else if (GetMesh()->IsUsingAbsoluteRotation())
	{
		LocomotionState.Location = ActorTransform.TransformPosition(GetMesh()->GetRelativeLocation() - GetBaseTranslationOffset());
		LocomotionState.RotationQuaternion = ActorTransform.GetRotation();
		LocomotionState.Rotation = GetActorRotation();
	}
	else
	{
		const auto SmoothTransform{
			ActorTransform * FTransform{
				GetMesh()->GetRelativeRotationCache().RotatorToQuat_ReadOnly(
					GetMesh()->GetRelativeRotation()) * GetBaseRotationOffset().Inverse(),
				GetMesh()->GetRelativeLocation() - GetBaseTranslationOffset()
			}
		};

		LocomotionState.Location = SmoothTransform.GetLocation();
		LocomotionState.RotationQuaternion = SmoothTransform.GetRotation();
		LocomotionState.Rotation = LocomotionState.RotationQuaternion.Rotator();
	}
}

void AAlsCharacter::RefreshLocomotionEarly()
{
	if (MovementBase.bHasRelativeRotation)
	{
		// Offset the rotations (the actor's rotation too) to keep them relative to the movement base.

		LocomotionState.TargetYawAngle = FRotator3f::NormalizeAxis(UE_REAL_TO_FLOAT(
			LocomotionState.TargetYawAngle + MovementBase.DeltaRotation.Yaw));

		LocomotionState.ViewRelativeTargetYawAngle = FRotator3f::NormalizeAxis(UE_REAL_TO_FLOAT(
			LocomotionState.ViewRelativeTargetYawAngle + MovementBase.DeltaRotation.Yaw));

		LocomotionState.SmoothTargetYawAngle = FRotator3f::NormalizeAxis(UE_REAL_TO_FLOAT(
			LocomotionState.SmoothTargetYawAngle + MovementBase.DeltaRotation.Yaw));

		auto NewRotation{ GetActorRotation() };
		NewRotation.Pitch += MovementBase.DeltaRotation.Pitch;
		NewRotation.Yaw += MovementBase.DeltaRotation.Yaw;
		NewRotation.Normalize();

		SetActorRotation(NewRotation);
	}

	RefreshLocomotionLocationAndRotation();

	LocomotionState.PreviousVelocity = LocomotionState.Velocity;
	LocomotionState.PreviousYawAngle = UE_REAL_TO_FLOAT(LocomotionState.Rotation.Yaw);
}

void AAlsCharacter::RefreshLocomotion(const float DeltaTime)
{
	LocomotionState.Velocity = GetVelocity();

	// Determine if the character is moving by getting its speed. The speed equals the length
	// of the horizontal velocity, so it does not take vertical movement into account. If the
	// character is moving, update the last velocity rotation. This value is saved because it might
	// be useful to know the last orientation of a movement even after the character has stopped.

	LocomotionState.Speed = UE_REAL_TO_FLOAT(LocomotionState.Velocity.Size2D());

	static constexpr auto HasSpeedThreshold{ 1.0f };

	LocomotionState.bHasSpeed = LocomotionState.Speed >= HasSpeedThreshold;

	if (LocomotionState.bHasSpeed)
	{
		LocomotionState.VelocityYawAngle = UE_REAL_TO_FLOAT(UAlsMath::DirectionToAngleXY(LocomotionState.Velocity));
	}

	if (Settings->bRotateTowardsDesiredVelocityInVelocityDirectionRotationMode && GetLocalRole() >= ROLE_AutonomousProxy)
	{
		FVector DesiredVelocity;

		SetDesiredVelocityYawAngle(AlsCharacterMovement->TryConsumePrePenetrationAdjustmentVelocity(DesiredVelocity) &&
			DesiredVelocity.Size2D() >= HasSpeedThreshold
			? UE_REAL_TO_FLOAT(UAlsMath::DirectionToAngleXY(DesiredVelocity))
			: LocomotionState.VelocityYawAngle);
	}

	if (DeltaTime > UE_SMALL_NUMBER)
	{
		LocomotionState.Acceleration = (LocomotionState.Velocity - LocomotionState.PreviousVelocity) / DeltaTime;
	}

	// Character is moving if has speed and current acceleration, or if the speed is greater than the moving speed threshold.

	LocomotionState.bMoving = (LocomotionState.bHasInput && LocomotionState.bHasSpeed) ||
		LocomotionState.Speed > Settings->MovingSpeedThreshold;
}

void AAlsCharacter::RefreshLocomotionLate(const float DeltaTime)
{
	if (!LocomotionMode.IsValid() || LocomotionAction.IsValid())
	{
		RefreshLocomotionLocationAndRotation();
		RefreshTargetYawAngleUsingLocomotionRotation();
	}

	if (DeltaTime > UE_SMALL_NUMBER)
	{
		LocomotionState.YawSpeed = FRotator3f::NormalizeAxis(UE_REAL_TO_FLOAT(
			LocomotionState.Rotation.Yaw - LocomotionState.PreviousYawAngle)) / DeltaTime;
	}
}

void AAlsCharacter::Jump()
{
	if (Stance == AlsStanceTags::Standing && !LocomotionAction.IsValid() &&
		LocomotionMode == AlsLocomotionModeTags::Grounded)
	{
		SetStamina(GetStamina() - JumpStaminaCost);
		Super::Jump();
	}
}

void AAlsCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		OnJumpedNetworked();
	}
	else if (GetLocalRole() >= ROLE_Authority)
	{
		MulticastOnJumpedNetworked();
	}
}

void AAlsCharacter::MulticastOnJumpedNetworked_Implementation()
{
	if (GetLocalRole() != ROLE_AutonomousProxy)
	{
		OnJumpedNetworked();
	}
}

void AAlsCharacter::OnJumpedNetworked()
{
	if (AnimationInstance.IsValid())
	{
		AnimationInstance->Jump();
	}
}

void AAlsCharacter::FaceRotation(const FRotator Rotation, const float DeltaTime)
{
	// Left empty intentionally. We are ignoring rotation changes from external
	// sources because ALS itself has full control over character rotation.
}

void AAlsCharacter::CharacterMovement_OnPhysicsRotation(const float DeltaTime)
{
	RefreshRollingPhysics(DeltaTime);
}

void AAlsCharacter::RefreshGroundedRotation(const float DeltaTime)
{
	if (LocomotionAction.IsValid() || LocomotionMode != AlsLocomotionModeTags::Grounded)
	{
		return;
	}

	if (HasAnyRootMotion())
	{
		RefreshTargetYawAngleUsingLocomotionRotation();
		return;
	}

	if (!LocomotionState.bMoving)
	{
		// Not moving.

		ApplyRotationYawSpeedAnimationCurve(DeltaTime);

		if (RefreshCustomGroundedNotMovingRotation(DeltaTime))
		{
			return;
		}

		if (RotationMode == AlsRotationModeTags::VelocityDirection)
		{
			// Rotate to the last target yaw angle when not moving (relative to the movement base or not).

			const auto TargetYawAngle{
				MovementBase.bHasRelativeLocation && !MovementBase.bHasRelativeRotation &&
				Settings->bInheritMovementBaseRotationInVelocityDirectionRotationMode
					? UE_REAL_TO_FLOAT(LocomotionState.TargetYawAngle + MovementBase.DeltaRotation.Yaw)
					: LocomotionState.TargetYawAngle
			};

			static constexpr auto RotationInterpolationSpeed{ 12.0f };
			static constexpr auto TargetYawAngleRotationSpeed{ 800.0f };

			RefreshRotationExtraSmooth(TargetYawAngle, DeltaTime, RotationInterpolationSpeed, TargetYawAngleRotationSpeed);
			return;
		}

		if (RotationMode == AlsRotationModeTags::Aiming || ViewMode == AlsViewModeTags::FirstPerson)
		{
			RefreshGroundedAimingRotation(DeltaTime);
			return;
		}

		RefreshTargetYawAngleUsingLocomotionRotation();
		return;
	}

	// Moving.

	if (RefreshCustomGroundedMovingRotation(DeltaTime))
	{
		return;
	}

	if (RotationMode == AlsRotationModeTags::VelocityDirection &&
		(LocomotionState.bHasInput || !LocomotionState.bRotationTowardsLastInputDirectionBlocked))
	{
		LocomotionState.bRotationTowardsLastInputDirectionBlocked = false;

		static constexpr auto TargetYawAngleRotationSpeed{ 800.0f };

		RefreshRotationExtraSmooth(
			Settings->bRotateTowardsDesiredVelocityInVelocityDirectionRotationMode
			? DesiredVelocityYawAngle
			: LocomotionState.VelocityYawAngle,
			DeltaTime, CalculateGroundedMovingRotationInterpolationSpeed(), TargetYawAngleRotationSpeed);
		return;
	}

	if (RotationMode == AlsRotationModeTags::ViewDirection)
	{
		const auto TargetYawAngle{
			Gait == AlsGaitTags::Sprinting
				? LocomotionState.VelocityYawAngle
				: UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw +
					GetMesh()->GetAnimInstance()->GetCurveValue(UAlsConstants::RotationYawOffsetCurveName()))
		};

		static constexpr auto TargetYawAngleRotationSpeed{ 500.0f };

		RefreshRotationExtraSmooth(TargetYawAngle, DeltaTime, CalculateGroundedMovingRotationInterpolationSpeed(),
			TargetYawAngleRotationSpeed);
		return;
	}

	if (RotationMode == AlsRotationModeTags::Aiming)
	{
		RefreshGroundedAimingRotation(DeltaTime);
		return;
	}

	RefreshTargetYawAngleUsingLocomotionRotation();
}

bool AAlsCharacter::RefreshCustomGroundedMovingRotation(const float DeltaTime)
{
	return false;
}

bool AAlsCharacter::RefreshCustomGroundedNotMovingRotation(const float DeltaTime)
{
	return false;
}

void AAlsCharacter::RefreshGroundedAimingRotation(const float DeltaTime)
{
	if (!LocomotionState.bHasInput && !LocomotionState.bMoving)
	{
		// Not moving.

		if (RefreshConstrainedAimingRotation(DeltaTime, true))
		{
			return;
		}

		RefreshTargetYawAngleUsingLocomotionRotation();
		return;
	}

	// Moving.

	if (RefreshConstrainedAimingRotation(DeltaTime))
	{
		return;
	}

	static constexpr auto RotationInterpolationSpeed{ 20.0f };
	static constexpr auto TargetYawAngleRotationSpeed{ 1000.0f };

	RefreshRotationExtraSmooth(UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw), DeltaTime,
		RotationInterpolationSpeed, TargetYawAngleRotationSpeed);
}

bool AAlsCharacter::RefreshConstrainedAimingRotation(const float DeltaTime, const bool bApplySecondaryConstraint)
{
	// Limit the character's rotation when aiming to prevent situations where the lower body noticeably
	// fails to keep up with the rotation of the upper body when the camera is rotating very fast.

	static constexpr auto ViewYawSpeedThreshold{ 620.0f };

	const auto bApplyPrimaryConstraint{ ViewState.YawSpeed > ViewYawSpeedThreshold };

	if (!bApplyPrimaryConstraint && !bApplySecondaryConstraint)
	{
		return false;
	}

	auto ViewRelativeYawAngle{ FRotator3f::NormalizeAxis(UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw - LocomotionState.Rotation.Yaw)) };
	static constexpr auto ViewRelativeYawAngleThreshold{ 70.0f };

	if (FMath::Abs(ViewRelativeYawAngle) <= ViewRelativeYawAngleThreshold + UE_KINDA_SMALL_NUMBER)
	{
		return false;
	}

	ViewRelativeYawAngle = UAlsMath::RemapAngleForCounterClockwiseRotation(ViewRelativeYawAngle);

	const auto TargetYawAngle{
		UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw +
			(ViewRelativeYawAngle >= 0.0f ? -ViewRelativeYawAngleThreshold : ViewRelativeYawAngleThreshold))
	};

	// Primary constraint. Prevents the character from rotating past a certain angle when the camera rotation speed is very high.

	if (bApplyPrimaryConstraint)
	{
		RefreshRotationInstant(TargetYawAngle);
		return true;
	}

	// Secondary constraint. Simply increases the character's rotation speed. Typically only used when the character is standing still.

	if (bApplySecondaryConstraint)
	{
		static constexpr auto RotationInterpolationSpeed{ 20.0f };

		RefreshRotation(TargetYawAngle, DeltaTime, RotationInterpolationSpeed);
		return true;
	}

	return false;
}

float AAlsCharacter::CalculateGroundedMovingRotationInterpolationSpeed() const
{
	// Calculate the rotation speed by using the rotation speed curve in the movement gait settings. Using
	// the curve in conjunction with the gait amount gives you a high level of control over the rotation
	// rates for each speed. Increase the speed if the camera is rotating quickly for more responsive rotation.

	const auto* InterpolationSpeedCurve{ AlsCharacterMovement->GetGaitSettings().RotationInterpolationSpeedCurve.Get() };

	static constexpr auto DefaultInterpolationSpeed{ 5.0f };

	const auto InterpolationSpeed{
		ALS_ENSURE(IsValid(InterpolationSpeedCurve))
			? InterpolationSpeedCurve->GetFloatValue(AlsCharacterMovement->CalculateGaitAmount())
			: DefaultInterpolationSpeed
	};

	static constexpr auto MaxInterpolationSpeedMultiplier{ 3.0f };
	static constexpr auto ReferenceViewYawSpeed{ 300.0f };

	return InterpolationSpeed * UAlsMath::LerpClamped(1.0f, MaxInterpolationSpeedMultiplier, ViewState.YawSpeed / ReferenceViewYawSpeed);
}

void AAlsCharacter::ApplyRotationYawSpeedAnimationCurve(const float DeltaTime)
{
	const auto DeltaYawAngle{ GetMesh()->GetAnimInstance()->GetCurveValue(UAlsConstants::RotationYawSpeedCurveName()) * DeltaTime };
	if (FMath::Abs(DeltaYawAngle) > UE_SMALL_NUMBER)
	{
		auto NewRotation{ GetActorRotation() };
		NewRotation.Yaw += DeltaYawAngle;

		SetActorRotation(NewRotation);

		RefreshLocomotionLocationAndRotation();
		RefreshTargetYawAngleUsingLocomotionRotation();
	}
}

void AAlsCharacter::RefreshInAirRotation(const float DeltaTime)
{
	if (LocomotionAction.IsValid() || LocomotionMode != AlsLocomotionModeTags::InAir)
	{
		return;
	}

	if (RefreshCustomInAirRotation(DeltaTime))
	{
		return;
	}

	static constexpr auto RotationInterpolationSpeed{ 5.0f };

	if (RotationMode == AlsRotationModeTags::VelocityDirection || RotationMode == AlsRotationModeTags::ViewDirection)
	{
		switch (Settings->InAirRotationMode)
		{
		case EAlsInAirRotationMode::RotateToVelocityOnJump:
			if (LocomotionState.bMoving)
			{
				RefreshRotation(LocomotionState.VelocityYawAngle, DeltaTime, RotationInterpolationSpeed);
			}
			else
			{
				RefreshTargetYawAngleUsingLocomotionRotation();
			}
			break;

		case EAlsInAirRotationMode::KeepRelativeRotation:
			RefreshRotation(UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw - LocomotionState.ViewRelativeTargetYawAngle),
				DeltaTime, RotationInterpolationSpeed);
			break;

		default:
			RefreshTargetYawAngleUsingLocomotionRotation();
			break;
		}
	}
	else if (RotationMode == AlsRotationModeTags::Aiming)
	{
		RefreshInAirAimingRotation(DeltaTime);
	}
	else
	{
		RefreshTargetYawAngleUsingLocomotionRotation();
	}
}

bool AAlsCharacter::RefreshCustomInAirRotation(const float DeltaTime)
{
	return false;
}

void AAlsCharacter::RefreshInAirAimingRotation(const float DeltaTime)
{
	if (RefreshConstrainedAimingRotation(DeltaTime))
	{
		return;
	}

	static constexpr auto RotationInterpolationSpeed{ 15.0f };

	RefreshRotation(UE_REAL_TO_FLOAT(ViewState.Rotation.Yaw), DeltaTime, RotationInterpolationSpeed);
}

void AAlsCharacter::RefreshRotation(const float TargetYawAngle, const float DeltaTime, const float RotationInterpolationSpeed)
{
	RefreshTargetYawAngle(TargetYawAngle);

	auto NewRotation{ GetActorRotation() };
	NewRotation.Yaw = UAlsMath::ExponentialDecayAngle(UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(NewRotation.Yaw)),
		TargetYawAngle, DeltaTime, RotationInterpolationSpeed);

	SetActorRotation(NewRotation);

	RefreshLocomotionLocationAndRotation();
}

void AAlsCharacter::RefreshRotationExtraSmooth(const float TargetYawAngle, const float DeltaTime,
	const float RotationInterpolationSpeed, const float TargetYawAngleRotationSpeed)
{
	LocomotionState.TargetYawAngle = FRotator3f::NormalizeAxis(TargetYawAngle);

	RefreshViewRelativeTargetYawAngle();

	// Interpolate target yaw angle for extra smooth rotation.

	LocomotionState.SmoothTargetYawAngle = UAlsMath::InterpolateAngleConstant(LocomotionState.SmoothTargetYawAngle,
		LocomotionState.TargetYawAngle,
		DeltaTime, TargetYawAngleRotationSpeed);

	auto NewRotation{ GetActorRotation() };
	NewRotation.Yaw = UAlsMath::ExponentialDecayAngle(UE_REAL_TO_FLOAT(FRotator::NormalizeAxis(NewRotation.Yaw)),
		LocomotionState.SmoothTargetYawAngle, DeltaTime, RotationInterpolationSpeed);

	SetActorRotation(NewRotation);

	RefreshLocomotionLocationAndRotation();
}

void AAlsCharacter::RefreshRotationInstant(const float TargetYawAngle, const ETeleportType Teleport)
{
	RefreshTargetYawAngle(TargetYawAngle);

	auto NewRotation{ GetActorRotation() };
	NewRotation.Yaw = TargetYawAngle;

	SetActorRotation(NewRotation, Teleport);

	RefreshLocomotionLocationAndRotation();
}

void AAlsCharacter::RefreshTargetYawAngleUsingLocomotionRotation()
{
	RefreshTargetYawAngle(UE_REAL_TO_FLOAT(LocomotionState.Rotation.Yaw));
}

void AAlsCharacter::RefreshTargetYawAngle(const float TargetYawAngle)
{
	LocomotionState.TargetYawAngle = FRotator3f::NormalizeAxis(TargetYawAngle);

	RefreshViewRelativeTargetYawAngle();

	LocomotionState.SmoothTargetYawAngle = LocomotionState.TargetYawAngle;
}

void AAlsCharacter::RefreshViewRelativeTargetYawAngle()
{
	LocomotionState.ViewRelativeTargetYawAngle = FRotator3f::NormalizeAxis(UE_REAL_TO_FLOAT(
		ViewState.Rotation.Yaw - LocomotionState.TargetYawAngle));
}

// Attributes
float AAlsCharacter::GetMaxHealth()
{
	return MaxHealth;
}

float AAlsCharacter::GetHealth()
{
	return Health;
}

float AAlsCharacter::GetMaxStamina()
{
	return MaxStamina;
}

float AAlsCharacter::GetStamina()
{
	return Stamina;
}

float AAlsCharacter::GetMaxStrength()
{
	return MaxStrength;
}

float AAlsCharacter::GetStrength()
{
	return Strength;
}

float AAlsCharacter::GetMaxEndurance()
{
	return MaxEndurance;
}

float AAlsCharacter::GetEndurance()
{
	return Endurance;
}

float AAlsCharacter::GetMaxVitality()
{
	return MaxVitality;
}

float AAlsCharacter::GetVitality()
{
	return Vitality;
}

float AAlsCharacter::GetMaxAgility()
{
	return MaxAgility;
}

float AAlsCharacter::GetAgility()
{
	return Agility;
}

float AAlsCharacter::GetMaxDexterity()
{
	return MaxDexterity;
}

float AAlsCharacter::GetDexterity()
{
	return Dexterity;
}

float AAlsCharacter::GetMaxPerception()
{
	return MaxPerception;
}

float AAlsCharacter::GetPerception()
{
	return Perception;
}

float AAlsCharacter::GetArmour()
{
	return Armour;
}

void AAlsCharacter::SetMaxHealth(float NewMaxHealth)
{
	MaxHealth = NewMaxHealth;
}

void AAlsCharacter::SetHealth(float NewHealth)
{
	float HealthDiff = GetHealth() - NewHealth;

	if (HealthDiff > 0)
	{
		HealthDiff *= HealthLossRate;

		if (bShouldReduceStamina)
		{
			if (GetStamina() - HealthDiff < 0)
			{
				HealthDiff -= GetStamina();
				SetStamina(0.0f);
			}
			else
			{
				SetStamina(GetStamina() - HealthDiff);
				HealthDiff = 0.0f;
			}
		}

		if (bShouldConvertDamageToStamina_30)
		{
			if (GetStamina() - HealthDiff * 0.3f < 0)
			{
				HealthDiff -= GetStamina();
				SetStamina(0.0f);
			}
			else
			{
				SetStamina(GetStamina() - HealthDiff * 0.3f);
				HealthDiff *= 0.7f;
			}
		}
	}
	CalculateDamageSlowdownDuration(NewHealth);

	Health = FMath::Clamp(GetHealth() - HealthDiff, 0.0f, GetMaxHealth());

	RefreshStaminaAndRecoilIfHealthIsUnder_20();
	CheckForHealthReplenish();
	CheckIfHealthIsUnder_20();
	ShouldIgnoreStunIfHealthIsUnder_50();
	CheckIfHealthIsUnder_30();

	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void AAlsCharacter::SetMaxStamina(float NewMaxStamina)
{
	MaxStamina = NewMaxStamina;
}

void AAlsCharacter::SetStamina(float NewStamina)
{
	float StaminaDiff = GetStamina() - NewStamina;
	if (StaminaDiff > 0)
	{
		StaminaDiff *= StaminaLossRate;
	}
	Stamina = FMath::Clamp(GetStamina() - StaminaDiff, 0.0f, GetMaxStamina());

	RefreshHealthIfStaminaIsUnder_30();
	CheckIfStaminaIsUnder_70();

	OnStaminaChanged.Broadcast(Stamina, MaxStamina);
}

void AAlsCharacter::SetMaxStrength(float NewMaxStrength)
{
	MaxStrength = NewMaxStrength;
}

void AAlsCharacter::SetStrength(float NewStrength)
{
	Strength = FMath::Clamp(NewStrength, 0.0f, GetMaxStrength());
	OnStrengthChanged.Broadcast(Strength, MaxStrength);
}

void AAlsCharacter::SetMaxEndurance(float NewMaxEndurance)
{
	MaxEndurance = NewMaxEndurance;
}

void AAlsCharacter::SetEndurance(float NewEndurance)
{
	Endurance = FMath::Clamp(NewEndurance, 0.0f, GetMaxEndurance());
	OnEnduranceChanged.Broadcast(Endurance, MaxEndurance);
}

void AAlsCharacter::SetMaxVitality(float NewMaxVitality)
{
	MaxVitality = NewMaxVitality;
}

void AAlsCharacter::SetVitality(float NewVitality)
{
	Vitality = FMath::Clamp(NewVitality, 0.0f, GetMaxVitality());
	OnVitalityChanged.Broadcast(Vitality, MaxVitality);
}

void AAlsCharacter::SetMaxAgility(float NewMaxAgility)
{
	MaxAgility = NewMaxAgility;
}

void AAlsCharacter::SetAgility(float NewAgility)
{
	Agility = FMath::Clamp(NewAgility, 0.0f, GetMaxAgility());
	OnAgilityChanged.Broadcast(Agility, MaxAgility);
}

void AAlsCharacter::SetMaxDexterity(float NewMaxDexterity)
{
	MaxDexterity = NewMaxDexterity;
}

void AAlsCharacter::SetDexterity(float NewDexterity)
{
	Dexterity = FMath::Clamp(NewDexterity, 0.0f, GetMaxDexterity());
	OnDexterityChanged.Broadcast(Dexterity, MaxDexterity);
}

void AAlsCharacter::SetMaxPerception(float NewMaxPerception)
{
	MaxPerception = NewMaxPerception;
}

void AAlsCharacter::SetPerception(float NewPerception)
{
	Perception = FMath::Clamp(NewPerception, 0.0f, GetMaxPerception());
	OnPerceptionChanged.Broadcast(Perception, MaxPerception);
}

void AAlsCharacter::SetArmour(float NewArmour)
{
	Armour = FMath::Max(NewArmour, 0.0f);
	OnArmourChanged.Broadcast(Armour);
}

void AAlsCharacter::HealthRecovery()
{
	if (GetHealth() <= 33.0f)
	{
		SetHealth(FMath::Clamp(GetHealth() + GetWorld()->GetDeltaSeconds() * 0.25f * HealthRecoveryRate_50 * StaminaHealthStandingMultiplier * StaminaHealthRunningMultiplier * HealthRecoveryRateValue_12, 0.0f, 33.0f));
	}
	else if (GetHealth() <= 67.0f)
	{
		SetHealth(FMath::Clamp(GetHealth() + GetWorld()->GetDeltaSeconds() * 0.25f * HealthRecoveryRate_50 * StaminaHealthStandingMultiplier * StaminaHealthRunningMultiplier * HealthRecoveryRateValue_12, 0.0f, 67.0f));
	}
	else if (GetHealth() <= 100.0f)
	{
		SetHealth(FMath::Clamp(GetHealth() + GetWorld()->GetDeltaSeconds() * 0.25f * HealthRecoveryRate_50 * StaminaHealthStandingMultiplier * StaminaHealthRunningMultiplier * HealthRecoveryRateValue_12, 0.0f, 100.0f));
	}
}

void AAlsCharacter::StaminaRecovery()
{
	SetStamina(GetStamina() + StaminaRegenerationRate * StaminaRecoveryRate_50 * StaminaHealthStandingMultiplier * StaminaHealthRunningMultiplier * StaminaRegenerationRateValue_11);
}

void AAlsCharacter::RefreshRecoil()
{
	RecoilMultiplier = RecoilMultiplier_1 * RecoilMultiplierValue_11;
}

void AAlsCharacter::CalculateBackwardAndStrafeMoveReducement()
{
	float MovementDirection = UKismetMathLibrary::Dot_VectorVector(GetVelocity().GetSafeNormal(), GetActorRotation().Vector().GetSafeNormal());

	// The less health left the slower movement
	float DamageMovementPenalty = FMath::Clamp(GetHealth() / GetMaxHealth(), 1.0f - HealthMovementPenalty_01, 1.0f);

	SpeedMultiplier = FMath::GetMappedRangeValueClamped(FVector2D(-1.0f, 1.0f), FVector2D(MovementBackwardSpeedMultiplier, 1.0f), MovementDirection);

	// Final speed depends on  weapon weight, health left, damage got, surface slope angle and wind.
	SpeedMultiplier *= (1 - WeaponMovementPenalty) * DamageMovementPenalty * DamageSlowdownMultiplier * SurfaceSlopeEffectMultiplier * WindIfluenceEffect0_2 * StunRecoveryMultiplier * StickyMultiplier * StickyStuckMultiplier
		* ShockSpeedMultiplier * Slowdown_01Range * WireEffectPower_01Range * GrappleEffectSpeedMultiplier * MagneticEffectSpeedMultiplier * ConcatenationEffectSpeedMultiplier * StaticGrenadeEffect * WeightMultiplier
		* LastStandSpeedMultiplier * WalkAndRunSpeedMultiplier_15 * WalkRunSpeedMultiplier_25 * SpeedMultiplierIfStaminaLess_70;

	if (abs(PrevSpeedMultiplier - SpeedMultiplier) > 0.0001f)
	{
		AlsCharacterMovement->MovementSpeedMultiplier = SpeedMultiplier;
		AlsCharacterMovement->RefreshMaxWalkSpeed();
	}
	PrevSpeedMultiplier = SpeedMultiplier;
}

void AAlsCharacter::CalculateFallDistanceToCountStunAndDamage()
{
	if (bShouldIgnoreFallDamageAndStun)
	{
		return;
	}

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);
	FHitResult HitResult;

	bool bIsHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), GetActorLocation(),
		GetActorLocation() + FVector(0.0f, 0.0f, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 20.0f), ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::None, HitResult, true);

	if (!bIsHit)
	{
		ZLocation = GetActorLocation().Z;
		if (PrevZLocation)
		{
			FallDistanceToCountStunAndDamage += (PrevZLocation - ZLocation) > 0 ? PrevZLocation - ZLocation : 0.0f;
		}
		PrevZLocation = ZLocation;
	}
	else
	{
		if (FallDistanceToCountStunAndDamage > MinFallHeightWithoutDamageAndStun)
		{
			FallDamage = (FallDistanceToCountStunAndDamage - MinFallHeightWithoutDamageAndStun) / 10.0f;
			StunTime = (FallDistanceToCountStunAndDamage - MinFallHeightWithoutDamageAndStun) / 100.0f;
			UGameplayStatics::ApplyDamage(this, FallDamage, GetController(), this, nullptr);
			StunEffect(StunTime);
		}

		FallDistanceToCountStunAndDamage = 0.0f;
		FallDamage = 0.0f;
		StunTime = 0.0f;
		PrevZLocation = 0.0f;
		ZLocation = 0.0f;
	}
}

void AAlsCharacter::StunEffect(float Time)
{
	if (bShouldIgnoreStun || ShouldIgnoreEnemyAbilityEffect() || ShouldIgnoreStunIfHealthIsUnder_50())
	{
		return;
	}

	float StunTimeLocal = Time;
	if (bIsStunned)
	{
		StunTimeLocal += GetWorldTimerManager().GetTimerRemaining(StunTimerHandle);
	}
	FTimerDelegate StunDelegate;
	StunDelegate.BindLambda([this]()
		{
			bIsStunned = false;
			StopRagdolling();
		});

	GetWorldTimerManager().ClearTimer(StunTimerHandle);
	if (StunTimeLocal > 0)
	{
		bIsStunned = true;
		StunRecoveryMultiplier = 0.1f;
		GetWorldTimerManager().SetTimer(StunTimerHandle, StunDelegate, StunTimeLocal, false);
	}
}

void AAlsCharacter::StunRecovery()
{
	if (!bIsStunned && StunRecoveryMultiplier < 1.0f)
	{
		StunRecoveryMultiplier += GetWorld()->GetDeltaSeconds() / StunRecoveryTime;
	}
	StunRecoveryMultiplier = UKismetMathLibrary::FClamp(StunRecoveryMultiplier, 0.1f, 1.0f);
}

void AAlsCharacter::CalculateDamageSlowdownDuration(float NewHealth)
{
	float DeltaHealth = GetHealth() - NewHealth;
	if (DeltaHealth <= 0)
	{
		return;
	}

	float DamageSlowdownDuration = DeltaHealth / 10.0f * DamageSlowdownTime;
	DamageSlowdownMultiplier -= (DeltaHealth / GetMaxHealth() * DamageSlowdownEffect);

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [&]() {DamageSlowdownMultiplier = 1.0f; }, DamageSlowdownDuration + 0.001f, false);
}

void AAlsCharacter::CalculateSpeedMultiplierOnGoingUpOrDown()
{
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);
	FHitResult PrevHitResult;
	FHitResult NextHitResult;
	FVector PrevLineTraceStart = GetActorLocation() + GetVelocity().GetSafeNormal() * FVector(30.0f, 30.0f, 0.0);
	FVector PrevLineTraceEnd = PrevLineTraceStart + FVector(0.0f, 0.0f, -200.0f);
	FVector NextLineTraceStart = GetActorLocation() + GetVelocity().GetSafeNormal() * FVector(40.0f, 40.0f, 0.0);
	FVector NextLineTraceEnd = NextLineTraceStart + FVector(0.0f, 0.0f, -200.0f);

	bool bIsHitPrev = UKismetSystemLibrary::LineTraceSingle(GetWorld(), PrevLineTraceStart, PrevLineTraceEnd, ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::None, PrevHitResult, true);
	bool bIsHitNext = UKismetSystemLibrary::LineTraceSingle(GetWorld(), NextLineTraceStart, NextLineTraceEnd, ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::None, NextHitResult, true);

	if (bIsHitPrev && bIsHitNext)
	{
		float DeltaTilt = FMath::GetMappedRangeValueClamped(FVector2D(-10.0f, 10.0f), FVector2D(-0.9f, 0.9f), NextHitResult.Location.Z - PrevHitResult.Location.Z);
		SurfaceSlopeEffectMultiplier = FMath::FInterpTo(SurfaceSlopeEffectMultiplier, 1 - DeltaTilt * SurfaceSlopeEffect, GetWorld()->DeltaTimeSeconds, 1.0f);
	}
}

bool AAlsCharacter::SwitcherForSlidingLogic_OnSurfaceFriction()
{
	if (!SlidingTurnOnOff)
	{
		return false;
	}

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);
	FHitResult HitResult;
	FVector LineTraceStart = GetActorLocation() + GetVelocity().GetSafeNormal() * FVector(30.0f, 30.0f, 0.0);
	FVector LineTraceEnd = LineTraceStart + FVector(0.0f, 0.0f, -200.0f);

	bool bIsHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), LineTraceStart, LineTraceEnd, ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::None, HitResult, true);

	if (bIsHit)
	{
		SurfacePhysicFriction = HitResult.PhysMaterial->Friction;
		if (SurfacePhysicFriction < 0.7f)
		{
			return true;
		}
	}

	bIsSliding = false;
	return false;
}

void AAlsCharacter::CalculateStartStopSliding()
{
	if (!SwitcherForSlidingLogic_OnSurfaceFriction())
	{
		return;
	}

	PrevControlRotation = CurrentControlRotation;
	CurrentControlRotation = GetControlRotation();
	float DeltaControlRotation = abs(PrevControlRotation.Yaw - CurrentControlRotation.Yaw);

	PrevVelocity2D = CurrentVelocity2D.GetSafeNormal();
	CurrentVelocity2D = FVector2D(GetVelocity()).GetSafeNormal();
	float DeltaChangeVelocityDirection = UKismetMathLibrary::DotProduct2D(PrevVelocity2D, CurrentVelocity2D);

	PrevVelocityLength2D = CurrentVelocityLength2D;
	CurrentVelocityLength2D = FVector2D(GetVelocity()).Length();
	float DeltaChangeVelocityLength = PrevVelocityLength2D - CurrentVelocityLength2D;

	if (!AlsCharacterMovement->IsFalling() && (((DeltaChangeVelocityDirection < 0.75f || DeltaControlRotation > 20.0f) && CurrentVelocityLength2D > 100.0f) || DeltaChangeVelocityLength > 10.0f))
	{
		bIsSliding = true;
	}

	if (bIsSliding && DeltaDistanceToGetToStopPoint > 0.0f)
	{
		AddActorWorldOffset(LastVelocity.GetSafeNormal() * DeltaDistanceToGetToStopPoint, true);
		LastVelocity = FMath::VInterpTo(LastVelocity, LastVelocityConsideringWind, GetWorld()->DeltaTimeSeconds, 3.0f);

		SlidingTime += GetWorld()->GetDeltaSeconds();
		DeltaDistanceToGetToStopPoint -= (FMath::Pow(SlidingTime, 2) + SurfacePhysicFriction);
		AlphaForLeanAnim = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 23.0f), FVector2D(0.0f, 1.0f), DeltaDistanceToGetToStopPoint);
	}
	else
	{
		bIsSliding = false;
		SlidingTime = 0.0f;
		LastVelocity = GetVelocity() * FVector(1.0f, 1.0f, 0.0f);
		LastVelocityConsideringWind = LastVelocity + FVector(WindDirectionAndSpeed.X, WindDirectionAndSpeed.Y, 0.0f) / 5.0f;
		LastGaitTag = GetGait();
		DeltaDistanceToGetToStopPoint = FVector2D(LastVelocityConsideringWind).Length() * GetWorld()->GetDeltaSeconds();
	}
}

void AAlsCharacter::SetWindDirection()
{
	TArray<AActor*> WindControllers;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("WindController"), WindControllers);

	if (WindControllers.Num() > 0)
	{
		AWindDirectionalSource* WindDirectionalSource = Cast<AWindDirectionalSource>(WindControllers[0]);
		WindDirectionAndSpeed = FVector2D(WindDirectionalSource->GetActorForwardVector() * WindDirectionalSource->GetComponent()->Speed);
	}
}

void AAlsCharacter::CalculateWindInfluenceOnFalling()
{
	if (AlsCharacterMovement->IsFalling())
	{
		AddActorWorldOffset(FVector(WindDirectionAndSpeed.X, WindDirectionAndSpeed.Y, 0.0f) / 1000.0f, true);
	}
}

void AAlsCharacter::CalculateWindInfluenceEffect()
{
	BackwardForward_WindAmount = UKismetMathLibrary::DotProduct2D(FVector2D(GetActorForwardVector().GetSafeNormal()), WindDirectionAndSpeed.GetSafeNormal());
	LeftRight_WindAmount = UKismetMathLibrary::DotProduct2D(FVector2D(GetActorRightVector().GetSafeNormal()), WindDirectionAndSpeed.GetSafeNormal());

	BackwardForward_WindAmount *= FMath::GetMappedRangeValueClamped(FVector2D(1000.0f, 2500.0f), FVector2D(0.0f, 1.0f), WindDirectionAndSpeed.Length());
	LeftRight_WindAmount *= FMath::GetMappedRangeValueClamped(FVector2D(1000.0f, 2500.0f), FVector2D(0.0f, 1.0f), WindDirectionAndSpeed.Length());

	WindIfluenceEffect0_2 = FMath::FInterpTo(WindIfluenceEffect0_2, 1 + UKismetMathLibrary::DotProduct2D(FVector2D(GetVelocity().GetSafeNormal()), WindDirectionAndSpeed.GetSafeNormal()) *
		FMath::GetMappedRangeValueClamped(FVector2D(1000.0f, 2500.0f), FVector2D(0.0f, 1.0f), WindDirectionAndSpeed.Length()), GetWorld()->GetDeltaSeconds(), 2.0f);
}

bool AAlsCharacter::IsStickySurface(FName Bone)
{
	if (bUsedMashToEscape)
	{
		bUsedMashToEscape = false;
		StickyMultiplier = 1.0f;
		bIsStickyStuck = false;
		SetDesiredGait(AlsGaitTags::Walking);
		return false;
	}

	if (GetMesh())
	{
		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(this);
		FHitResult HitResult;
		FVector LineTraceStart = GetMesh()->GetSocketLocation(Bone) + GetActorUpVector() * 10.0f;
		FVector LineTraceEnd = LineTraceStart + FVector(0.0f, 0.0f, -50.0f);

		bool bIsHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), LineTraceStart, LineTraceEnd, ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::None, HitResult, true);

		if (bIsHit && HitResult.PhysMaterial.IsValid())
		{
			switch (HitResult.PhysMaterial->SurfaceType)
			{
			case EPhysicalSurface::SurfaceType8:
			{
				bIsSticky = true;
				StickyMultiplier = UAlsMath::Clamp01(StickyMultiplier -= StickyStuckSpeed / 100.0f);
				bIsStickyStuck = !StickyMultiplier;
				SetDesiredGait(AlsGaitTags::Walking);
				return true;
			}
			default:
			{
				bIsSticky = false;
				bIsStickyStuck = false;
				StickyMultiplier = 1.0f;
				break;
			}
			}
		}
	}

	return false;
}

void AAlsCharacter::RemoveSticknessByMash()
{
	if (bTapInTime)
	{
		// check for directional keys - if pressed same direction key. If want use it just remove comment below
		//if (PrevInputDirection == LastInputDirection)
		{
			GetWorld()->GetTimerManager().ClearTimer(TapInTimeTimerHandle);
			--TapCounter;
			if (!TapCounter)
			{
				bUsedMashToEscape = true;
				bTapInTime = false;
				return;
			}
			GetWorld()->GetTimerManager().SetTimer(TapInTimeTimerHandle, [this]() {bTapInTime = false; }, TimeBetweenTaps, false);
		}
	}
	else
	{
		bTapInTime = true;
		TapCounter = HowManyTaps;
		GetWorld()->GetTimerManager().SetTimer(TapInTimeTimerHandle, [this]() {bTapInTime = false; }, TimeBetweenTaps, false);
	}

	PrevInputDirection = LastInputDirection;

}

void AAlsCharacter::SetArmLockEffect_Implementation(bool bIsSet, bool bShouldResetEffect)
{
}

void AAlsCharacter::StumbleEffect(FVector InstigatorLocation, float InstigatorPower)
{
	if (ShouldIgnoreEnemyAbilityEffect())
	{
		return;
	}

	float Power = FMath::Clamp(InstigatorPower - UKismetMathLibrary::Vector_Distance(InstigatorLocation, GetActorLocation()), 0.0f, 1000.0f);
	FVector Direction = (GetActorLocation() - InstigatorLocation).GetSafeNormal() * Power;
	Direction.Z = FMath::Clamp(Direction.Z, 0.0f, 1000.0f);
	LaunchCharacter(Direction, false, false);

	float Time = UKismetMathLibrary::MapRangeClamped(Power, 200.0f, 1000.0f, 0.0f, 5.0f);
	StunEffect(Time);
}

void AAlsCharacter::KnockdownEffect(FVector InstigatorLocation, float InfluenceRadius)
{
	if (ShouldIgnoreEnemyAbilityEffect() || CheckIfShouldIgnoreKnockdownEffect())
	{
		return;
	}

	if (UKismetMathLibrary::Vector_Distance(InstigatorLocation, GetActorLocation()) <= InfluenceRadius)
	{
		float Force = GetMesh()->IsSimulatingPhysics("pelvis") ? 200.0f : 5000.0f;
		StartRagdolling();
		GetMesh()->AddRadialImpulse(InstigatorLocation, InfluenceRadius, Force, ERadialImpulseFalloff::RIF_Constant, true);
		StunEffect(2.0f);
	}
}

void AAlsCharacter::ShockEffect()
{
	if (bIsShocked && !bShouldIgnoreJitterynessShockEffect)
	{
		// moving
		ShockSpeedMultiplier = 1.0f - UKismetMathLibrary::RandomFloatInRange(0.0f, ShockEffectPower_01Range);

		//side offset impulse
		if (!GetWorldTimerManager().IsTimerActive(LaunchTimerHandle))
		{
			GetWorldTimerManager().SetTimer(LaunchTimerHandle, [&]()
				{
					FVector LaunchDirection = UKismetMathLibrary::RandomUnitVector() * FVector(1.0f, 1.0f, 0.0f);
					float ForceToLaunch = 500.0f * ShockEffectPower_01Range;
					FTimerHandle VelocityTimerHandle;
					AlsCharacterMovement->Velocity = GetVelocity() + LaunchDirection * ForceToLaunch;
					GetWorldTimerManager().SetTimer(VelocityTimerHandle, [&]()
						{
							AlsCharacterMovement->Velocity = GetVelocity() - LaunchDirection * ForceToLaunch;
						}, 0.2f, false);
				}, UKismetMathLibrary::RandomFloatInRange(0.5f, 1.5f), false);
		}

		//camera trembling
		if (!GetWorldTimerManager().IsTimerActive(CameraTimerHandle))
		{
			GetWorldTimerManager().SetTimer(CameraTimerHandle, [&]()
				{
					CameraPitchOffset = UKismetMathLibrary::RandomFloatInRange(1.0f, 2.0f) * (FMath::RandBool() ? 1 : -1) * ShockEffectPower_01Range;
					CameraYawOffset = UKismetMathLibrary::RandomFloatInRange(1.0f, 2.0f) * (FMath::RandBool() ? 1 : -1) * ShockEffectPower_01Range;
				}, UKismetMathLibrary::RandomFloatInRange(0.5f, 1.5f), false);
		}
		else if (!GetWorldTimerManager().IsTimerActive(DiscreteTimerHandle))
		{
			GetWorldTimerManager().SetTimer(DiscreteTimerHandle, [this]()
				{
					AddControllerPitchInput(CameraPitchOffset);
					AddControllerYawInput(CameraYawOffset);
				}, UKismetMathLibrary::RandomFloatInRange(0.02f, 0.07f), false);
		}

		//camera rapid side move
		if (!GetWorldTimerManager().IsTimerActive(RapidTimerHandle))
		{
			GetWorldTimerManager().SetTimer(RapidTimerHandle, [&]()
				{
					RapidFinalDistance = UKismetMathLibrary::RandomFloatInRange(2.0f, 3.0f) * ShockEffectPower_01Range;
					RapidFinalDistanceTransition = 0.0f;
					float PitchOffset = UKismetMathLibrary::RandomFloatInRange(0.0f, 5.0f);
					CameraRapidPitchOffset = PitchOffset * (FMath::RandBool() ? 1 : -1) * ShockEffectPower_01Range;
					CameraRapidYawOffset = (5.0f - PitchOffset) * (FMath::RandBool() ? 1 : -1) * ShockEffectPower_01Range;
				}, UKismetMathLibrary::RandomFloatInRange(3.0f, 5.0f), false);
		}
		else
		{
			if (RapidFinalDistanceTransition < RapidFinalDistance)
			{
				++RapidFinalDistanceTransition;
				AddControllerPitchInput(CameraRapidPitchOffset);
				AddControllerYawInput(CameraRapidYawOffset);
			}
			else if (RapidFinalDistance > 0)
			{
				float ReturnSpeedMult = 0.5f;
				RapidFinalDistance -= ReturnSpeedMult;
				AddControllerPitchInput(-CameraRapidPitchOffset * ReturnSpeedMult);
				AddControllerYawInput(-CameraRapidYawOffset * ReturnSpeedMult);
			}
		}

		return;
	}
	ShockSpeedMultiplier = FMath::FInterpTo(ShockSpeedMultiplier, 1.0f, GetWorld()->GetDeltaSeconds(), 1.0f);
}

void AAlsCharacter::SetSlowedEffect(float SlowdownValue)
{
	if (ShouldIgnoreEnemyAbilityEffect())
	{
		Slowdown_01Range = 1.0f;
	}
	else
	{
		Slowdown_01Range = FMath::Clamp(SlowdownValue, 0.0f, 1.0f);
	}
}

void AAlsCharacter::DiscombobulateEffect()
{
	if (bIsDiscombobulated)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [&]()
			{
				float PitchOffset = UKismetMathLibrary::RandomFloatInRange(0.0f, 3.0f);
				TargetDiscombobulateCameraPitchOffset = PitchOffset * (FMath::RandBool() ? 1 : -1) * DiscombobulateEffectPower_01Range;
				TargetDiscombobulateCameraYawOffset = (3.0f - PitchOffset) * (FMath::RandBool() ? 1 : -1) * DiscombobulateEffectPower_01Range;
			}, UKismetMathLibrary::RandomFloatInRange(0.5f, 1.5f), false);

		CurrentDiscombobulateCameraPitchOffset = FMath::FInterpTo(CurrentDiscombobulateCameraPitchOffset, TargetDiscombobulateCameraPitchOffset, GetWorld()->GetDeltaSeconds(), 1.0f);
		CurrentDiscombobulateCameraYawOffset = FMath::FInterpTo(CurrentDiscombobulateCameraYawOffset, TargetDiscombobulateCameraYawOffset, GetWorld()->GetDeltaSeconds(), 1.0f);

		AddControllerPitchInput(CurrentDiscombobulateCameraPitchOffset);
		AddControllerYawInput(CurrentDiscombobulateCameraYawOffset);

		DiscombobulateTimeDelay = UKismetMathLibrary::MapRangeClamped(DiscombobulateEffectPower_01Range, 0.0f, 1.0f, 0.0001f, 2.0f);
	}
}

void AAlsCharacter::SetRemoveBlindness(bool IsSet)
{
	if (IsSet && !bShouldIgnoreBlindnessEffect && !ShouldIgnoreEnemyAbilityEffect())
	{
		if (!BlindnessWidget && BlindnessWidgetClass)
		{
			BlindnessWidget = CreateWidget<UBlindnessWidget>(GetWorld(), BlindnessWidgetClass);
			if (BlindnessWidget)
			{
				BlindnessWidget->AddToViewport(9);
			}
		}
		if (BlindnessWidget)
		{
			if (BlindnessEffectTimerHandle.IsValid())
			{
				GetWorldTimerManager().ClearTimer(BlindnessEffectTimerHandle);
			}
			BlindnessWidget->StopAnimation(BlindnessWidget->FadeOut);
		}
	}
	else if (BlindnessWidget && !BlindnessWidget->IsAnimationPlaying(BlindnessWidget->FadeOut))
	{
		BlindnessWidget->PlayAnimation(BlindnessWidget->FadeOut, 0.0f, 1, EUMGSequencePlayMode::Forward, 1.0f);
		GetWorldTimerManager().SetTimer(BlindnessEffectTimerHandle, [this]()
			{
				if (BlindnessWidget)
				{
					BlindnessWidget->RemoveFromParent();
					BlindnessWidget = nullptr;
				}
			}, BlindnessWidget->FadeOut->GetEndTime(), false);
	}
}

void AAlsCharacter::SetReverseEffect(bool IsSet)
{
	if (IsSet && !ShouldIgnoreEnemyAbilityEffect())
	{
		bIsInputReversed = true;
	}
	else
	{
		bIsInputReversed = false;
	}
}

void AAlsCharacter::SetRemoveWireEffect(bool bIsSet, float EffectPower)
{
	if (bIsSet && !ShouldIgnoreEnemyAbilityEffect())
	{
		bIsWired = true;
		WireEffectPower_01Range = FMath::Clamp(1 - EffectPower, 0.0f, 1.0f);
	}
	else
	{
		bIsWired = false;
		WireEffectPower_01Range = 1.0f;
	}
}

void AAlsCharacter::ShakeMouseRemoveEffect(FVector2D Value)
{
	CurrentMouseValueLength = Value.Length();
	if (CurrentMouseValueLength > 30.0f && PrevMouseValueLength > 30.0f && PrevPrevMouseValueLength < 10.0f)
	{
		SetRemoveWireEffect(false);
		if (bIsTwoKeysHold)
		{
			SetRemoveGrappleEffect(false);
		}
	}
	PrevPrevMouseValueLength = PrevMouseValueLength;
	PrevMouseValueLength = CurrentMouseValueLength;
}

void AAlsCharacter::PressTwoKeysRemoveGrappleEffect(bool bIsHold)
{
	bIsTwoKeysHold = bIsHold;
}

float AAlsCharacter::GetStaticGrenadeEffect() const
{
	float Effect = 0;
	TArray<AActor*> Keys;
	StasisGrenadeEffectMap.GetKeys(Keys);
	for (AActor* Key : Keys)
	{
		if (Key && IsValid(Key))
		{
			float EffectValue = StasisGrenadeEffectMap.FindRef(Key);

			if (EffectValue > Effect)
			{
				Effect = EffectValue;
			}
		}
	}

	return Effect;
}

void AAlsCharacter::SetRemoveGrappleEffect(bool bIsSet)
{
	static uint8 Counter = 0;
	if (bIsSet && !ShouldIgnoreEnemyAbilityEffect())
	{
		if (Counter < 3)
		{
			++Counter;
			bIsGrappled = true;
			GrappleEffectSpeedMultiplier -= 0.33f;
			SetDesiredGait(AlsGaitTags::Walking);
		}
	}
	else
	{
		Counter = 0;
		bIsGrappled = false;
		GrappleEffectSpeedMultiplier = 1.0f;
		SetDesiredGait(AlsGaitTags::Running);
	}
}

void AAlsCharacter::MagneticEffect()
{
	if (bIsMagnetic)
	{
		//movement speed influence
		FVector MagnetForceDirection = (MagnetLocation - GetMesh()->GetSocketLocation("hand_r")).GetSafeNormal();
		float DistanceToMagnet = FVector::Distance(GetMesh()->GetSocketLocation("hand_r"), MagnetLocation);
		float MagnetPowerOnDirection = FMath::GetMappedRangeValueClamped(FVector2D(-1.0f, 1.0f), FVector2D(-0.8f, 0.8f), UKismetMathLibrary::Dot_VectorVector(GetVelocity().GetSafeNormal(), MagnetForceDirection));
		float DistanceCoefficient = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, MagneticSphereRadius), FVector2D(MagneticEffectPower_Range01, 0.01f), DistanceToMagnet);
		MagneticEffectSpeedMultiplier = 1 + MagnetPowerOnDirection * DistanceCoefficient;

		//aim influence
		FRotator DeltaMagnetControl = UKismetMathLibrary::NormalizedDeltaRotator(MagnetForceDirection.Rotation(), GetControlRotation());
		float AngleDistanceToMagnet = hypot(DeltaMagnetControl.Pitch, DeltaMagnetControl.Yaw);

		if (AngleDistanceToMagnet > 10.0f * DistanceCoefficient)
		{
			AddControllerPitchInput(DeltaMagnetControl.Pitch / AngleDistanceToMagnet * FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 180.0f), FVector2D(20.0f, 0.01f), AngleDistanceToMagnet) * DistanceCoefficient);
			AddControllerYawInput(DeltaMagnetControl.Yaw / AngleDistanceToMagnet * FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 180.0f), FVector2D(20.0f, 0.01f), AngleDistanceToMagnet) * DistanceCoefficient);
		}
	}
	else
	{
		MagneticEffectSpeedMultiplier = 1.0f;
	}
}

void AAlsCharacter::SetRemoveMagneticEffect(bool bIsSet, float SphereRadius, float MagnetPower, FVector ActorLocation)
{
	if (bIsSet && !ShouldIgnoreEnemyAbilityEffect())
	{
		bIsMagnetic = true;
	}
	else
	{
		bIsMagnetic = false;
	}

	MagneticSphereRadius = SphereRadius;
	MagnetLocation = ActorLocation;
	MagneticEffectPower_Range01 = FMath::Clamp(MagnetPower, 0.01f, 1.0f);
}

void AAlsCharacter::SetRemoveInkEffect(bool bIsSet, float EffectPower)
{
	if (bIsSet && !ShouldIgnoreEnemyAbilityEffect())
	{
		bIsInked = true;
	}
	else
	{
		bIsInked = false;
	}
	InkEffectPower_01Range = FMath::Clamp(EffectPower, 0.0f, 1.0f);
	InkTimeDelay = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, 1.0f), FVector2D(0.0001f, 1.0f), InkEffectPower_01Range);
}

void AAlsCharacter::CalculateInkEffect()
{
	if (bIsInked)
	{
		if (!bIsInkProcessed)
		{
			WeaponRotation_InkEffect = FRotator::ZeroRotator;
			CurrentControlRotation_Ink = GetControlRotation();
			DeltaControlRotatiton = UKismetMathLibrary::NormalizedDeltaRotator(CurrentControlRotation_Ink, PrevControlRotation_Ink);
			PrevControlRotation_Ink = CurrentControlRotation_Ink;

			CurrentLookDirection = FVector2D(DeltaControlRotatiton.Yaw, DeltaControlRotatiton.Pitch);
			float DotDirections = FVector2D::DotProduct(PrevLookDirection.GetSafeNormal(), CurrentLookDirection.GetSafeNormal());
			PrevLookDirection = CurrentLookDirection;

			CurrentLookSpeed = FVector2D(DeltaControlRotatiton.Yaw, DeltaControlRotatiton.Pitch).Length();
			float DeltaSpeed = PrevLookSpeed - CurrentLookSpeed;
			PrevLookSpeed = CurrentLookSpeed;

			if (DeltaSpeed > 2.0f || DotDirections < 0.0f)
			{
				bIsInkProcessed = true;
				DeltaControlRotatiton.Pitch *= InkEffectPower_01Range;
				DeltaControlRotatiton.Yaw *= InkEffectPower_01Range;
			}
		}
		else
		{
			float DeltaLength = FVector2D(DeltaControlRotatiton.Pitch, DeltaControlRotatiton.Yaw).Length();
			if (DeltaLength > 0.01f)
			{
				WeaponRotation_InkEffect += FRotator(DeltaControlRotatiton.Pitch, DeltaControlRotatiton.Yaw, 0.0f);
				DeltaControlRotatiton.Pitch /= 2.0f;
				DeltaControlRotatiton.Yaw /= 2.0f;
			}
			else
			{
				WeaponRotation_InkEffect = FMath::RInterpTo(WeaponRotation_InkEffect, FRotator::ZeroRotator, GetWorld()->GetDeltaSeconds(), 2);
				float QuatDot = WeaponRotation_InkEffect.Quaternion() | FRotator::ZeroRotator.Quaternion();
				if (QuatDot > 0.99999f)
				{
					bIsInkProcessed = false;
				}
			}
		}
	}
	else
	{
		PrevControlRotation_Ink = GetControlRotation();
	}
}

void AAlsCharacter::Restore_Speed_JumpHeight_Health()
{
	if (bIsRestored)
	{
		float InterpSpeed = 2.0f;

		SpeedMultiplier = FMath::FInterpTo(SpeedMultiplier, SpeedMultiplier + CurrentDeltaSpeed, GetWorld()->GetDeltaSeconds(), InterpSpeed);
		CurrentDeltaSpeed = FMath::FInterpTo(CurrentDeltaSpeed, 0.0f, GetWorld()->GetDeltaSeconds(), InterpSpeed);

		AlsCharacterMovement->JumpZVelocity = FMath::FInterpTo(AlsCharacterMovement->JumpZVelocity, AlsCharacterMovement->JumpZVelocity + CurrentDeltaJumpHeight, GetWorld()->GetDeltaSeconds(), InterpSpeed);
		CurrentDeltaJumpHeight = FMath::FInterpTo(CurrentDeltaJumpHeight, 0.0f, GetWorld()->GetDeltaSeconds(), InterpSpeed);

		SetHealth(FMath::FInterpTo(GetHealth(), GetHealth() + CurrentDeltaHealth, GetWorld()->GetDeltaSeconds(), InterpSpeed));
		CurrentDeltaHealth = FMath::FInterpTo(CurrentDeltaHealth, 0.0f, GetWorld()->GetDeltaSeconds(), InterpSpeed);

		SetStamina(FMath::FInterpTo(GetStamina(), GetStamina() + CurrentDeltaStamina, GetWorld()->GetDeltaSeconds(), InterpSpeed));
		CurrentDeltaStamina = FMath::FInterpTo(CurrentDeltaStamina, 0.0f, GetWorld()->GetDeltaSeconds(), InterpSpeed);

		if (CurrentDeltaSpeed == 0.0f && CurrentDeltaJumpHeight == 0.0f && CurrentDeltaHealth == 0.0f)
		{
			bIsRestored = false;
		}
	}
}

void AAlsCharacter::Alter_Speed_JumpHeight_Health_Stamina(float DeltaSpeed, float DeltaJumpHeight, float DeltaHealth, float DeltaStamina, float TimeToRestore)
{
	if (bShouldIgnoreEnemyAbilityEffect)
	{
		return;
	}

	float TempDeltaSpeed = FMath::Clamp(DeltaSpeed, 0.0f, SpeedMultiplier);
	float TempDeltaJumpHeight = FMath::Clamp(DeltaJumpHeight, 0.0f, AlsCharacterMovement->JumpZVelocity);
	float TempDeltaHealth = FMath::Clamp(DeltaHealth, 0.0f, GetHealth());
	float TempDeltaStamina = FMath::Clamp(DeltaStamina, 0.0f, GetStamina());

	SpeedMultiplier -= TempDeltaSpeed;
	AlsCharacterMovement->JumpZVelocity -= TempDeltaJumpHeight;
	SetHealth(GetHealth() - TempDeltaHealth);
	SetStamina(GetStamina() - TempDeltaStamina);

	if (TimeToRestore)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this, TempDeltaSpeed, TempDeltaJumpHeight, TempDeltaHealth, TempDeltaStamina]()
			{
				CurrentDeltaSpeed += TempDeltaSpeed;
				CurrentDeltaJumpHeight += TempDeltaJumpHeight;
				CurrentDeltaHealth += TempDeltaHealth;
				CurrentDeltaStamina += TempDeltaStamina;
				bIsRestored = true;
			}, TimeToRestore, false);
	}
}

void AAlsCharacter::ConcatenationEffect_Implementation(bool bIsSet, bool bReplaceWeapon, int32 GluedObjectsQuantity_1to6)
{
	if (bIsSet && !ShouldIgnoreEnemyAbilityEffect())
	{
		SphereCollisionForGluedActors = NewObject<USphereComponent>(this, USphereComponent::StaticClass(), TEXT("GluedActorsSphereCollision"));

		SphereCollisionForGluedActors->RegisterComponent();

		SphereCollisionForGluedActors->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);

		SphereCollisionForGluedActors->SetSphereRadius(1000.0f);
		SphereCollisionForGluedActors->SetRelativeLocation(FVector(0.0f, 1000.0f, -40.0f));
		//SphereCollisionForGluedActors->SetHiddenInGame(false);
		SphereCollisionForGluedActors->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		SphereCollisionForGluedActors->SetCollisionProfileName(FName(TEXT("OverlapAll")));
		SphereCollisionForGluedActors->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

		TArray<FName> SocketNames;
		SocketNames = GetMesh()->GetAllSocketNames();
		for (FName SocketName : SocketNames)
		{
			if (SocketName.ToString().StartsWith(TEXT("Glued_Socket_")))
			{
				GluedSocketNames.AddUnique(SocketName);
			}
		}

		ConcatenationEffectSpeedMultiplier = 1.0f - 0.15f * GluedObjectsQuantity_1to6;
		ConcatenationEffectLookSpeedMultiplier = 1.0f - 0.075f * GluedObjectsQuantity_1to6;
	}
	else
	{
		for (AActor* GluedActor : GluedActors)
		{
			GluedActor->Destroy();
		}
		GluedActors.Empty();
		ConcatenationEffectSpeedMultiplier = 1.0f;
		ConcatenationEffectLookSpeedMultiplier = 1.0f;
	}
}

void AAlsCharacter::SetWeightSpeedMultiplier(float CurrentWeight)
{
	WeightMultiplier = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, GetStrength()), FVector2D(1.0f, 0.0f), CurrentWeight - GetStrength());
	RefreshJumpZVelocity();
	if (WeightMultiplier == 0.0f)
	{
		bIsOverload = true;
	}
	else
	{
		bIsOverload = false;
	}
}

void AAlsCharacter::RefreshJumpZVelocity()
{
	AlsCharacterMovement->JumpZVelocity = CurrentZVelocity * WeightMultiplier * HigherJumpBy_40;
}

void AAlsCharacter::SprintTimeDelayCount()
{
	if (GetDesiredGait() == AlsGaitTags::Sprinting)
	{
		SprintTimeDelay += GetWorld()->GetDeltaSeconds();
	}
	else
	{
		SprintTimeDelay -= GetWorld()->GetDeltaSeconds();
	}
	SprintTimeDelay = FMath::Clamp(SprintTimeDelay, 0.0f, SprintTimeDelayMax);
}

void AAlsCharacter::CheckForHealthReplenish()
{
	if (bShouldReplenish_50 && GetHealth() <= 10.0f)
	{
		bShouldReplenish_50 = false;
		Health = 50.0f;
	}
}

void AAlsCharacter::RefreshStaminaHealthStandingMultiplier()
{
	if (bIsStaminaHealthStandingMultiplierApplied && GetVelocity().Length() == 0.0f)
	{
		StaminaHealthStandingMultiplier = 1.33f;
	}
	else
	{
		StaminaHealthStandingMultiplier = 1.0f;
	}
}

void AAlsCharacter::RefreshStaminaHealthRunningMultiplier()
{
	if (bIsStaminaHealthRunningMultiplierApplied && GetVelocity().Length() > 0.0f && GetGait() != AlsGaitTags::Sprinting)
	{
		StaminaHealthRunningMultiplier = 1.25f;
	}
	else
	{
		StaminaHealthRunningMultiplier = 1.0f;
	}
}

void AAlsCharacter::RefreshAimAccuracy()
{
	AimAccuracyOnMove = 1.0f;
	AimAccuracyOnStrafing = 1.0f;
	AimAccuracyOnWalking = 1.0f;

	if (IsAiming)
	{
		if (bIsAimPrecisionOnMoveApplied)
		{
			if (GetVelocity().Length() == 0.0f)
			{
				AimAccuracyOnMove = 1.25f;
			}
			else
			{
				AimAccuracyOnMove = 0.75f;
			}
		}

		if (bAimAccuracyOnStrafing_30 && GetVelocity().Length() > 0.0f && FMath::IsNearlyEqual(FVector::DotProduct(GetVelocity().GetSafeNormal(), GetActorForwardVector()), 0.0f, 0.001f))
		{
			AimAccuracyOnStrafing = 0.7f;
		}

		if (bAimAccuracyOnWalking_30 && GetVelocity().Length() > 0.0f && GetGait() == AlsGaitTags::Walking)
		{
			AimAccuracyOnWalking = 0.7f;
		}
	}

	AimAccuracyMultiplier = AimAccuracyOnMove * AimAccuracyOnStrafing * AimAccuracyOnWalking * AimAccuracy_50;
}

void AAlsCharacter::RefreshDamage()
{
	MainDamageMultiplier = DamageMultiplier_13 * LastStandDamageMultiplier * DamageMultiplier_25 * DamageMultiplierIfHealthIsUnder_30;
}

void AAlsCharacter::RefreshStaminaAndRecoilIfHealthIsUnder_20()
{
	if (bIsHealthIsUnder_20 && GetHealth() < 20.0f)
	{
		StaminaRegenerationRateValue_11 = 10.0f;
		RecoilMultiplierValue_11 = 0.5f;
	}
	else
	{
		StaminaRegenerationRateValue_11 = 1.0f;
		RecoilMultiplierValue_11 = 1.0f;
	}
}

void AAlsCharacter::RefreshHealthIfStaminaIsUnder_30()
{
	if (bIsStaminaIsUnder_30 && GetStamina() < 30.0f)
	{
		HealthRecoveryRateValue_12 = 1.5f;
	}
	else
	{
		HealthRecoveryRateValue_12 = 1.0f;
	}
}

void AAlsCharacter::RefreshDamageAmountOnMovingOrOnStanding()
{
	if (bIsDamagedOnMovingOrOnStanding)
	{
		if (GetVelocity().Length() > 0.0f)
		{
			DamageMultiplier_13 = 0.7f;
		}
		else
		{
			DamageMultiplier_13 = 1.3f;
		}
	}
	else
	{
		DamageMultiplier_13 = 1.0f;
	}
}

float AAlsCharacter::RecalculateDamage(float Damage, FText WeaponName)
{
	//GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, WeaponName.ToString());

	if (bShouldIgnoreDamageOnRoll && LocomotionAction == AlsLocomotionActionTags::Rolling || bShouldIgnoreDamage)
	{
		return 0.0f;
	}

	if ((bShouldReduceDamageMelee && WeaponName.ToString() == "Melee") || (bShouldReduceDamageProjectile && WeaponName.ToString() == "Projectile"))
	{
		return Damage * 0.5f;
	}

	return Damage;
}

void AAlsCharacter::CheckIfHealthIsUnder_20()
{
	LastStandSpeedMultiplier = 1.0f;
	LastStandDamageMultiplier = 1.0f;

	if (GetHealth() < GetMaxHealth() * 0.2f)
	{
		LastStandSpeedMultiplier = 1.2f;
		LastStandDamageMultiplier = 0.8f;
	}
}

bool AAlsCharacter::ShouldIgnoreEnemyAbilityEffect()
{
	float RandomChance = FMath::FRandRange(0.0f, 100.0f);
	if (bShouldIgnoreEnemyAbilityEffect && RandomChance < 30.0f)
	{
		return true;
	}
	return false;
}

void AAlsCharacter::CheckIfShouldIncreaseWalkAndRunSpeed()
{
	WalkAndRunSpeedMultiplier_15 = 1.0f;

	if (bShouldIncreaseWalkAndRunSpeed && GetGait() != AlsGaitTags::Sprinting)
	{
		WalkAndRunSpeedMultiplier_15 = 1.15f;
	}
}

void AAlsCharacter::CheckIfShouldDecreaseWalkRunSpeedAnDamage()
{
	if (bShouldDecreaseWalkRunSpeedAndDamage && GetVelocity().Length() > 0.0f)
	{
		WalkRunSpeedMultiplier_25 = 0.75f;
		DamageMultiplier_25 = 0.75f;
	}
	else
	{
		WalkRunSpeedMultiplier_25 = 1.0f;
		DamageMultiplier_25 = 1.0f;
	}
}

void AAlsCharacter::IncreaseHealth_30_20c()
{
	if (bShouldIncreaseHealth_30)
	{
		SetHealth(GetHealth() + 30.0f * GetWorld()->GetDeltaSeconds() / 20.0f);
	}
}

bool AAlsCharacter::ShouldIgnoreStunIfHealthIsUnder_50()
{
	if (bIsSetEffect_41)
	{
		if (GetHealth() < 50.0f)
		{
			float ChanceToIgnore = FMath::FRandRange(0.0f, 100.0f);
			if (ChanceToIgnore > 70.0)
			{
				return true;
			}
		}
	}
	return false;
}

void AAlsCharacter::CheckIfStaminaIsUnder_70()
{
	SpeedMultiplierIfStaminaLess_70 = 1.0f;

	if (bShouldIncreaseSpeedIfStaminaLess_70)
	{
		if (GetStamina() < 70.0f)
		{
			SpeedMultiplierIfStaminaLess_70 = 1.2f;
		}
	}
}

bool AAlsCharacter::CheckIfShouldIgnoreKnockdownEffect()
{
	if (bIsSetEffect_44)
	{
		if (GetVelocity().Length() == 0.0f)
		{
			float ChanceToIgnoreKnockdownEffect = FMath::FRandRange(0.0f, 100.0f);
			if (ChanceToIgnoreKnockdownEffect > 50.0f)
			{
				return true;
			}
		}
	}
	return false;
}

void AAlsCharacter::CheckIfHealthIsUnder_30()
{
	DamageMultiplierIfHealthIsUnder_30 = 1.0f;

	if (bIsSetEffect_45)
	{
		if (GetHealth() < GetMaxHealth() * 0.3f)
		{
			DamageMultiplierIfHealthIsUnder_30 = 1.3f;
		}
	}
}
