#include "Cooking/A_Dishes.h"
#include "AlsCharacterExample.h"
#include "Components/SphereComponent.h"
#include "Cooking/A_Cookable.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "Cooking/DA_FluidPoints.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/SpringArmComponent.h"
#include "Cooking/W_PourEvent.h"
#include "Components/AudioComponent.h"
#include "InteractiveItemComponent.h"

AA_Dishes::AA_Dishes()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionShape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CollisionShape"));
	CheckForPlateSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CheckForPlateSphere"));
	LiquidShape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LiquidShape"));
	CookedResultSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CookedResultSpawnPoint"));
	LiquidLevelPoint = CreateDefaultSubobject<USceneComponent>(TEXT("LiquidLevelPoint"));
	FullLiquidLevelPoint = CreateDefaultSubobject<USceneComponent>(TEXT("FullLiquidLevelPoint"));
	CentreLiquidLevelPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CentreLiquidLevelPoint"));
	EmptyLiquidLevelPoint = CreateDefaultSubobject<USceneComponent>(TEXT("EmptyLiquidLevelPoint"));
	SpringArmToEdge = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmToEdgePoint"));
	EdgeLiquidPoint = CreateDefaultSubobject<USceneComponent>(TEXT("EdgeLiquidPoint"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	DishWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("DishWidget"));
	FluidFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FluidFX"));
	PourFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PourFX"));

	CollisionShape->SetupAttachment(RootComponent);
	CheckForPlateSphere->SetupAttachment(RootComponent);
	LiquidShape->SetupAttachment(RootComponent);
	CookedResultSpawnPoint->SetupAttachment(RootComponent);
	LiquidLevelPoint->SetupAttachment(RootComponent);
	FullLiquidLevelPoint->SetupAttachment(RootComponent);
	CentreLiquidLevelPoint->SetupAttachment(RootComponent);
	EmptyLiquidLevelPoint->SetupAttachment(RootComponent);
	SpringArmToEdge->SetupAttachment(RootComponent);
	EdgeLiquidPoint->SetupAttachment(SpringArmToEdge);
	AudioComponent->SetupAttachment(RootComponent);
	DishWidget->SetupAttachment(RootComponent);
	FluidFX->SetupAttachment(LiquidShape);
	PourFX->SetupAttachment(EdgeLiquidPoint);

	CollisionShape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionShape->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	CollisionShape->bHiddenInGame = true;

	LiquidShape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	LiquidShape->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	LiquidShape->SetCollisionResponseToChannel(ECC_GameTraceChannel10, ECollisionResponse::ECR_Block);
	LiquidShape->bHiddenInGame = true;

	SpringArmToEdge->bDoCollisionTest = false;

	AudioComponent->bAutoActivate = false;

	DishWidget->SetWidgetSpace(EWidgetSpace::Screen);
	DishWidget->SetDrawSize(FVector2D(200.0f, 400.0f));

	FluidFX->SetAutoActivate(false);
	PourFX->SetAutoActivate(false);
}

void AA_Dishes::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (LiquidType != ELiquidType::None)
	{
		if (!FluidFX->IsActive())
		{
			FluidFX->Activate();
		}

		// Initialize Liquid
		FluidFX->SetVariableLinearColor(FName(TEXT("User.LiquidColor")), LiquidColor);
		PourFX->SetVariableLinearColor(FName(TEXT("User.LiquidColor")), LiquidColor);
		PourFX->SetVariableFloat(FName(TEXT("User.PourVelocityValue")), PourVelocityValue);
		PourFX->SetVariableFloat(FName(TEXT("User.SphereLocationRadius")), SphereLocationRadius);
		FullLiquidVolume = LiquidVolume;
		BoundsRadius = StaticMesh->Bounds.SphereRadius;
		LiquidLevelPoint->SetWorldLocation(FullLiquidVolume == 0.0f ? EmptyLiquidLevelPoint->GetComponentLocation() : FullLiquidLevelPoint->GetComponentLocation());

		// Update liquid level
		FluidFX->SetVariableVec3(FName(TEXT("User.LiquidLevelPoint")), LiquidLevelPoint->GetComponentLocation());
		FluidFX->SetVariableVec3(FName(TEXT("User.CutPlaneNormal")), CutPlaneNormal);
	}
	else
	{
		FluidFX->Deactivate();
	}
}

