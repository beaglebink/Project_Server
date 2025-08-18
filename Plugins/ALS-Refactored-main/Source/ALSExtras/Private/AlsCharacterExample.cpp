#include "AlsCharacterExample.h"

#include "AlsCameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include <PhysicsEngine/PhysicsConstraintComponent.h>
#include "Kismet/KismetMathLibrary.h"
#include "AlsCharacterMovementComponent.h"
#include "UI/AttributesWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Inventory/AC_Inventory.h"
#include "Inventory/AC_Container.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include <Blueprint/AIBlueprintHelperLibrary.h>
#include "BehaviorTree/BlackboardComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AlsCharacterExample)

AAlsCharacterExample::AAlsCharacterExample()
{
	Camera = CreateDefaultSubobject<UAlsCameraComponent>(FName{ TEXTVIEW("Camera") });
	Camera->SetupAttachment(GetMesh());
	Camera->SetRelativeRotation_Direct({ 0.0f, 90.0f, 0.0f });

	SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureCompopent2D"));
	SceneCaptureComponent->SetupAttachment(RootComponent);
	SceneCaptureComponent->SetRelativeLocation(FVector(200.0f, 0.0f, 20.0f));
	SceneCaptureComponent->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	SceneCaptureComponent->FOVAngle = 50.0f;

	InventoryComponent = CreateDefaultSubobject<UAC_Inventory>(TEXT("InventoryComponent"));

	//PhysicsConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("PhysicsConstraint"));
	//PhysicsConstraint->SetupAttachment(RootComponent);

	EffectTimerHandles.SetNum(56);
}

void AAlsCharacterExample::BeginPlay()
{
	Super::BeginPlay();
	/*
		PhysicsConstraint->SetLinearXLimit(LCM_Free, 0.0f);
		PhysicsConstraint->SetLinearYLimit(LCM_Free, 0.0f);
		PhysicsConstraint->SetLinearZLimit(LCM_Free, 0.0f);

		PhysicsConstraint->SetAngularSwing1Limit(ACM_Free, 0.0f);
		PhysicsConstraint->SetAngularSwing2Limit(ACM_Free, 0.0f);
		PhysicsConstraint->SetAngularTwistLimit(ACM_Free, 0.0f);

		PhysicsConstraint->SetLinearPositionDrive(true, true, true);
		PhysicsConstraint->SetLinearDriveParams(500.0f, 50.0f, 0.0f);

		PhysicsConstraint->SetAngularOrientationDrive(true, true);
		PhysicsConstraint->SetAngularDriveParams(500.0f, 50.0f, 0.0f);
	*/

	FrameListSize = static_cast<uint8>(floor(RepeatingPeaceDuration / GetWorld()->GetDeltaSeconds()));

	InitializeFoodEffectMap();
	InitializeFoodEffectTimerDelegates();
}

void AAlsCharacterExample::NotifyControllerChanged()
{
	const auto* PreviousPlayer{ Cast<APlayerController>(PreviousController) };
	if (IsValid(PreviousPlayer))
	{
		auto* InputSubsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PreviousPlayer->GetLocalPlayer()) };
		if (IsValid(InputSubsystem))
		{
			InputSubsystem->RemoveMappingContext(InputMappingContext);
		}
	}

	auto* NewPlayer{ Cast<APlayerController>(GetController()) };
	if (IsValid(NewPlayer))
	{
		NewPlayer->InputYawScale_DEPRECATED = 1.0f;
		NewPlayer->InputPitchScale_DEPRECATED = 1.0f;
		NewPlayer->InputRollScale_DEPRECATED = 1.0f;

		auto* InputSubsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(NewPlayer->GetLocalPlayer()) };
		if (IsValid(InputSubsystem))
		{
			FModifyContextOptions Options;
			Options.bNotifyUserSettings = true;

			InputSubsystem->AddMappingContext(InputMappingContext, 0, Options);
		}
	}

	Super::NotifyControllerChanged();
}

float AAlsCharacterExample::GetNetGrenadeParalyseTime() const
{
	return NetGrenadeParalyseTime;
}
void AAlsCharacterExample::ParalyzeNPC(AActor* Reason, float Time)
{
	if (Reason == ReasonParalyse) return;

	ReasonParalyse = Reason;

	if (bIsStunned || bIsGrappled || bIsWired || bIsSliding || bIsStickyStuck || bIsBubbled || bIsOverload)
	{
		return;
	}
	if (Time > 0.0f)
	{
		bIsStunned = true;
		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->StopMovementImmediately();

		AAIController* AIController = Cast<AAIController>(GetController());
		if (AIController)
		{
			TestController = GetController();
			FocusActor = AIController->GetFocusActor();
			AIController->SetFocus(nullptr);
			UBlackboardComponent* BlackBoard = UAIBlueprintHelperLibrary::GetBlackboard(AIController);
			if (BlackBoard)
			{
				Target = BlackBoard->GetValueAsObject(TEXT("Target"));
				BlackBoard->SetValueAsObject(TEXT("Target"), nullptr);
			}

			Controller = nullptr; // Clear the controller to prevent any further input processing
		}
		GetWorldTimerManager().SetTimer(StunTimerHandle, this, &AAlsCharacterExample::EndStun, Time, false);
	}
}
void AAlsCharacterExample::EndStun()
{
	OnNetParalyse.Broadcast(ReasonParalyse);
	bIsStunned = false;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Walking);

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->SetFocus(FocusActor);
		UBlackboardComponent* BlackBoard = UAIBlueprintHelperLibrary::GetBlackboard(AIController);
		if (BlackBoard)
		{
			BlackBoard->SetValueAsObject(TEXT("Target"), Target);
		}
	}

	/*
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		UBrainComponent* BrainComponent = AIController->GetBrainComponent();
		if (BrainComponent)
		{
			BrainComponent->RestartLogic();
		}
	}
	*/
	GetCharacterMovement()->SetDefaultMovementMode();
	GetWorldTimerManager().ClearTimer(StunTimerHandle);

	Controller = TestController; // Restore the controller after stun ends
	/*
	if (bIsDiscombobulated)
	{
		DiscombobulateEffect();
	}
	if (bIsInked)
	{
		CalculateInkEffect();
	}
	if (bIsWired)
	{
		SetRemoveWireEffect(false, 0.0f);
	}
	if (bIsGrappled)
	{
		SetRemoveGrapple(false);
	}
	if (bIsStickyStuck)
	{
		SetRemoveStickyStuck(false);
	}
	*/
}
void AAlsCharacterExample::CalcCamera(const float DeltaTime, FMinimalViewInfo& ViewInfo)
{
	if (Camera->IsActive())
	{
		Camera->GetViewInfo(ViewInfo);
		return;
	}

	Super::CalcCamera(DeltaTime, ViewInfo);
}

