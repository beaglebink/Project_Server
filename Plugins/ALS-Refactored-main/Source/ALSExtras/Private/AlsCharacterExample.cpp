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

#include UE_INLINE_GENERATED_CPP_BY_NAME(AlsCharacterExample)

AAlsCharacterExample::AAlsCharacterExample()
{
	Camera = CreateDefaultSubobject<UAlsCameraComponent>(FName{ TEXTVIEW("Camera") });
	Camera->SetupAttachment(GetMesh());
	Camera->SetRelativeRotation_Direct({ 0.0f, 90.0f, 0.0f });

	//PhysicsConstraint = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("PhysicsConstraint"));
	//PhysicsConstraint->SetupAttachment(RootComponent);
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

	auto* EnhancedInput{ Cast<UEnhancedInputComponent>(Input) };
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
	}
}

void AAlsCharacterExample::Input_OnLookMouse(const FInputActionValue& ActionValue)
{
	LoopEffectFrame.FrameActionValue_OnLookMouse = ActionValue;

	const auto Value{ ActionValue.Get<FVector2D>() };
	ShakeMouseRemoveWireEffect(Value);

	if (!bIsStunned)
	{
		if (bIsDiscombobulated)
		{
			float PitchDirection = Value.Y * LookUpMouseSensitivity * StunRecoveryMultiplier * WireEffectPower_01Range;
			float YawDirection = Value.X * LookRightMouseSensitivity * StunRecoveryMultiplier * WireEffectPower_01Range;
			FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimer(TimerHandle, [this, PitchDirection, YawDirection]()
				{
					AddControllerPitchInput(PitchDirection);
					AddControllerYawInput(YawDirection);
				}, DiscombobulateTimeDelay, false);
		}
		else
		{
			AddControllerPitchInput(Value.Y * LookUpMouseSensitivity * StunRecoveryMultiplier * WireEffectPower_01Range);
			AddControllerYawInput(Value.X * LookRightMouseSensitivity * StunRecoveryMultiplier * WireEffectPower_01Range);
		}
	}
}

void AAlsCharacterExample::Input_OnLook(const FInputActionValue& ActionValue)
{
	LoopEffectFrame.FrameActionValue_OnLook = ActionValue;

	const auto Value{ ActionValue.Get<FVector2D>() };

	if (!bIsStunned)
	{
		if (bIsDiscombobulated)
		{
			float PitchDirection = Value.Y * LookUpRate * StunRecoveryMultiplier * WireEffectPower_01Range;
			float YawDirection = Value.X * LookRightRate * StunRecoveryMultiplier * WireEffectPower_01Range;
			FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimer(TimerHandle, [this, PitchDirection, YawDirection]()
				{
					AddControllerPitchInput(PitchDirection);
					AddControllerYawInput(YawDirection);
				}, DiscombobulateTimeDelay, false);
		}
		else
		{
			AddControllerPitchInput(Value.Y * LookUpRate * StunRecoveryMultiplier * WireEffectPower_01Range);
			AddControllerYawInput(Value.X * LookRightRate * StunRecoveryMultiplier * WireEffectPower_01Range);
		}
	}
}

void AAlsCharacterExample::Input_OnMove(const FInputActionValue& ActionValue)
{
	LoopEffectFrame.FrameActionValue_OnMove = ActionValue;

	const auto Value{ UAlsMath::ClampMagnitude012D(ActionValue.Get<FVector2D>()) };

	LastInputDirection = Value;

	const auto ForwardDirection{ UAlsMath::AngleToDirectionXY(UE_REAL_TO_FLOAT(GetViewState().Rotation.Yaw)) };
	const auto RightDirection{ UAlsMath::PerpendicularCounterClockwiseXY(ForwardDirection) };
	if (!bIsStunned && !bIsSliding && !bIsStickyStuck)
	{
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
}

void AAlsCharacterExample::Input_OnMove_Released()
{
	RemoveSticknessByMash();
}

void AAlsCharacterExample::Input_OnSprint(const FInputActionValue& ActionValue)
{
	FGameplayTag GaitTag = ActionValue.Get<bool>() ? AlsGaitTags::Sprinting : AlsGaitTags::Running;

	auto StartSprintLambda = [this, GaitTag]()
		{
			if (GetStamina() > SprintStaminaDrainRate && AbleToSprint)
			{
				if (GetDesiredGait() == AlsGaitTags::Sprinting && GaitTag == AlsGaitTags::Sprinting)
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

	if (!bIsStunned && !bIsSliding && !bIsSticky)
	{
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
}

void AAlsCharacterExample::Input_OnWalk()
{
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

	if (bIsStunned || bIsSliding || bIsStickyStuck || bIsWired)
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
	LoopEffectFrame.FrameState = EnumLoopStates::Roll;

	static constexpr auto PlayRate{ 1.3f };

	if (GetStamina() > RollStaminaCost && !bIsStunned && !bIsSliding && !bIsSticky)
	{
		if (bIsDiscombobulated)
		{
			FTimerHandle TimerHandleDiscombobulate;
			GetWorldTimerManager().SetTimer(TimerHandleDiscombobulate, [this]()
				{
					StartRolling(PlayRate);
				}, DiscombobulateTimeDelay, false);
		}
		else
		{
			StartRolling(PlayRate);
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
	SetViewMode(GetViewMode() == AlsViewModeTags::ThirdPerson ? AlsViewModeTags::FirstPerson : AlsViewModeTags::ThirdPerson);
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

	SetStamina(GetStamina() + StaminaRegenerationRate);

	LoopEffect();
}

void AAlsCharacterExample::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	InitStatWidget();
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

void AAlsCharacterExample::LoopEffect()
{
	if (bIsLooped)
	{
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