void AA_Dishes::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PauseTimeOnRotation = FMath::Clamp(PauseTimeOnRotation - DeltaTime, 0.0f, DefaultPauseTimeOnRotation);

	// Dishes attach to player
	if (AttachedMesh)
	{
		if (PauseTimeOnRotation <= 0.0f)
		{
			FQuat NewQuat = FQuat::Slerp(GetActorQuat(), FRotator(0.f, AttachedMesh->GetComponentRotation().Yaw, 0.f).Quaternion(), DeltaTime * 0.5f);
			SetActorRotation(NewQuat);

			CurrentTiltAngle = FMath::FInterpTo(CurrentTiltAngle, 0.0f, DeltaTime, 0.5f);
			PreviousTiltAngle = CurrentTiltAngle;
		}

		if (AttachedMesh->DoesSocketExist(AttachedSocketName))
		{
			SetActorLocation(FMath::VInterpTo(GetActorLocation(), AttachedMesh->GetSocketLocation(AttachedSocketName), DeltaTime, 5.0f));
		}
	}

	// Dishes places on surface
	if (bIsPlacing)
	{
		OnPlacingPauseCheckTime += GetWorld()->GetDeltaSeconds();
		if (OnPlacingPauseCheckTime > 0.5f && StaticMesh->GetPhysicsLinearVelocity().Length() < 2.0f && StaticMesh->GetPhysicsAngularVelocityInDegrees().Length() < 2.0f)
		{
			StaticMesh->SetSimulatePhysics(false);
			bIsPlacing = false;
			OnPlacingPauseCheckTime = 0.0f;
		}
	}

	//Check if tossed
	DeltaLocation = GetActorLocation() - PrevLocation;
	PrevLocation = GetActorLocation();

	float DeltaLength = DeltaLocation.Length();
	float DirectionCheck = FVector::DotProduct(DeltaLocation.GetSafeNormal(), FVector(0.0f, 0.0f, 1.0f));
	if (DeltaLength > 1.0f && DirectionCheck > 0.9f)
	{
		DeltaLengthAccum += DeltaLength;
	}
	else if (DeltaLengthAccum >= 15.0f)
	{
		for (AA_Cookable* Ingredient : Ingredients)
		{
			Ingredient->bWasTossed = true;
		}
		if (bShowCookingDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Tossed")));
		}
		DeltaLengthAccum = 0.0f;
	}
	else
	{
		DeltaLengthAccum = 0.0f;
	}


	//Find lowest edge point to pour liquid
	SpringArmToEdge->SetWorldRotation((GetActorUpVector().Cross(FVector(0.0f, 0.0f, 1.0f)).Cross(GetActorUpVector()) * (-1.0f)).Rotation());

	// Update liquid level
	if (LiquidType != ELiquidType::None)
	{
		FluidFX->SetVariableVec3(FName(TEXT("User.LiquidLevelPoint")), LiquidLevelPoint->GetComponentLocation());
		FluidFX->SetVariableVec3(FName(TEXT("User.CutPlaneNormal")), CutPlaneNormal);

		PourLiquid(DeltaTime);
	}
}

void AA_Dishes::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			CollisionShape->GetOverlappingActors(OverlappingActors, AA_Cookable::StaticClass());

			// Check if ingredient doesn't belong any dish
			for (AActor* Ingredient : OverlappingActors.Array())
			{
				if (AA_Cookable* CookableIngredient = Cast<AA_Cookable>(Ingredient))
				{
					if (!Ingredients.Contains(CookableIngredient))
					{
						Ingredients.Add(CookableIngredient);
						if (CookableIngredient->PrevParentDish != this)
						{
							CookableIngredient->bWasTossed = false;
						}
						CookableIngredient->PrevParentDish = this;
						CookableIngredient->bIsInsideADish = true;

						int32& CountRef = IngredientCountMap.FindOrAdd(CookableIngredient->Name);
						++CountRef;

						if (bShowCookingDebug)
						{
							GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Added ingredient: %s, Count: %d"), *CookableIngredient->Name.ToString(), CountRef));
						}
					}
				}
			}

			// Check if ingredient is out of dish
			for (int32 i = Ingredients.Num() - 1; i >= 0; --i)
			{
				if (!OverlappingActors.Contains(Ingredients[i]))
				{
					Ingredients[i]->bIsInsideADish = false;
					int32* CountPtr = IngredientCountMap.Find(Ingredients[i]->Name);
					if (CountPtr)
					{
						--(*CountPtr);
						if (*CountPtr <= 0)
						{
							IngredientCountMap.Remove(Ingredients[i]->Name);
						}
					}

					if (bShowCookingDebug)
					{
						GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Removed ingredient: %s, Count: %d"), *Ingredients[i]->Name.ToString(), *CountPtr));
					}

					Ingredients.RemoveAt(i);
				}
			}

			UpdateCookingSession();

			//Check for plate
			Plate = nullptr;
			if (IsACookWare && Ingredients.Num() > 0)
			{
				OverlappingPlates.Empty();
				CheckForPlateSphere->GetOverlappingActors(OverlappingPlates, AA_Dishes::StaticClass());

				float Distance = static_cast<float>(INT_MAX);
				for (AActor* DishActor : OverlappingPlates)
				{
					if (AA_Dishes* PlateActor = Cast<AA_Dishes>(DishActor))
					{
						if (PlateActor->Name == FName(TEXT("Plate")) && PlateActor->Ingredients.Num() == 0)
						{
							float DistanceToPlate = FVector::Dist(GetActorLocation(), PlateActor->GetActorLocation());
							if (DistanceToPlate < Distance)
							{
								Distance = DistanceToPlate;
								Plate = PlateActor;
							}
						}
					}
				}
			}

			bCurrentPlateState = (Plate != nullptr);
			if (UInteractiveItemComponent* InteractiveItemComp = Cast<UInteractiveItemComponent>(GetComponentByClass(UInteractiveItemComponent::StaticClass())))
			{
				if (!bPrevPlateState && bCurrentPlateState)
				{
					PrevTooltipText = InteractiveItemComp->GetTooltip();
					InteractiveItemComp->SetTooltip(CheckIfCooked() ? FText::FromString(TEXT("To place food on plate, press F.")) : FText::FromString(TEXT("Recipe is failed or incomplete.")));
				}
				else if (bPrevPlateState && !bCurrentPlateState)
				{
					InteractiveItemComp->SetTooltip(PrevTooltipText);
				}
			}
			bPrevPlateState = bCurrentPlateState;
		}, 0.5f, true);

	//Fluid visual
	if (LiquidType != ELiquidType::None)
	{
		FluidFX->Activate();
	}
	else
	{
		FluidFX->Deactivate();
	}
}