void AAlsCharacterExample::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	auto* EnhancedInput = Cast<UEnhancedInputComponent>(Input);
	if (IsValid(EnhancedInput))
	{
		EnhancedInput->BindAction(LookMouseAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnLookMouse);
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnLook);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnMove);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::Input_OnMove_Released);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnSprint);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::Input_OnSprint);
		EnhancedInput->BindAction(WalkAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnWalk);
		EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnCrouch);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnJump);
		EnhancedInput->BindAction(AimAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnAim);
		EnhancedInput->BindAction(RagdollAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnRagdoll);
		EnhancedInput->BindAction(RollAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnRoll);
		EnhancedInput->BindAction(RotationModeAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnRotationMode);
		EnhancedInput->BindAction(ViewModeAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnViewMode);
		EnhancedInput->BindAction(SwitchShoulderAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnSwitchShoulder);
		EnhancedInput->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnSwitchWeapon);
		EnhancedInput->BindAction(RemoveSticknessAction, ETriggerEvent::Completed, this, &ThisClass::Input_OnRemoveStickness);
		EnhancedInput->BindAction(GrappleRemoveAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnRemoveGrapple);

		if (Inventory)
		{
			Inventory->BindInput(EnhancedInput);
		}
	}
}

void AAlsCharacterExample::Input_OnLookMouse(const FInputActionValue& ActionValue)
{
	if (bIsStunned || bIsInkProcessed)
	{
		return;
	}

	LoopEffectFrame.FrameActionValue_OnLookMouse = ActionValue;

	const auto Value{ ActionValue.Get<FVector2D>() };
	float PitchDirection = Value.Y * LookUpMouseSensitivity * StunRecoveryMultiplier * WireEffectPower_01Range * GrappleEffectSpeedMultiplier * ConcatenationEffectLookSpeedMultiplier;
	float YawDirection = Value.X * LookRightMouseSensitivity * StunRecoveryMultiplier * WireEffectPower_01Range * GrappleEffectSpeedMultiplier * ConcatenationEffectLookSpeedMultiplier;

	ShakeMouseRemoveEffect(Value);

	if (bIsDiscombobulated)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this, PitchDirection, YawDirection]()
			{
				AddControllerPitchInput(PitchDirection);
				AddControllerYawInput(YawDirection);
			}, DiscombobulateTimeDelay, false);
	}
	if (bIsInked)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this, PitchDirection, YawDirection]()
			{
				if (!bIsInkProcessed)
				{
					AddControllerPitchInput(PitchDirection);
					AddControllerYawInput(YawDirection);
				}
			}, InkTimeDelay, false);
	}
	if (!bIsDiscombobulated && !bIsInked)
	{
		AddControllerPitchInput(PitchDirection);
		AddControllerYawInput(YawDirection);
	}
}

void AAlsCharacterExample::Input_OnLook(const FInputActionValue& ActionValue)
{
	if (bIsStunned || bIsInkProcessed)
	{
		return;
	}

	LoopEffectFrame.FrameActionValue_OnLook = ActionValue;

	const auto Value{ ActionValue.Get<FVector2D>() };
	float PitchDirection = Value.Y * LookUpRate * StunRecoveryMultiplier * WireEffectPower_01Range * GrappleEffectSpeedMultiplier * ConcatenationEffectLookSpeedMultiplier;
	float YawDirection = Value.X * LookRightRate * StunRecoveryMultiplier * WireEffectPower_01Range * GrappleEffectSpeedMultiplier * ConcatenationEffectLookSpeedMultiplier;

	if (bIsDiscombobulated)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this, PitchDirection, YawDirection]()
			{
				AddControllerPitchInput(PitchDirection);
				AddControllerYawInput(YawDirection);
			}, DiscombobulateTimeDelay, false);
	}
	if (bIsInked)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this, PitchDirection, YawDirection]()
			{
				if (!bIsInkProcessed)
				{
					AddControllerPitchInput(PitchDirection);
					AddControllerYawInput(YawDirection);
				}
			}, InkTimeDelay, false);
	}
	if (!bIsDiscombobulated && !bIsInked)
	{
		AddControllerPitchInput(PitchDirection);
		AddControllerYawInput(YawDirection);
	}
}

void AAlsCharacterExample::Input_OnMove(const FInputActionValue& ActionValue)
{
	if (bIsStunned || bIsSliding || bIsStickyStuck || bIsBubbled || bIsOverload)
	{
		return;
	}

	LoopEffectFrame.FrameActionValue_OnMove = ActionValue;

	const auto Value{ UAlsMath::ClampMagnitude012D(ActionValue.Get<FVector2D>()) };

	LastInputDirection = Value;

	const auto ForwardDirection{ UAlsMath::AngleToDirectionXY(UE_REAL_TO_FLOAT(GetViewState().Rotation.Yaw)) };
	const auto RightDirection{ UAlsMath::PerpendicularCounterClockwiseXY(ForwardDirection) };
	if (bIsDiscombobulated)
	{
		FVector Direction = ForwardDirection * Value.Y + RightDirection * Value.X;
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this, Direction]()
			{
				AddMovementInput(Direction * (bIsInputReversed ? -1.0f : 1.0f));
			}, DiscombobulateTimeDelay, false);
	}
	else
	{
		AddMovementInput((ForwardDirection * Value.Y + RightDirection * Value.X) * (bIsInputReversed ? -1.0f : 1.0f));
	}
}

void AAlsCharacterExample::Input_OnMove_Released()
{
	RemoveSticknessByMash();
}

void AAlsCharacterExample::Input_OnSprint(const FInputActionValue& ActionValue)
{
	if (bIsGrappled || bIsStunned || bIsSliding || bIsSticky || bIsOverload)
	{
		return;
	}

	FGameplayTag GaitTag = ActionValue.Get<bool>() ? AlsGaitTags::Sprinting : AlsGaitTags::Running;

	auto StartSprintLambda = [this, GaitTag]()
		{
			if (GetStamina() > SprintStaminaDrainRate && AbleToSprint)
			{
				if (GetDesiredGait() == AlsGaitTags::Sprinting && GaitTag == AlsGaitTags::Sprinting && FVector2D(GetVelocity()).Length())
				{
					SetStamina(GetStamina() - SprintStaminaDrainRate);
				}
				else
				{
					SetDesiredGait(GaitTag);
					OnSetSprintMode(GaitTag == AlsGaitTags::Sprinting ? true : false);
				}
			}
			else if (AbleToSprint && GetDesiredGait() == AlsGaitTags::Sprinting)
			{
				AbleToSprint = false;
				SetDesiredGait(AlsGaitTags::Running);
				OnSetSprintMode(false);
				FTimerHandle TimerHandle;
				GetWorld()->GetTimerManager().SetTimer(TimerHandle, [&]() {AbleToSprint = true; }, ExhaustionPenaltyDuration, false);
			}
		};

	if (bIsDiscombobulated)
	{
		FTimerHandle TimerHandleDiscombobulate;
		GetWorldTimerManager().SetTimer(TimerHandleDiscombobulate, StartSprintLambda, DiscombobulateTimeDelay, false);
	}
	else
	{
		StartSprintLambda();
	}
}