void AA_Dishes::Destroyed()
{
	Super::Destroyed();

}

void AA_Dishes::PourLiquid(float DeltaTime)
{
	PourIntensityNormalized = FMath::GetMappedRangeValueClamped(FVector2D(0.0f, BoundsRadius * 2.0f), FVector2D(0.0f, 1.0f), LiquidLevelPoint->GetComponentLocation().Z - EdgeLiquidPoint->GetComponentLocation().Z);
	if (PourIntensityNormalized > 0.0f && LiquidLevelNormalized > 0.0f)
	{
		FHitResult UpHitResult, DownHitResult;
		GetWorld()->LineTraceSingleByChannel(UpHitResult, CentreLiquidLevelPoint->GetComponentLocation(), CentreLiquidLevelPoint->GetComponentLocation() + FVector::UpVector * BoundsRadius * 2.0f, ECC_GameTraceChannel11);
		GetWorld()->LineTraceSingleByChannel(DownHitResult, CentreLiquidLevelPoint->GetComponentLocation(), CentreLiquidLevelPoint->GetComponentLocation() + FVector::DownVector * BoundsRadius * 2.0f, ECC_GameTraceChannel11);
		if (UpHitResult.bBlockingHit)
		{
			FullLiquidLevelPoint->SetWorldLocation(UpHitResult.Location);
		}
		if (DownHitResult.bBlockingHit)
		{
			EmptyLiquidLevelPoint->SetWorldLocation(DownHitResult.Location);
		}

		LiquidLevelNormalized -= PourIntensityNormalized * LiquidPourRateNormalized * DeltaTime;
		LiquidVolume = FMath::Lerp(0.0f, FullLiquidVolume, LiquidLevelNormalized);
		LiquidLevelPoint->SetWorldLocation(FMath::Lerp(EmptyLiquidLevelPoint->GetComponentLocation(), FullLiquidLevelPoint->GetComponentLocation(), LiquidLevelNormalized));

		//Check if liquid gets into dish
		FPredictProjectilePathParams Params;

		Params.StartLocation = EdgeLiquidPoint->GetComponentLocation();
		Params.LaunchVelocity = EdgeLiquidPoint->GetUpVector() * PourVelocityValue * PourIntensityNormalized;
		Params.ProjectileRadius = SphereLocationRadius;
		Params.MaxSimTime = 1.0f;
		Params.SimFrequency = 15.0f;
		Params.bTraceWithCollision = false;
		Params.DrawDebugType = EDrawDebugTrace::None;

		FPredictProjectilePathResult Result;
		UGameplayStatics::PredictProjectilePath(this, Params, Result);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		FHitResult PourHitResult;
		for (int32 i = 1; i < Result.PathData.Num(); ++i)
		{
			FVector Start = Result.PathData[i - 1].Location;
			FVector End = Result.PathData[i].Location;

			GetWorld()->SweepSingleByChannel(PourHitResult, Start, End, FQuat::Identity, ECC_GameTraceChannel10, FCollisionShape::MakeSphere(Params.ProjectileRadius), QueryParams);
			if (PourHitResult.bBlockingHit)
			{
				if (AA_Dishes* RefillableDish = Cast<AA_Dishes>(PourHitResult.GetActor()))
				{
					RefillableDish->AddLiquid(LiquidType, FullLiquidVolume * PourIntensityNormalized * LiquidPourRateNormalized * DeltaTime);
					PourFX->SetVariableInt(FName(TEXT("User.HeatingLevel")), static_cast<int32>(RefillableDish->GetHeatingLevel()));
					break;
				}
			}
		}

		//GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Blue, FString::Printf(TEXT("PourIntensityNormalized: %f   LiquidLevelNormalized: %f   LiquidVolume: %f"), PourIntensityNormalized, LiquidLevelNormalized, LiquidVolume));

		if (!PourFX->IsActive())
		{
			PourFX->ReinitializeSystem();
			PourFX->Activate();
		}
	}

	// Update Niagara pour effect
	PourFX->SetVariableVec3(FName(TEXT("User.DishUpVector")), GetActorUpVector());
	PourFX->SetVariableFloat(FName(TEXT("User.PourIntensity")), PourIntensityNormalized);

	if (LiquidLevelNormalized <= 0.0f)
	{
		FluidFX->Deactivate();
		PourFX->Deactivate();
	}
}