void AAlsCharacterExample::Input_OnWalk()
{
	if (bIsGrappled)
	{
		return;
	}

	if (GetDesiredGait() == AlsGaitTags::Walking)
	{
		SetDesiredGait(AlsGaitTags::Running);
	}
	else if (GetDesiredGait() == AlsGaitTags::Running)
	{
		SetDesiredGait(AlsGaitTags::Walking);
	}
}

void AAlsCharacterExample::Input_OnCrouch()
{
	if (bIsWired)
	{
		return;
	}
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, [&]()
		{
			if (GetDesiredStance() == AlsStanceTags::Standing)
			{
				SetDesiredStance(AlsStanceTags::Crouching);
			}
			else if (GetDesiredStance() == AlsStanceTags::Crouching)
			{
				SetDesiredStance(AlsStanceTags::Standing);
			}
		}, DelayCrouchInOut, false);
}

void AAlsCharacterExample::Input_OnJump(const FInputActionValue& ActionValue)
{
	LoopEffectFrame.FrameActionValue_OnJump = ActionValue;
	LoopEffectFrame.FrameState = EnumLoopStates::Jump;

	if (bIsStunned || bIsSliding || bIsStickyStuck || bIsWired || bIsGrappled || bIsBubbled || bIsOverload)
	{
		return;
	}

	if (GetStamina() > JumpStaminaCost && ActionValue.Get<bool>())
	{
		if (bIsDiscombobulated)
		{
			FTimerHandle TimerHandleDiscombobulate;
			GetWorldTimerManager().SetTimer(TimerHandleDiscombobulate, [this]()
				{
					if (StopRagdolling())
					{
						return;
					}

					if (GetStance() == AlsStanceTags::Crouching)
					{
						SetDesiredStance(AlsStanceTags::Standing);
						return;
					}

					if (IsFirstJumpClick)
					{
						IsFirstJumpClick = false;
						GetWorldTimerManager().SetTimer(JumpTimerHandle, this, &AAlsCharacterExample::ContinueJump, DoubleSpaceTime, false);
						return;
					}
					else
					{
						JumpTimerHandle.Invalidate();
						if (StartMantlingGrounded())
						{
							return;
						}
					}

					Jump();
				}, DiscombobulateTimeDelay, false);
		}
		else
		{
			if (StopRagdolling())
			{
				return;
			}

			if (GetStance() == AlsStanceTags::Crouching)
			{
				SetDesiredStance(AlsStanceTags::Standing);
				return;
			}

			if (IsFirstJumpClick)
			{
				IsFirstJumpClick = false;
				GetWorldTimerManager().SetTimer(JumpTimerHandle, this, &AAlsCharacterExample::ContinueJump, DoubleSpaceTime, false);
				return;
			}
			else
			{
				JumpTimerHandle.Invalidate();
				if (StartMantlingGrounded())
				{
					return;
				}
			}

			Jump();
		}
	}
	else
	{
		StopJumping();
	}
}

void AAlsCharacterExample::Input_OnAim(const FInputActionValue& ActionValue)
{
	LoopEffectFrame.FrameActionValue_OnAim = ActionValue;
	LoopEffectFrame.FrameState = EnumLoopStates::Aim;

	if (IsImplementingAIM)
	{
		SetDesiredAiming(ActionValue.Get<bool>());
	}
}

void AAlsCharacterExample::Input_OnRagdoll()
{
	if (!StopRagdolling())
	{
		StartRagdolling();
	}
}

void AAlsCharacterExample::Input_OnRoll()
{
	if (bIsStunned || bIsSliding || bIsSticky || bIsGrappled || bIsWired || bIsBubbled || bIsOverload)
	{
		return;
	}

	LoopEffectFrame.FrameState = EnumLoopStates::Roll;

	static constexpr auto PlayRate{ 1.3f };
	float CurrentPlayRate = PlayRate * FasterRollRate;


	if (GetStamina() > RollStaminaCost)
	{
		if (bIsDiscombobulated)
		{
			FTimerHandle TimerHandleDiscombobulate;
			GetWorldTimerManager().SetTimer(TimerHandleDiscombobulate, [this, CurrentPlayRate]()
				{
					StartRolling(CurrentPlayRate);
				}, DiscombobulateTimeDelay, false);
		}
		else
		{
			StartRolling(CurrentPlayRate);
		}
	}
}

void AAlsCharacterExample::Input_OnRotationMode()
{
	SetDesiredRotationMode(GetDesiredRotationMode() == AlsRotationModeTags::VelocityDirection
		? AlsRotationModeTags::ViewDirection
		: AlsRotationModeTags::VelocityDirection);
}

void AAlsCharacterExample::Input_OnViewMode()
{
	// need to be remade from BP
	//SetViewMode(GetViewMode() == AlsViewModeTags::ThirdPerson ? AlsViewModeTags::FirstPerson : AlsViewModeTags::ThirdPerson);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void AAlsCharacterExample::Input_OnSwitchShoulder()
{
	Camera->SetRightShoulder(!Camera->IsRightShoulder());
}

void AAlsCharacterExample::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& Unused, float& VerticalLocation)
{
	if (Camera->IsActive())
	{
		Camera->DisplayDebug(Canvas, DisplayInfo, VerticalLocation);
	}

	Super::DisplayDebug(Canvas, DisplayInfo, Unused, VerticalLocation);
}

void AAlsCharacterExample::GrabExistingObject(AActor* ExistingActor)
{
	/*
	if (ExistingActor)
	{
		UE_LOG(LogTemp, Log, TEXT("ExistingActor is valid: %s"), *ExistingActor->GetName());

		UPrimitiveComponent* ComponentToGrab = Cast<UPrimitiveComponent>(ExistingActor->GetComponentByClass(UPrimitiveComponent::StaticClass()));

		if (ComponentToGrab)
		{
			UE_LOG(LogTemp, Log, TEXT("ComponentToGrab is valid: %s"), *ComponentToGrab->GetName());

			if (PhysicsConstraint && AttachmentPoint)
			{
				UE_LOG(LogTemp, Log, TEXT("PhysicsConstraint and AttachmentPoint are valid"));

				AttachmentPoint->SetSimulatePhysics(true);
				// Проверка физического состояния компонентов
				if (!AttachmentPoint->IsSimulatingPhysics())
				{
					UE_LOG(LogTemp, Warning, TEXT("AttachmentPoint is not simulating physics. Enabling physics simulation."));
					AttachmentPoint->SetSimulatePhysics(true);
				}

				if (!ComponentToGrab->IsSimulatingPhysics())
				{
					UE_LOG(LogTemp, Warning, TEXT("ComponentToGrab is not simulating physics. Enabling physics simulation."));
					ComponentToGrab->SetSimulatePhysics(true);
				}

				PhysicsConstraint->SetConstrainedComponents(AttachmentPoint, NAME_None, ComponentToGrab, NAME_None);
				PhysicsConstraint->SetWorldLocation(AttachmentPoint->GetComponentLocation());

				// Проверка, что компоненты действительно присоединены
				UPrimitiveComponent* ConstrainedComponent1;
				UPrimitiveComponent* ConstrainedComponent2;
				FName BoneName1, BoneName2;
				PhysicsConstraint->GetConstrainedComponents(ConstrainedComponent1, BoneName1, ConstrainedComponent2, BoneName2);

				if (ConstrainedComponent1 == AttachmentPoint && ConstrainedComponent2 == ComponentToGrab)
				{
					UE_LOG(LogTemp, Log, TEXT("Components successfully constrained"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Failed to constrain components"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("PhysicsConstraint or AttachmentPoint is null"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ComponentToGrab is null in GrabExistingObject"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ExistingActor is null in GrabExistingObject"));
	}
	*/
}

void AAlsCharacterExample::ReleaseObject()
{
	//PhysicsConstraint->BreakConstraint();
}

void AAlsCharacterExample::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	/*
		if (PhysicsConstraint->ConstraintInstance.IsValidConstraintInstance())
		{
			FVector TargetLocation = AttachmentPoint->GetComponentLocation();
			FRotator TargetRotation = AttachmentPoint->GetComponentRotation();

			PhysicsConstraint->SetWorldLocationAndRotation(TargetLocation, TargetRotation);
		}
	*/

	SprintTimeDelayCount();

	LoopEffect();
}

void AAlsCharacterExample::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitStatWidget();

	Inventory = FindComponentByClass<UAC_Inventory>();
	if (Inventory)
	{
		Inventory->ContainerComponent->OnWeightChanged.AddDynamic(this, &AAlsCharacter::SetWeightSpeedMultiplier);
	}
}

void AAlsCharacterExample::UnPossessed()
{
	Super::UnPossessed();

	if (AttributesWidget)
	{
		AttributesWidget->RemoveFromParent();
		AttributesWidget = nullptr;
	}
}

void AAlsCharacterExample::ContinueJump()
{
	IsFirstJumpClick = true;
	Jump();
}

void AAlsCharacterExample::Input_OnSwitchWeapon()
{
	SwitchWeaponHandle();
}

void AAlsCharacterExample::Input_OnRemoveStickness()
{
	RemoveSticknessByMash();
}

void AAlsCharacterExample::Input_OnRemoveGrapple(const FInputActionValue& ActionValue)
{
	float X = ActionValue.Get<FVector2D>().X;
	float Y = ActionValue.Get<FVector2D>().Y;
	PressTwoKeysRemoveGrappleEffect(X || Y ? true : false);
}

// UI
void AAlsCharacterExample::InitStatWidget()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (AttributesWidgetClass)
		{
			if (AttributesWidget = CreateWidget<UAttributesWidget>(PC, AttributesWidgetClass))
			{
				AttributesWidget->InitWithCharacterOwner(this);
				AttributesWidget->AddToViewport(10);
			}
		}
	}
}

void AAlsCharacterExample::SetLoopEffect(bool bIsSet)
{
	if (bIsSet && !ShouldIgnoreEnemyAbilityEffect())
	{
		bIsLooped = true;
	}
	else
	{
		bIsLooped = false;
	}
}

void AAlsCharacterExample::LoopEffect()
{
	if (bIsLooped)
	{
		auto* InputSubsystem{ ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Cast<APlayerController>(GetController())->GetLocalPlayer()) };
		if (IsValid(InputSubsystem))
		{
			InputSubsystem->RemoveMappingContext(InputMappingContext);
		}

		if (LoopsCounter < HowManyLoops)
		{
			if (FrameIt)
			{
				switch (FrameIt.GetNode()->GetValue().FrameState)
				{
				case EnumLoopStates::None:
					SetDesiredGait(FrameIt.GetNode()->GetValue().LoopEffectGaitTag);
					SetDesiredStance(FrameIt.GetNode()->GetValue().LoopEffectStanceTag);
					break;
				case EnumLoopStates::Jump:
					Input_OnJump(FrameIt.GetNode()->GetValue().FrameActionValue_OnJump);
					break;
				case EnumLoopStates::Aim:
					Input_OnAim(FrameIt.GetNode()->GetValue().FrameActionValue_OnAim);
					break;
				case EnumLoopStates::Roll:
					Input_OnRoll();
					break;
				default:
					break;
				}

				Input_OnLookMouse(FrameIt.GetNode()->GetValue().FrameActionValue_OnLookMouse);
				Input_OnLook(FrameIt.GetNode()->GetValue().FrameActionValue_OnLook);
				Input_OnMove(FrameIt.GetNode()->GetValue().FrameActionValue_OnMove);

				++FrameIt;
			}
			else
			{
				FrameIt = TDoubleLinkedList<FLoopEffectFrame>::TIterator(FrameList.GetHead());
				++LoopsCounter;
			}
		}
		else
		{
			FrameList.Empty();
			LoopEffectFrame = FLoopEffectFrame();
			bIsLooped = false;
			LoopsCounter = 0;

			InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Cast<APlayerController>(GetController())->GetLocalPlayer());
			if (IsValid(InputSubsystem))
			{
				InputSubsystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}
	else
	{
		if (FrameList.Num() == FrameListSize)
		{
			FrameList.RemoveNode(FrameList.GetHead());
		}
		LoopEffectFrame.LoopEffectGaitTag = GetDesiredGait();
		LoopEffectFrame.LoopEffectStanceTag = GetDesiredStance();
		FrameList.AddTail(LoopEffectFrame);
		FrameIt = TDoubleLinkedList<FLoopEffectFrame>::TIterator(FrameList.GetHead());
		LoopEffectFrame = FLoopEffectFrame();
	}
}

void AAlsCharacterExample::SetSceneRenderComponents(AActor* Actor)
{
	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);

	for (UActorComponent* Component : Components)
	{
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
		{
			if (Primitive->IsVisible() && Primitive->bRenderInMainPass)
			{
				SceneCaptureComponent->ShowOnlyComponents.Add(Primitive);
			}
		}
	}
	TArray<AActor*> AttachedActors;
	Actor->GetAttachedActors(AttachedActors, true, true);

	for (AActor* Attached : AttachedActors)
	{
		SetSceneRenderComponents(Attached);
	}
}