void AA_Dishes::AttachDishToHand(ACharacter* PlayerCharacter, FName SocketName)
{
	if (!PlayerCharacter)
	{
		return;
	}

	AttachedMesh = PlayerCharacter->GetMesh();
	AttachedSocketName = SocketName;
	if (!AttachedMesh)
	{
		return;
	}

	StaticMesh->SetSimulatePhysics(false);
}

void AA_Dishes::DetachDishFromHand()
{
	bIsOnAttaching = false;
	AttachedMesh = nullptr;
	StaticMesh->SetSimulatePhysics(true);
}

void AA_Dishes::RotateDish(float AngleDelta)
{
	PauseTimeOnRotation = DefaultPauseTimeOnRotation;

	CurrentTiltAngle += AngleDelta * 5.0f;
	CurrentTiltAngle = FMath::Clamp(CurrentTiltAngle, -RotateAngle, 0.0f);
	float DeltaTilt = CurrentTiltAngle - PreviousTiltAngle;

	FQuat BaseQuat = GetActorQuat();
	FQuat DeltaQuat(-GetActorRightVector(), FMath::DegreesToRadians(DeltaTilt));

	SetActorRotation(DeltaQuat * BaseQuat);
	PreviousTiltAngle = CurrentTiltAngle;
}

void AA_Dishes::SetHeatingLevel(EHeatingLevel NewLevel)
{
	if (HeatingLevel == NewLevel)
	{
		return;
	}

	HeatingLevel = NewLevel;

	switch (HeatingLevel)
	{
	case EHeatingLevel::None:
	{
		GetWorldTimerManager().ClearTimer(CookingTimerHandle);
		break;
	}
	case EHeatingLevel::Low:
	case EHeatingLevel::Medium:
	case EHeatingLevel::High:
	{
		GetWorldTimerManager().SetTimer(CookingTimerHandle, [this]()
			{
				for (AA_Cookable* Ingredient : Ingredients)
				{
					Ingredient->IncreaseCookingTime(static_cast<int32>(HeatingLevel));
				}
			}, 1.0f, true);
		break;
	}
	default:
		break;
	}
}

EHeatingLevel AA_Dishes::GetHeatingLevel()
{
	return HeatingLevel;
}