void AAlsCharacterExample::InitializeFoodEffectTimerDelegates()
{
	EffectTimerHandles[1].EffectDelegate.BindLambda([this]() {SetEffect_1(); });
	EffectTimerHandles[2].EffectDelegate.BindLambda([this]() {SetEffect_2(); });
	EffectTimerHandles[3].EffectDelegate.BindLambda([this]() {SetEffect_3(); });
	EffectTimerHandles[4].EffectDelegate.BindLambda([this]() {SetEffect_4(); });
	EffectTimerHandles[5].EffectDelegate.BindLambda([this]() {SetEffect_5(); });
	EffectTimerHandles[6].EffectDelegate.BindLambda([this]() {SetEffect_6(); });
	EffectTimerHandles[7].EffectDelegate.BindLambda([this]() {SetEffect_7(); });
	EffectTimerHandles[8].EffectDelegate.BindLambda([this]() {SetEffect_8(); });
	EffectTimerHandles[9].EffectDelegate.BindLambda([this]() {SetEffect_9(); });
	EffectTimerHandles[10].EffectDelegate.BindLambda([this]() {SetEffect_10(); });
	EffectTimerHandles[11].EffectDelegate.BindLambda([this]() {SetEffect_11(); });
	EffectTimerHandles[12].EffectDelegate.BindLambda([this]() {SetEffect_12(); });
	EffectTimerHandles[13].EffectDelegate.BindLambda([this]() {SetEffect_13(); });
	EffectTimerHandles[14].EffectDelegate.BindLambda([this]() {SetEffect_14(); });
	EffectTimerHandles[15].EffectDelegate.BindLambda([this]() {SetEffect_15(); });
	EffectTimerHandles[16].EffectDelegate.BindLambda([this]() {SetEffect_16(); });
	EffectTimerHandles[17].EffectDelegate.BindLambda([this]() {SetEffect_17(); });
	EffectTimerHandles[18].EffectDelegate.BindLambda([this]() {SetEffect_18(); });
	EffectTimerHandles[19].EffectDelegate.BindLambda([this]() {SetEffect_19(); });
	EffectTimerHandles[20].EffectDelegate.BindLambda([this]() {SetEffect_20(); });
	EffectTimerHandles[21].EffectDelegate.BindLambda([this]() {SetEffect_21(); });
	EffectTimerHandles[22].EffectDelegate.BindLambda([this]() {SetEffect_22(); });
	EffectTimerHandles[23].EffectDelegate.BindLambda([this]() {SetEffect_23(); });
	EffectTimerHandles[24].EffectDelegate.BindLambda([this]() {SetEffect_24(); });
	EffectTimerHandles[25].EffectDelegate.BindLambda([this]() {SetEffect_25(); });
	EffectTimerHandles[26].EffectDelegate.BindLambda([this]() {SetEffect_26(); });
	EffectTimerHandles[27].EffectDelegate.BindLambda([this]() {SetEffect_27(); });
	EffectTimerHandles[28].EffectDelegate.BindLambda([this]() {SetEffect_28(); });
	EffectTimerHandles[29].EffectDelegate.BindLambda([this]() {SetEffect_29(); });
	EffectTimerHandles[30].EffectDelegate.BindLambda([this]() {SetEffect_30(); });
	EffectTimerHandles[31].EffectDelegate.BindLambda([this]() {SetEffect_31(); });
	EffectTimerHandles[32].EffectDelegate.BindLambda([this]() {SetEffect_32(); });
	EffectTimerHandles[33].EffectDelegate.BindLambda([this]() {SetEffect_33(); });
	EffectTimerHandles[34].EffectDelegate.BindLambda([this]() {SetEffect_34(); });
	EffectTimerHandles[35].EffectDelegate.BindLambda([this]() {SetEffect_35(); });
	EffectTimerHandles[36].EffectDelegate.BindLambda([this]() {SetEffect_36(); });
	EffectTimerHandles[37].EffectDelegate.BindLambda([this]() {SetEffect_37(); });
	EffectTimerHandles[38].EffectDelegate.BindLambda([this]() {SetEffect_38(); });
	EffectTimerHandles[39].EffectDelegate.BindLambda([this]() {SetEffect_39(); });
	EffectTimerHandles[40].EffectDelegate.BindLambda([this]() {SetEffect_40(); });
	EffectTimerHandles[41].EffectDelegate.BindLambda([this]() {SetEffect_41(); });
	EffectTimerHandles[42].EffectDelegate.BindLambda([this]() {SetEffect_42(); });
	EffectTimerHandles[43].EffectDelegate.BindLambda([this]() {SetEffect_43(); });
	EffectTimerHandles[44].EffectDelegate.BindLambda([this]() {SetEffect_44(); });
	EffectTimerHandles[45].EffectDelegate.BindLambda([this]() {SetEffect_45(); });
	EffectTimerHandles[46].EffectDelegate.BindLambda([this]() {SetEffect_46(); });
	EffectTimerHandles[47].EffectDelegate.BindLambda([this]() {SetEffect_47(); });
	EffectTimerHandles[48].EffectDelegate.BindLambda([this]() {SetEffect_48(); });
	EffectTimerHandles[49].EffectDelegate.BindLambda([this]() {SetEffect_49(); });
	EffectTimerHandles[50].EffectDelegate.BindLambda([this]() {SetEffect_50(); });
	EffectTimerHandles[51].EffectDelegate.BindLambda([this]() {SetEffect_51(); });
	EffectTimerHandles[52].EffectDelegate.BindLambda([this]() {SetEffect_52(); });
	EffectTimerHandles[53].EffectDelegate.BindLambda([this]() {SetEffect_53(); });
	EffectTimerHandles[54].EffectDelegate.BindLambda([this]() {SetEffect_54(); });
	EffectTimerHandles[55].EffectDelegate.BindLambda([this]() {SetEffect_55(); });
}