bool AA_Dishes::CheckIfCooked()
{
	AAlsCharacterExample* PlayerCharacter = Cast<AAlsCharacterExample>(GetWorld()->GetFirstPlayerController()->GetPawn());
	if (bIsOnRecipeChecking || !PlayerCharacter || !PlayerCharacter->GetCurrentRecipe(CheckedRecipe))
	{
		return false;
	}

	bIsOnRecipeChecking = true;

	//Check for being tossed
	if (CheckedRecipe.bRequiresToss)
	{
		for (AA_Cookable* Ingredient : Ingredients)
		{
			if (!Ingredient->bWasTossed)
			{
				bIsOnRecipeChecking = false;
				return false;
			}
		}
	}

	TMap<FName, int32> RecipeMap;
	for (const FRecipeIngredient& Ingredient : CheckedRecipe.Ingredients)
	{
		RecipeMap.Add(Ingredient.IngredientName, Ingredient.IngredientQuantity);
	}

	//Check for recipe matching from recipe
	for (const auto& Pair : RecipeMap)
	{
		if (!IngredientCountMap.Contains(Pair.Key))
		{
			bIsOnRecipeChecking = false;
			return false;
		}
	}

	//Check for recipe matching - extra ingredients
	for (const auto& Pair : IngredientCountMap)
	{
		if (!RecipeMap.Contains(Pair.Key))
		{
			bIsOnRecipeChecking = false;
			return false;
		}
	}

	// Calculate chunks average quality
	for (AA_Cookable* Ingredient : Ingredients)
	{
		// Find the maximum chunk weight for each ingredient type
		FIngredientQuality& QualityRef = IngredientQualityMap.FindOrAdd(Ingredient->Name);
		QualityRef.MaxChunkWeight = FMath::Max(QualityRef.MaxChunkWeight, Ingredient->ChunkMass);
	}

	for (AA_Cookable* Ingredient : Ingredients)
	{
		// Calculate only significant chunks which weight is more than 10% of the maximum chunk weight for this ingredient type
		FIngredientQuality& QualityRef = IngredientQualityMap.FindOrAdd(Ingredient->Name);
		if (Ingredient->ChunkMass >= CheckedRecipe.SignificantChunkPercentage * QualityRef.MaxChunkWeight)
		{
			QualityRef.TotalQualityByWeight += Ingredient->GetChunkQuality() * Ingredient->ChunkMass;
			QualityRef.ChunksWeightTotal += Ingredient->ChunkMass;
			QualityRef.WorstChunkQuality = FMath::Min(QualityRef.WorstChunkQuality, Ingredient->GetChunkQuality());
			QualityRef.WorstChunkInfluence = Ingredient->WorstChunkInfluence;
			QualityRef.RecipeImportance = Ingredient->RecipeImportance;
		}
	}

	for (auto& Pair : IngredientQualityMap)
	{
		Pair.Value.GroupQuality = (Pair.Value.TotalQualityByWeight / Pair.Value.ChunksWeightTotal) * (1 - Pair.Value.WorstChunkInfluence + Pair.Value.WorstChunkInfluence * Pair.Value.WorstChunkQuality);
		Pair.Value.FailureSeverity = (1 - Pair.Value.GroupQuality) * Pair.Value.RecipeImportance;
	}

	// Calculate total dish quality
	DishQuality = 0.0f;
	float TotalRecipeImportance = 0.0f;
	LowestGroupQuality = 1.0f;
	for (const auto& Pair : IngredientQualityMap)
	{
		DishQuality += Pair.Value.GroupQuality * Pair.Value.RecipeImportance;
		TotalRecipeImportance += Pair.Value.RecipeImportance;
		if (Pair.Value.FailureSeverity >= CheckedRecipe.FailureSeverityThreshold)
		{
			LowestGroupQuality = FMath::Min(LowestGroupQuality, Pair.Value.GroupQuality);
		}
	}

	//LiquidSteps quality calculation
	for (int i = 0; i < LiquidStepsOnPour.Num(); ++i)
	{
		FLiquidStepOnPour& LiquidStepOnPour = LiquidStepsOnPour[i];
		if (!CheckedRecipe.LiquidSteps.IsValidIndex(i) || LiquidStepOnPour.PourEvents[0].LiquidType != CheckedRecipe.LiquidSteps[i].LiquidType)
		{
			LiquidStepOnPour.AmountQuality = 0.0f;
			LiquidStepOnPour.WeightedTimingQuality = 0.0f;
			LiquidStepOnPour.LiquidStepQuality = 0.0f;
			LiquidStepOnPour.WeightedLiquidContribution = 0.0f;
		}
		else
		{
			float AmountAddedPerStep = 0.0f;
			float Sum_PourAmount_PourTimingQuality = 0.0f;
			for (FPourEvent& PourEvent : LiquidStepOnPour.PourEvents)
			{
				//AmountQuality
				AmountAddedPerStep += PourEvent.AmountAdded;

				//TimingQuality
				PourEvent.TimingQuality = PourEvent.AmountAdded > 0.0f ? PourEvent.TimingScore / PourEvent.AmountAdded : 0.0f;

				Sum_PourAmount_PourTimingQuality += PourEvent.AmountAdded * PourEvent.TimingQuality;
			}

			if (AmountAddedPerStep >= CheckedRecipe.LiquidSteps[i].IdealMinimumAmount && AmountAddedPerStep <= CheckedRecipe.LiquidSteps[i].IdealMaximumAmount)
			{
				LiquidStepOnPour.AmountQuality = 1.0f;
			}
			else if (AmountAddedPerStep < CheckedRecipe.LiquidSteps[i].IdealMinimumAmount)
			{
				LiquidStepOnPour.AmountQuality = FMath::Clamp(1 - (CheckedRecipe.LiquidSteps[i].IdealMinimumAmount - AmountAddedPerStep) / CheckedRecipe.LiquidSteps[i].UnderAmountTolerance, 0.0f, 1.0f);
			}
			else if (AmountAddedPerStep > CheckedRecipe.LiquidSteps[i].IdealMaximumAmount)
			{
				LiquidStepOnPour.AmountQuality = FMath::Clamp(1 - (AmountAddedPerStep - CheckedRecipe.LiquidSteps[i].IdealMaximumAmount) / CheckedRecipe.LiquidSteps[i].OverAmountTolerance, 0.0f, 1.0f);
			}

			//WeightedTimingQuality
			LiquidStepOnPour.WeightedTimingQuality = Sum_PourAmount_PourTimingQuality / AmountAddedPerStep;

			//LiquidStepQuality
			LiquidStepOnPour.LiquidStepQuality = LiquidStepOnPour.AmountQuality * CheckedRecipe.LiquidSteps[i].AmountScoreWeight +
				LiquidStepOnPour.WeightedTimingQuality * CheckedRecipe.LiquidSteps[i].TimingScoreWeight;

			//WeightedLiquidContribution
			LiquidStepOnPour.WeightedLiquidContribution = LiquidStepOnPour.LiquidStepQuality * CheckedRecipe.LiquidSteps[i].RecipeImportance;
		}

		DishQuality += LiquidStepOnPour.WeightedLiquidContribution;
		TotalRecipeImportance += CheckedRecipe.LiquidSteps.IsValidIndex(i) ? CheckedRecipe.LiquidSteps[i].RecipeImportance : 1.0f;
	}

	DishQuality /= TotalRecipeImportance;
	DishQuality *= (0.5f + 0.5f * LowestGroupQuality); // Adjust dish quality based on the lowest group quality

	//Missed requared pieces calculation
	float WeightedShortageSeverity = 0.0f;
	MissingPieceDeduction = 0.0f;
	float SumShortageSeverity_RecipeImportance = 0.0f;
	for (const auto& Ingredient : CheckedRecipe.Ingredients)
	{
		if (FIngredientQuality* IngredientQuality = IngredientQualityMap.Find(Ingredient.IngredientName))
		{
			IngredientQuality->MissingProportion = 1.0f - static_cast<float>(FMath::Min(IngredientCountMap.FindRef(Ingredient.IngredientName), Ingredient.IngredientQuantity)) / static_cast<float>(Ingredient.IngredientQuantity);
			if (IngredientQuality->MissingProportion < 0.1f)
			{
				IngredientQuality->ShortageSeverity = 0.0f;
			}
			else if (IngredientQuality->MissingProportion < 0.2f)
			{
				IngredientQuality->ShortageSeverity = FMath::GetMappedRangeValueClamped(FVector2D(0.1f, 0.2f), FVector2D(0.0f, 0.2f), IngredientQuality->MissingProportion);
			}
			else if (IngredientQuality->MissingProportion < 0.4f)
			{
				IngredientQuality->ShortageSeverity = FMath::GetMappedRangeValueClamped(FVector2D(0.2f, 0.4f), FVector2D(0.2f, 0.5f), IngredientQuality->MissingProportion);
			}
			else if (IngredientQuality->MissingProportion < 0.6f)
			{
				IngredientQuality->ShortageSeverity = FMath::GetMappedRangeValueClamped(FVector2D(0.4f, 0.6f), FVector2D(0.5f, 0.75f), IngredientQuality->MissingProportion);
			}
			else
			{
				IngredientQuality->ShortageSeverity = FMath::GetMappedRangeValueClamped(FVector2D(0.6f, 1.0f), FVector2D(0.75f, 1.0f), IngredientQuality->MissingProportion);
			}
			SumShortageSeverity_RecipeImportance += IngredientQuality->ShortageSeverity * IngredientQuality->RecipeImportance;
		}
	}
	WeightedShortageSeverity = SumShortageSeverity_RecipeImportance / TotalRecipeImportance;
	MissingPieceDeduction = WeightedShortageSeverity * CheckedRecipe.MissingPenaltyStrength;

	DishAverage = DishQuality;
	DishQuality = FMath::Clamp(DishQuality - MissingPieceDeduction, 0.0f, 1.0f);

	bIsOnRecipeChecking = false;

	return true;
}

void AA_Dishes::ReplaceIngredientsByCookedFood()
{
	// Debug display
	if (bShowCookingDebug)
	{
		FString DebugText;

		for (const auto& Pair : IngredientQualityMap)
		{
			const float AverageQuality = Pair.Value.TotalQualityByWeight / Pair.Value.ChunksWeightTotal;

			DebugText += FString::Printf(
				TEXT("%s\n")
				TEXT("Average Quality: %.2f\n")
				TEXT("Worst Chunk: %.2f\n")
				TEXT("Group Quality: %.2f\n")
				TEXT("Importance: %.2f\n")
				TEXT("Failure Severity: %.2f\n\n")
				TEXT("Missing Proportion: %.2f\n")
				TEXT("ShortageSeverity: %.2f\n\n"),
				*Pair.Key.ToString(),
				AverageQuality,
				Pair.Value.WorstChunkQuality,
				Pair.Value.GroupQuality,
				Pair.Value.RecipeImportance,
				Pair.Value.FailureSeverity,
				Pair.Value.MissingProportion,
				Pair.Value.ShortageSeverity
			);
		}

		DebugText += TEXT("--------------------------------\n");
		DebugText += TEXT("LIQUID STEPS\n\n");

		for (int32 StepIndex = 0; StepIndex < LiquidStepsOnPour.Num(); ++StepIndex)
		{
			const FLiquidStepOnPour& LiquidStep = LiquidStepsOnPour[StepIndex];

			ELiquidType StepLiquidType = ELiquidType::None;
			if (LiquidStep.PourEvents.Num() > 0)
			{
				StepLiquidType = LiquidStep.PourEvents[0].LiquidType;
			}

			DebugText += FString::Printf(
				TEXT("Step %d\n")
				TEXT("PourEvents %d\n")
				TEXT("Liquid: %s\n")
				TEXT("Amount Quality: %.2f\n")
				TEXT("Timing Quality: %.2f\n")
				TEXT("Step Quality: %.2f\n")
				TEXT("Contribution: %.2f\n\n"),
				StepIndex + 1,
				LiquidStep.PourEvents.Num(),
				*StaticEnum<ELiquidType>()->GetDisplayNameTextByValue((int64)StepLiquidType).ToString(),
				LiquidStep.AmountQuality,
				LiquidStep.WeightedTimingQuality,
				LiquidStep.LiquidStepQuality,
				LiquidStep.WeightedLiquidContribution
			);
		}

		DebugText += FString::Printf(
			TEXT("--------------------------------\n")
			TEXT("Dish Average: %.2f\n")
			TEXT("Worst Important Ingredient: %.2f\n")
			TEXT("Missing Penalty: %.2f\n")
			TEXT("Final Quality: %.2f\n"),
			DishAverage,
			LowestGroupQuality,
			MissingPieceDeduction,
			DishQuality
		);

		GEngine->AddOnScreenDebugMessage(-1, 40.0f, FColor::Yellow, DebugText);
	}

	//If all conditions are ok, swap ingredients on result dish
	if (CheckedRecipe.ResultCookableClass)
	{
		for (AA_Cookable* Ingredient : Ingredients)
		{
			Ingredient->Destroy();
		}

		Ingredients.Empty();
		IngredientCountMap.Empty();

		AA_Cookable* NewDish = GetWorld()->SpawnActor<AA_Cookable>(CheckedRecipe.ResultCookableClass, Plate->CookedResultSpawnPoint->GetComponentLocation(), CookedResultSpawnPoint->GetComponentRotation());
		NewDish->CookingTime = static_cast<int32>(NewDish->DefaultCookingTime * DishQuality * 2.0f);
		NewDish->Quality = DishQuality;

		EDishRating FinalRating = NewDish->GetDishRating(DishQuality, CheckedRecipe.RatingThresholds);

		if (bShowCookingDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 40.0f, FColor::Green, FString::Printf(TEXT("Final Rating: %s"), *StaticEnum<EDishRating>()->GetDisplayNameTextByValue(static_cast<int64>(FinalRating)).ToString()));
		}

		NewDish->ShowFinalDishRating(FinalRating);
	}
	ResetCookingSession();
}