void AAlsCharacterExample::InitializeFoodEffectMap()
{
	FoodEffectMap.Add(FoodEffectTags::Effect_1, [this](bool Apply) { SetEffect_1(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_2, [this](bool Apply) { SetEffect_2(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_3, [this](bool Apply) { SetEffect_3(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_4, [this](bool Apply) { SetEffect_4(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_5, [this](bool Apply) { SetEffect_5(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_6, [this](bool Apply) { SetEffect_6(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_7, [this](bool Apply) { SetEffect_7(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_8, [this](bool Apply) { SetEffect_8(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_9, [this](bool Apply) { SetEffect_9(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_10, [this](bool Apply) { SetEffect_10(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_11, [this](bool Apply) { SetEffect_11(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_12, [this](bool Apply) { SetEffect_12(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_13, [this](bool Apply) { SetEffect_13(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_14, [this](bool Apply) { SetEffect_14(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_15, [this](bool Apply) { SetEffect_15(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_16, [this](bool Apply) { SetEffect_16(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_17, [this](bool Apply) { SetEffect_17(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_18, [this](bool Apply) { SetEffect_18(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_19, [this](bool Apply) { SetEffect_19(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_20, [this](bool Apply) { SetEffect_20(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_21, [this](bool Apply) { SetEffect_21(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_22, [this](bool Apply) { SetEffect_22(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_23, [this](bool Apply) { SetEffect_23(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_24, [this](bool Apply) { SetEffect_24(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_25, [this](bool Apply) { SetEffect_25(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_26, [this](bool Apply) { SetEffect_26(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_27, [this](bool Apply) { SetEffect_27(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_28, [this](bool Apply) { SetEffect_28(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_29, [this](bool Apply) { SetEffect_29(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_30, [this](bool Apply) { SetEffect_30(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_31, [this](bool Apply) { SetEffect_31(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_32, [this](bool Apply) { SetEffect_32(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_33, [this](bool Apply) { SetEffect_33(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_34, [this](bool Apply) { SetEffect_34(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_35, [this](bool Apply) { SetEffect_35(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_36, [this](bool Apply) { SetEffect_36(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_37, [this](bool Apply) { SetEffect_37(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_38, [this](bool Apply) { SetEffect_38(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_39, [this](bool Apply) { SetEffect_39(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_40, [this](bool Apply) { SetEffect_40(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_41, [this](bool Apply) { SetEffect_41(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_42, [this](bool Apply) { SetEffect_42(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_43, [this](bool Apply) { SetEffect_43(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_44, [this](bool Apply) { SetEffect_44(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_45, [this](bool Apply) { SetEffect_45(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_46, [this](bool Apply) { SetEffect_46(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_47, [this](bool Apply) { SetEffect_47(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_48, [this](bool Apply) { SetEffect_48(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_49, [this](bool Apply) { SetEffect_49(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_50, [this](bool Apply) { SetEffect_50(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_51, [this](bool Apply) { SetEffect_51(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_52, [this](bool Apply) { SetEffect_52(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_53, [this](bool Apply) { SetEffect_53(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_54, [this](bool Apply) { SetEffect_54(Apply); });
	FoodEffectMap.Add(FoodEffectTags::Effect_55, [this](bool Apply) { SetEffect_55(Apply); });
}

void AAlsCharacterExample::FoodEffectByTag(const FGameplayTag& Tag, bool Apply)
{
	if (const TFunction<void(bool)>* Func = FoodEffectMap.Find(Tag))
	{
		(*Func)(Apply);
	}
}

void AAlsCharacterExample::SetEffect_1(bool Apply)
{
	if (Apply)
	{
		RecoilMultiplier_1 = 0.7f;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[1].EffectTimerHandle, EffectTimerHandles[1].EffectDelegate, 900.0f, false);
	}
	else
	{
		RecoilMultiplier_1 = 1.0f;
	}
}

void AAlsCharacterExample::SetEffect_2(bool Apply)
{
	if (Apply)
	{
		HealthAdd_25 = GetHealth() * 0.25f;
		SetHealth(GetHealth() + HealthAdd_25);

		GetWorldTimerManager().SetTimer(EffectTimerHandles[2].EffectTimerHandle, EffectTimerHandles[2].EffectDelegate, 300.0f, false);
	}
	else
	{
		SetHealth(GetHealth() - HealthAdd_25);
		HealthAdd_25 = 0.0f;
	}
}

void AAlsCharacterExample::SetEffect_3(bool Apply)
{
	if (Apply)
	{
		HealthRecoveryRate_50 = 1.5f;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[3].EffectTimerHandle, EffectTimerHandles[3].EffectDelegate, 900.0f, false);
	}
	else
	{
		HealthRecoveryRate_50 = 1.0f;
	}
}

void AAlsCharacterExample::SetEffect_4(bool Apply)
{
	if (Apply)
	{
		StaminaRecoveryRate_50 = 1.5f;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[4].EffectTimerHandle, EffectTimerHandles[4].EffectDelegate, 900.0f, false);
	}
	else
	{
		StaminaRecoveryRate_50 = 1.0f;
	}
}

void AAlsCharacterExample::SetEffect_5(bool Apply)
{
	if (Apply)
	{
		bShouldReplenish_50 = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[5].EffectTimerHandle, EffectTimerHandles[5].EffectDelegate, 900.0f, false);
	}
	else
	{
		bShouldReplenish_50 = false;
	}
}

void AAlsCharacterExample::SetEffect_6(bool Apply)
{
	if (Apply)
	{
		bIsStaminaHealthStandingMultiplierApplied = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[6].EffectTimerHandle, EffectTimerHandles[6].EffectDelegate, 900.0f, false);
	}
	else
	{
		bIsStaminaHealthStandingMultiplierApplied = false;
	}
}

void AAlsCharacterExample::SetEffect_7(bool Apply)
{
	if (Apply)
	{
		bIsStaminaHealthRunningMultiplierApplied = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[7].EffectTimerHandle, EffectTimerHandles[7].EffectDelegate, 900.0f, false);
	}
	else
	{
		bIsStaminaHealthRunningMultiplierApplied = false;
	}
}

void AAlsCharacterExample::SetEffect_8(bool Apply)
{
	if (Apply)
	{
		bIsAimPrecisionOnMoveApplied = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[8].EffectTimerHandle, EffectTimerHandles[8].EffectDelegate, 900.0f, false);
	}
	else
	{
		bIsAimPrecisionOnMoveApplied = false;
	}
}

void AAlsCharacterExample::SetEffect_9(bool Apply)
{
	if (Apply)
	{
		bShouldIgnoreBlindnessEffect = true;
		SetRemoveBlindness(false);

		GetWorldTimerManager().SetTimer(EffectTimerHandles[9].EffectTimerHandle, EffectTimerHandles[9].EffectDelegate, 900.0f, false);
	}
	else
	{
		bShouldIgnoreBlindnessEffect = false;
	}
}

void AAlsCharacterExample::SetEffect_10(bool Apply)
{
	if (Apply)
	{
		bShouldReduceStamina = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[10].EffectTimerHandle, EffectTimerHandles[10].EffectDelegate, 15.0f, false);
	}
	else
	{
		bShouldReduceStamina = false;
	}
}

void AAlsCharacterExample::SetEffect_11(bool Apply)
{
	if (Apply)
	{
		bIsHealthIsUnder_20 = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[11].EffectTimerHandle, EffectTimerHandles[11].EffectDelegate, 900.0f, false);
	}
	else
	{
		bIsHealthIsUnder_20 = false;
	}
}

void AAlsCharacterExample::SetEffect_12(bool Apply)
{
	if (Apply)
	{
		bIsStaminaIsUnder_30 = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[12].EffectTimerHandle, EffectTimerHandles[12].EffectDelegate, 900.0f, false);
	}
	else
	{
		bIsStaminaIsUnder_30 = false;
	}
}

void AAlsCharacterExample::SetEffect_13(bool Apply)
{
	if (Apply)
	{
		bIsDamagedOnMovingOrOnStanding = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[13].EffectTimerHandle, EffectTimerHandles[13].EffectDelegate, 900.0f, false);
	}
	else
	{
		bIsDamagedOnMovingOrOnStanding = false;
	}
}

void AAlsCharacterExample::SetEffect_14(bool Apply)
{
	SetStamina(GetStamina() - GetStamina() * 0.5f);
	SetHealth(GetHealth() + GetHealth() * 0.25f);
}

void AAlsCharacterExample::SetEffect_15(bool Apply)
{
	SetStamina(GetStamina() + GetStamina() * 0.5f);
	SetHealth(GetHealth() - GetHealth() * 0.25f);
}

void AAlsCharacterExample::SetEffect_16(bool Apply)
{
	if (Apply)
	{
		StaminaLossRate = 0.5f;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[16].EffectTimerHandle, EffectTimerHandles[16].EffectDelegate, 900.0f, false);
	}
	else
	{
		StaminaLossRate = 1.0f;
	}
}

void AAlsCharacterExample::SetEffect_17(bool Apply)
{
	if (Apply)
	{
		HealthLossRate = 0.5f;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[17].EffectTimerHandle, EffectTimerHandles[17].EffectDelegate, 900.0f, false);
	}
	else
	{
		HealthLossRate = 1.0f;
	}
}

void AAlsCharacterExample::SetEffect_18(bool Apply)
{
	if (Apply)
	{
		HigherJumpBy_40 = 1.4f;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[18].EffectTimerHandle, EffectTimerHandles[18].EffectDelegate, 900.0f, false);
	}
	else
	{
		HigherJumpBy_40 = 1.0f;
	}

	RefreshJumpZVelocity();
}

void AAlsCharacterExample::SetEffect_19(bool Apply)
{
	if (Apply)
	{
		FasterRollRate = 1.5f;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[19].EffectTimerHandle, EffectTimerHandles[19].EffectDelegate, 900.0f, false);
	}
	else
	{
		FasterRollRate = 1.0f;
	}
}

void AAlsCharacterExample::SetEffect_20(bool Apply)
{
	if (Apply)
	{
		JumpStaminaCost *= 0.8f;
		RollStaminaCost *= 0.8f;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[20].EffectTimerHandle, EffectTimerHandles[20].EffectDelegate, 900.0f, false);
	}
	else
	{
		JumpStaminaCost *= 1.25f;
		RollStaminaCost *= 1.25f;
	}
}

void AAlsCharacterExample::SetEffect_21(bool Apply)
{
	if (Apply)
	{
		bShouldIgnoreDamageOnRoll = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[21].EffectTimerHandle, EffectTimerHandles[21].EffectDelegate, 60.0f, false);
	}
	else
	{
		bShouldIgnoreDamageOnRoll = false;
	}
}

void AAlsCharacterExample::SetEffect_22(bool Apply)
{
	if (Apply)
	{
		bShouldIgnoreStun = true;
		bIsStunned = false;
		StunRecoveryMultiplier = 1.0f;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[22].EffectTimerHandle, EffectTimerHandles[22].EffectDelegate, 180.0f, false);
	}
	else
	{
		bShouldIgnoreStun = false;
	}
}

void AAlsCharacterExample::SetEffect_23(bool Apply)
{
	if (Apply)
	{
		bShouldIgnoreDamage = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[23].EffectTimerHandle, EffectTimerHandles[23].EffectDelegate, 20.0f, false);
	}
	else
	{
		bShouldIgnoreDamage = false;
	}
}

void AAlsCharacterExample::SetEffect_24(bool Apply)
{
	if (Apply)
	{
		bShouldReduceDamageMelee = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[24].EffectTimerHandle, EffectTimerHandles[24].EffectDelegate, 20.0f, false);
	}
	else
	{
		bShouldReduceDamageMelee = false;
	}
}

void AAlsCharacterExample::SetEffect_25(bool Apply)
{
	if (Apply)
	{
		bShouldReduceDamageProjectile = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[25].EffectTimerHandle, EffectTimerHandles[25].EffectDelegate, 20.0f, false);
	}
	else
	{
		bShouldReduceDamageProjectile = false;
	}
}

void AAlsCharacterExample::SetEffect_26(bool Apply)
{
	if (Apply)
	{
		bAimAccuracyOnStrafing_30 = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[26].EffectTimerHandle, EffectTimerHandles[26].EffectDelegate, 900.0f, false);
	}
	else
	{
		bAimAccuracyOnStrafing_30 = false;
	}
}

void AAlsCharacterExample::SetEffect_27(bool Apply)
{
	if (Apply)
	{
		bAimAccuracyOnWalking_30 = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[27].EffectTimerHandle, EffectTimerHandles[27].EffectDelegate, 900.0f, false);
	}
	else
	{
		bAimAccuracyOnWalking_30 = false;
	}
}

void AAlsCharacterExample::SetEffect_28(bool Apply)
{
	if (Apply)
	{
		bShouldIgnoreArmLock = true;
		if (bArmLockEffectIsActive)
		{
			SetArmLockEffect(false, false);
		}

		GetWorldTimerManager().SetTimer(EffectTimerHandles[28].EffectTimerHandle, EffectTimerHandles[28].EffectDelegate, 120.0f, false);
	}
	else
	{
		bShouldIgnoreArmLock = false;
		if (bArmLockEffectIsActive)
		{
			SetArmLockEffect(true, false);
		}
	}
}

void AAlsCharacterExample::SetEffect_29(bool Apply)
{
	if (Apply)
	{
		bShouldConvertDamageToStamina_30 = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[29].EffectTimerHandle, EffectTimerHandles[29].EffectDelegate, 900.0f, false);
	}
	else
	{
		bShouldConvertDamageToStamina_30 = false;
	}
}

void AAlsCharacterExample::SetEffect_30(bool Apply)
{
	if (Apply)
	{
		bIsLastStandActive = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[30].EffectTimerHandle, EffectTimerHandles[30].EffectDelegate, 900.0f, false);
	}
	else
	{
		bIsLastStandActive = false;
	}
}

void AAlsCharacterExample::SetEffect_31(bool Apply)
{
	for (FEffectTimer& EffectTimer : EffectTimerHandles)
	{
		if (GetWorldTimerManager().IsTimerActive(EffectTimer.EffectTimerHandle))
		{
			float RemainingTime = GetWorldTimerManager().GetTimerRemaining(EffectTimer.EffectTimerHandle);
			RemainingTime *= 1.5f;

			GetWorldTimerManager().ClearTimer(EffectTimer.EffectTimerHandle);
			GetWorldTimerManager().SetTimer(EffectTimer.EffectTimerHandle, EffectTimer.EffectDelegate, RemainingTime, false);
		}
	}
}

void AAlsCharacterExample::SetEffect_32(bool Apply)
{
	if (Apply)
	{
		bShouldIgnoreEnemyAbilityEffect = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[32].EffectTimerHandle, EffectTimerHandles[32].EffectDelegate, 180.0f, false);
	}
	else
	{
		bShouldIgnoreEnemyAbilityEffect = false;
	}
}

void AAlsCharacterExample::SetEffect_33(bool Apply)
{
	if (Apply)
	{
		bShouldIgnorePainAndLowStamina = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[33].EffectTimerHandle, EffectTimerHandles[33].EffectDelegate, 900.0f, false);
	}
	else
	{
		bShouldIgnorePainAndLowStamina = false;
	}
}

void AAlsCharacterExample::SetEffect_34(bool Apply)
{
	if (Apply)
	{
		bShouldIgnoreJitterynessShockEffect = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[34].EffectTimerHandle, EffectTimerHandles[34].EffectDelegate, 180.0f, false);
	}
	else
	{
		bShouldIgnoreJitterynessShockEffect = false;
	}
}

void AAlsCharacterExample::SetEffect_35(bool Apply)
{
	if (Apply)
	{
		bShouldIncreaseWalkAndRunSpeed = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[35].EffectTimerHandle, EffectTimerHandles[35].EffectDelegate, 900.0f, false);
	}
	else
	{
		bShouldIncreaseWalkAndRunSpeed = false;
	}
}

void AAlsCharacterExample::SetEffect_36(bool Apply)
{
	if (Apply)
	{
		bShouldDecreaseWalkRunSpeedAndDamage = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[36].EffectTimerHandle, EffectTimerHandles[36].EffectDelegate, 900.0f, false);
	}
	else
	{
		bShouldDecreaseWalkRunSpeedAndDamage = false;
	}
}

void AAlsCharacterExample::SetEffect_37(bool Apply)
{
	if (!bShouldWaitToUseEffect_20 || bShouldResetWaitToUseEffect_20)
	{
		SetHealth(GetHealth() + 15.0f);
		bShouldWaitToUseEffect_20 = true;

		GetWorldTimerManager().SetTimer(WaitEffectTimerHandle, [&]()
			{
				bShouldWaitToUseEffect_20 = false;
				//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, "Can eat");
			}, 20.0f, false);
	}
}

void AAlsCharacterExample::SetEffect_38(bool Apply)
{
	if (Apply)
	{
		bShouldIncreaseHealth_30 = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[38].EffectTimerHandle, EffectTimerHandles[38].EffectDelegate, 20.0f, false);
	}
	else
	{
		bShouldIncreaseHealth_30 = false;
	}
}

void AAlsCharacterExample::SetEffect_39(bool Apply)
{
	if (Apply)
	{
		bShouldResetWaitToUseEffect_20 = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[39].EffectTimerHandle, EffectTimerHandles[39].EffectDelegate, 900.0f, false);
	}
	else
	{
		bShouldResetWaitToUseEffect_20 = false;
	}
}

void AAlsCharacterExample::SetEffect_40(bool Apply)
{
	if (Apply)
	{
		AimAccuracy_50 = 0.5f;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[40].EffectTimerHandle, EffectTimerHandles[40].EffectDelegate, 15.0f, false);
	}
	else
	{
		AimAccuracy_50 = 1.0f;
	}
}

void AAlsCharacterExample::SetEffect_41(bool Apply)
{
	if (Apply)
	{
		bIsSetEffect_41 = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[41].EffectTimerHandle, EffectTimerHandles[41].EffectDelegate, 900.0f, false);
	}
	else
	{
		bIsSetEffect_41 = false;
	}
}

void AAlsCharacterExample::SetEffect_42(bool Apply)
{
	if (Apply)
	{
		bShouldIgnoreFallDamageAndStun = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[42].EffectTimerHandle, EffectTimerHandles[42].EffectDelegate, 900.0f, false);
	}
	else
	{
		bShouldIgnoreFallDamageAndStun = false;
	}
}

void AAlsCharacterExample::SetEffect_43(bool Apply)
{
	if (Apply)
	{
		bShouldIncreaseSpeedIfStaminaLess_70 = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[43].EffectTimerHandle, EffectTimerHandles[43].EffectDelegate, 900.0f, false);
	}
	else
	{
		bShouldIncreaseSpeedIfStaminaLess_70 = false;
	}
}

void AAlsCharacterExample::SetEffect_44(bool Apply)
{
	if (Apply)
	{
		bIsSetEffect_44 = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[44].EffectTimerHandle, EffectTimerHandles[44].EffectDelegate, 900.0f, false);
	}
	else
	{
		bIsSetEffect_44 = false;
	}
}

void AAlsCharacterExample::SetEffect_45(bool Apply)
{
	if (Apply)
	{
		bIsSetEffect_45 = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[45].EffectTimerHandle, EffectTimerHandles[45].EffectDelegate, 900.0f, false);
	}
	else
	{
		bIsSetEffect_45 = false;
	}
}

void AAlsCharacterExample::SetEffect_46(bool Apply)
{
	if (Apply)
	{
		bIsSetEffect_46 = true;

		GetWorldTimerManager().SetTimer(EffectTimerHandles[46].EffectTimerHandle, EffectTimerHandles[46].EffectDelegate, 900.0f, false);
	}
	else
	{
		bIsSetEffect_46 = false;
	}
}

void AAlsCharacterExample::SetEffect_47(bool Apply)
{
	if (Apply)
	{
		bIsSetEffect_47 = true;

		CheckIfOnCrouchShouldReduceDamage();

		GetWorldTimerManager().SetTimer(EffectTimerHandles[46].EffectTimerHandle, EffectTimerHandles[46].EffectDelegate, 240.0f, false);
	}
	else
	{
		bIsSetEffect_47 = false;
	}
}

void AAlsCharacterExample::SetEffect_48(bool Apply)
{
}

void AAlsCharacterExample::SetEffect_49(bool Apply)
{
}

void AAlsCharacterExample::SetEffect_50(bool Apply)
{
}

void AAlsCharacterExample::SetEffect_51(bool Apply)
{
}

void AAlsCharacterExample::SetEffect_52(bool Apply)
{
}

void AAlsCharacterExample::SetEffect_53(bool Apply)
{
}

void AAlsCharacterExample::SetEffect_54(bool Apply)
{
}

void AAlsCharacterExample::SetEffect_55(bool Apply)
{
}