void AA_Dishes::UpdateNiagaraPreview()
{
	if (FluidPointsDataAsset)
	{
		UE_LOG(LogTemp, Log, TEXT("Updating Niagara preview with %d fluid points."), FluidPointsDataAsset->FluidPoints.LocalPositions.Num());

		FluidFX->SetVariableInt(FName(TEXT("User.FluidPointsQuantity")), FluidPointsDataAsset->FluidPoints.LocalPositions.Num());
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(FluidFX, TEXT("User.FluidPointsPositions"), FluidPointsDataAsset->FluidPoints.LocalPositions);
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(FluidFX, TEXT("User.FluidPointsDistancesToWall"), FluidPointsDataAsset->FluidPoints.DistancesToWall);
	}

	FluidFX->SetVariableVec3(FName(TEXT("User.LiquidLevelPoint")), LiquidLevelPoint->GetComponentLocation());
	FluidFX->SetVariableVec3(FName(TEXT("User.CutPlaneNormal")), CutPlaneNormal);

	FluidFX->ResetSystem();
}

void AA_Dishes::AddLiquid(ELiquidType Type, float Amount)
{
	CurrentPourTime = GetWorld()->GetTimeSeconds();
	float TimeBetweenPours = CurrentPourTime - PrevPourTime;

	if (CurrentBoundaryTime > 0.0f && CookingTimerValue > CurrentBoundaryTime)
	{
		LastLiquidType = ELiquidType::None;
	}

	if (LastLiquidType != Type)
	{
		//Calculate boundary time between steps
		++CurrentStepIndexInRecipe;
		CurrentBoundaryTime = 0.0f;
		if (AAlsCharacterExample* PlayerCharacter = Cast<AAlsCharacterExample>(GetWorld()->GetFirstPlayerController()->GetPawn()))
		{
			if (PlayerCharacter->GetCurrentRecipe(CurrentRecipe))
			{
				if (CurrentRecipe.LiquidSteps.IsValidIndex(CurrentStepIndexInRecipe))
				{
					CurrentBoundaryTime = (CurrentRecipe.LiquidSteps[CurrentStepIndexInRecipe].IdealStartTime + CurrentRecipe.LiquidSteps[CurrentStepIndexInRecipe - 1].IdealEndTime) * 0.5f;
				}
			}
			else
			{
				return;
			}
		}

		FPourEvent PourEvent;
		PourEvent.LiquidType = Type;
		PourEvent.AmountAdded = Amount;
		PourEvent.TimeStart = CookingTimerValue;
		PourEvent.TimeEnd = CookingTimerValue;
		PourEvent.TimingScore = 0.0f;
		if (CurrentRecipe.LiquidSteps.IsValidIndex(CurrentStepIndexInRecipe - 1))
		{
			PourEvent.TimingScore = PourEvent.AmountAdded * CalculateTimingQualityPerPourMoment(CookingTimerValue, CurrentRecipe.LiquidSteps[CurrentStepIndexInRecipe - 1]);
		}
		TotalAmountAddedPerStep = Amount;

		FLiquidStepOnPour LiquidStepOnPour;
		LiquidStepOnPour.PourEvents.Add(PourEvent);
		LiquidStepsOnPour.Add(LiquidStepOnPour);
	}
	else if (LastLiquidType == Type)
	{
		if (TimeBetweenPours > MinTimeToStartNewPourEvent)
		{
			FPourEvent PourEvent;
			PourEvent.LiquidType = Type;
			PourEvent.AmountAdded = Amount;
			PourEvent.TimeStart = CookingTimerValue;
			PourEvent.TimeEnd = CookingTimerValue;
			PourEvent.TimingScore = 0.0f;
			if (CurrentRecipe.LiquidSteps.IsValidIndex(CurrentStepIndexInRecipe - 1))
			{
				PourEvent.TimingScore = PourEvent.AmountAdded * CalculateTimingQualityPerPourMoment(CookingTimerValue, CurrentRecipe.LiquidSteps[CurrentStepIndexInRecipe - 1]);
			}

			LiquidStepsOnPour.Last().PourEvents.Add(PourEvent);
		}
		else
		{
			if (CurrentRecipe.LiquidSteps.IsValidIndex(CurrentStepIndexInRecipe - 1))
			{
				LiquidStepsOnPour.Last().PourEvents.Last().TimingScore += Amount * CalculateTimingQualityPerPourMoment(CookingTimerValue, CurrentRecipe.LiquidSteps[CurrentStepIndexInRecipe - 1]);
			}
			LiquidStepsOnPour.Last().PourEvents.Last().AmountAdded += Amount;
			LiquidStepsOnPour.Last().PourEvents.Last().TimeEnd = CookingTimerValue;
		}
		TotalAmountAddedPerStep += Amount;
	}

	//Visual data on pour

	if (CurrentRecipe.LiquidSteps.IsValidIndex(CurrentStepIndexInRecipe - 1) && LiquidStepsOnPour.Last().PourEvents[0].LiquidType == CurrentRecipe.LiquidSteps[CurrentStepIndexInRecipe - 1].LiquidType)
	{
		UpdatePourVisual_TargetDish(LiquidStepsOnPour.Last().PourEvents[0].LiquidType, TotalAmountAddedPerStep, CurrentRecipe.LiquidSteps[CurrentStepIndexInRecipe - 1]);
	}
	else
	{
		UpdatePourVisual_TargetDish(LiquidStepsOnPour.Last().PourEvents[0].LiquidType, TotalAmountAddedPerStep, FLiquidStep());
	}

	//Sound steam
	if (HeatingLevel != EHeatingLevel::None)
	{
		if (!AudioComponent->IsPlaying())
		{
			AudioComponent->Play();
		}

		GetWorldTimerManager().ClearTimer(SteamSoundStopTimerHandle);

		GetWorldTimerManager().SetTimer(SteamSoundStopTimerHandle, [this]()
			{
				AudioComponent->Stop();
			},
			1.0f, false);
	}

	LastLiquidType = Type;
	PrevPourTime = CurrentPourTime;
}

void AA_Dishes::UpdateCookingSession()
{
	if (!IsACookWare)
	{
		return;
	}

	bCurrentHasIngredientsState = Ingredients.Num() > 0;
	if (!bPrevHasIngredientsState && bCurrentHasIngredientsState && HeatingLevel != EHeatingLevel::None)
	{
		CookingTimerValue = 0.0f;
		SetCookingTimerValue(FMath::FloorToInt(CookingTimerValue));
		SetCookingTimerVisibility(true);
	}
	if (bPrevHasIngredientsState && bCurrentHasIngredientsState && HeatingLevel != EHeatingLevel::None)
	{
		CookingTimerValue += 0.5f;
		SetCookingTimerValue(FMath::FloorToInt(CookingTimerValue));
	}
	if (bPrevHasIngredientsState && !bCurrentHasIngredientsState)
	{
		SetCookingTimerVisibility(false);
	}
	bPrevHasIngredientsState = bCurrentHasIngredientsState;
}

float AA_Dishes::CalculateTimingQualityPerPourMoment(float CurrentTime, FLiquidStep CurrentRecipeStep)
{
	if (CurrentTime >= CurrentRecipeStep.IdealStartTime && CurrentTime <= CurrentRecipeStep.IdealEndTime)
	{
		return 1.0f;
	}
	if (CurrentTime < CurrentRecipeStep.IdealStartTime)
	{
		return FMath::Clamp(1 - (CurrentRecipeStep.IdealStartTime - CurrentTime) / CurrentRecipeStep.EarlyTolerance, 0.0f, 1.0f);
	}
	if (CurrentTime > CurrentRecipeStep.IdealEndTime)
	{
		return FMath::Clamp(1 - (CurrentTime - CurrentRecipeStep.IdealEndTime) / CurrentRecipeStep.LateTolerance, 0.0f, 1.0f);
	}
	return 0.0f;
}

void AA_Dishes::ResetCookingSession()
{
	PrevPourTime = 0.0f;
	CurrentBoundaryTime = 0.0f;
	LastLiquidType = ELiquidType::None;
	CurrentStepIndexInRecipe = 0;

	LiquidStepsOnPour.Empty();
}

void AA_Dishes::UpdatePourVisual_TargetDish(ELiquidType Type, float LiquidAmount, FLiquidStep RecipeLiquidStep)
{
	//Pour widget
	if (UW_PourEvent* PourEventWidget = Cast<UW_PourEvent>(DishWidget->GetUserWidgetObject()))
	{
		PourEventWidget->UpdateVisualDataOnPourEvent(Type, LiquidAmount, RecipeLiquidStep.UnderAmountTolerance, RecipeLiquidStep.IdealMinimumAmount, RecipeLiquidStep.IdealMaximumAmount, RecipeLiquidStep.OverAmountTolerance);
	}
}
