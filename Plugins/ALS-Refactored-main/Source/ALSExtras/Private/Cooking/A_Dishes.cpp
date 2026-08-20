#include "Cooking/A_Dishes.h"
#include "GameFramework/Character.h"
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

AA_Dishes::AA_Dishes()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionShape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CollisionShape"));
	LiquidShape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LiquidShape"));
	CookedResultSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CookedResultSpawnPoint"));
	LiquidLevelPoint = CreateDefaultSubobject<USceneComponent>(TEXT("LiquidLevelPoint"));
	FullLiquidLevelPoint = CreateDefaultSubobject<USceneComponent>(TEXT("FullLiquidLevelPoint"));
	CentreLiquidLevelPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CentreLiquidLevelPoint"));
	EmptyLiquidLevelPoint = CreateDefaultSubobject<USceneComponent>(TEXT("EmptyLiquidLevelPoint"));
	SpringArmToEdge = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmToEdgePoint"));
	EdgeLiquidPoint = CreateDefaultSubobject<USceneComponent>(TEXT("EdgeLiquidPoint"));
	DishWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("DishWidget"));
	TossTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("TossTimelineComponent"));
	FluidFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FluidFX"));
	PourFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PourFX"));

	CollisionShape->SetupAttachment(RootComponent);
	LiquidShape->SetupAttachment(RootComponent);
	CookedResultSpawnPoint->SetupAttachment(RootComponent);
	LiquidLevelPoint->SetupAttachment(RootComponent);
	FullLiquidLevelPoint->SetupAttachment(RootComponent);
	CentreLiquidLevelPoint->SetupAttachment(RootComponent);
	EmptyLiquidLevelPoint->SetupAttachment(RootComponent);
	SpringArmToEdge->SetupAttachment(RootComponent);
	EdgeLiquidPoint->SetupAttachment(SpringArmToEdge);
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

		if (!bIsOnAttaching && AttachedMesh->DoesSocketExist(AttachedSocketName))
		{
			if (FVector::Distance(GetActorLocation(), AttachedMesh->GetSocketLocation(AttachedSocketName)) <= 5.0f)
			{
				bIsOnAttaching = true;
				StaticMesh->AttachToComponent(AttachedMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachedSocketName);
			}
			SetActorLocation(FMath::VInterpTo(GetActorLocation(), AttachedMesh->GetSocketLocation(AttachedSocketName), DeltaTime, 2.0f));
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
	CurrentLocation = GetActorLocation();
	FVector DeltaLocation = CurrentLocation - PrevLocation;
	float DeltaLength = DeltaLocation.Length();
	float DirectionCheck = FVector::DotProduct(DeltaLocation.GetSafeNormal(), FVector(0.0f, 0.0f, 1.0f));

	if (DeltaLength > 10.0f && DirectionCheck > 0.9f)
	{
		DeltaLengthAccum += DeltaLength;
	}
	else if (DeltaLengthAccum >= 30.0f)
	{
		TossDish(DeltaLengthAccum);
		DeltaLengthAccum = 0.0f;
	}
	else
	{
		DeltaLengthAccum = 0.0f;
	}

	PrevLocation = CurrentLocation;

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

	if (TossLocationFloatCurve && TossRotationFloatCurve)
	{
		TossLocationProgressFunction.BindUFunction(this, FName("TossLocationTimelineProgress"));
		TossRotationProgressFunction.BindUFunction(this, FName("TossRotationTimelineProgress"));
		TossTimeline->AddInterpFloat(TossLocationFloatCurve, TossLocationProgressFunction);
		TossTimeline->AddInterpFloat(TossRotationFloatCurve, TossRotationProgressFunction);

		TossFinishedFunction.BindUFunction(this, FName("TossTimelineFinished"));
		TossTimeline->SetTimelineFinishedFunc(TossFinishedFunction);

		TossTimeline->SetLooping(false);
	}

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			if (!GetWorldTimerManager().IsTimerActive(TossTimerHandle))
			{
				CollisionShape->GetOverlappingActors(OverlappingActors, AA_Cookable::StaticClass());

				// Check if ingredient doesn't belong any dish
				for (AActor* Ingredient : OverlappingActors.Array())
				{
					if (AA_Cookable* CookableIngredient = Cast<AA_Cookable>(Ingredient))
					{
						if (CookableIngredient->AttachedDish == nullptr)
						{
							Ingredients.AddUnique(CookableIngredient);
							int32& CountRef = IngredientCountMap.FindOrAdd(CookableIngredient->Name);
							++CountRef;
							CookableIngredient->AttachedDish = this;
							CookableIngredient->bIsAttaching = true;

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
						int32* CountPtr = IngredientCountMap.Find(Ingredients[i]->Name);
						if (CountPtr)
						{
							--(*CountPtr);
							if (*CountPtr <= 0)
							{
								IngredientCountMap.Remove(Ingredients[i]->Name);
							}
						}
						Ingredients[i]->AttachedDish = nullptr;

						if (bShowCookingDebug)
						{
							GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Removed ingredient: %s, Count: %d"), *Ingredients[i]->Name.ToString(), *CountPtr));
						}

						Ingredients.RemoveAt(i);
					}
				}

				UpdateCookingSession();
			}
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
					break;
				}
			}
		}

		//GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Blue, FString::Printf(TEXT("PourIntensityNormalized: %f   LiquidLevelNormalized: %f   LiquidVolume: %f"), PourIntensityNormalized, LiquidLevelNormalized, LiquidVolume));

		if (!PourFX->IsActive())
		{
			PourFX->Activate();
		}
	}

	// Update Niagara pour effect
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
	StaticMesh->AttachToComponent(AttachedMesh, FAttachmentTransformRules::KeepWorldTransform, SocketName);
}

void AA_Dishes::DetachDishFromHand()
{
	bIsOnAttaching = false;
	AttachedMesh = nullptr;
	StaticMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
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

	CheckIfCooked();
}

void AA_Dishes::TossDish(float Delta)
{
	if (GetWorldTimerManager().IsTimerActive(TossTimerHandle))
	{
		return;
	}

	GetWorldTimerManager().SetTimer(TossTimerHandle, [this]()
		{
		}, 3.0f, false);

	TossOffset = Delta;
	TossTimeline->PlayFromStart();
}

void AA_Dishes::TossLocationTimelineProgress(float Value)
{
	//StaticMesh->SetRelativeLocation(FMath::Lerp(FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, TossOffset / 2.0f), Value));
}

void AA_Dishes::TossRotationTimelineProgress(float Value)
{
	float CurrentTime = TossTimeline->GetPlaybackPosition();
	float Duration = TossTimeline->GetTimelineLength();
	float Percent = CurrentTime / Duration;

	if (!bIsTossing && Percent > 0.5f)
	{
		bIsTossing = true;
		for (AA_Cookable* Ingredient : Ingredients)
		{
			Ingredient->Toss(TossOffset);
		}
	}

	float CurrentAngle = Value;
	float DeltaAngle = (CurrentAngle - PreviousAngle) * TossOffset;
	//StaticMesh->AddLocalRotation(FRotator(DeltaAngle, 0.0f, 0.0f));
	PreviousAngle = CurrentAngle;
}

void AA_Dishes::TossTimelineFinished()
{
	bIsTossing = false;
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

void AA_Dishes::CheckIfCooked()
{
	if (bIsOnRecipeChecking || Recipes == nullptr)
	{
		return;
	}

	bIsOnRecipeChecking = true;
	const FRecipe* MatchedRecipe = nullptr;

	for (const FRecipe& Recipe : Recipes->Recipes)
	{
		bool bIsFollowingRecipe = true;

		//Check for being tossed
		if (Recipe.bRequiresToss)
		{
			for (AA_Cookable* Ingredient : Ingredients)
			{
				if (!Ingredient->bWasTossed)
				{
					bIsFollowingRecipe = false;
					break;
				}
			}

			if (!bIsFollowingRecipe)
			{
				continue;
			}
		}

		TMap<FName, int32> RecipeMap;
		for (const FRecipeIngredient& Ingredient : Recipe.Ingredients)
		{
			RecipeMap.Add(Ingredient.IngredientName, Ingredient.IngredientQuantity);
		}

		//Check for recipe matching from recipe
		for (const auto& Pair : RecipeMap)
		{
			if (!IngredientCountMap.Contains(Pair.Key))
			{
				bIsFollowingRecipe = false;
				break;
			}
		}

		if (!bIsFollowingRecipe)
		{
			continue;
		}

		//Check for recipe matching - extra ingredients
		for (const auto& Pair : IngredientCountMap)
		{
			if (!RecipeMap.Contains(Pair.Key))
			{
				bIsFollowingRecipe = false;
				break;
			}
		}

		if (!bIsFollowingRecipe)
		{
			continue;
		}


		MatchedRecipe = &Recipe;
		break;
	}

	struct FIngredientQuality
	{
		float GroupQuality = 0.0f;
		float TotalQualityByWeight = 0.0f;
		float ChunksWeightTotal = 0.0f;
		float WorstChunkQuality = 1.0f;
		float WorstChunkInfluence = 0.0f;
		float MaxChunkWeight = 0.0f;
		float RecipeImportance = 1.0f;
		float FailureSeverity = 0.0f;
		float MissingProportion = 0.0f;
		float ShortageSeverity = 0.0f;
	};

	// Calculate chunks average quality
	TMap<FName, FIngredientQuality> IngredientQualityMap;
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
		if (Ingredient->ChunkMass >= SignificantChunkPercentage * QualityRef.MaxChunkWeight)
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
	float DishQuality = 0.0f;
	float TotalRecipeImportance = 0.0f;
	float LowestGroupQuality = 1.0f;
	for (const auto& Pair : IngredientQualityMap)
	{
		DishQuality += Pair.Value.GroupQuality * Pair.Value.RecipeImportance;
		TotalRecipeImportance += Pair.Value.RecipeImportance;
		if (Pair.Value.FailureSeverity >= FailureSeverityThreshold)
		{
			LowestGroupQuality = FMath::Min(LowestGroupQuality, Pair.Value.GroupQuality);
		}
	}
	DishQuality /= TotalRecipeImportance;
	DishQuality *= (0.5f + 0.5f * LowestGroupQuality); // Adjust dish quality based on the lowest group quality

	//Missed requared pieces calculation
	float WeightedShortageSeverity = 0.0f;
	float MissingPieceDeduction = 0.0f;
	if (MatchedRecipe)
	{
		float SumShortageSeverity_RecipeImportance = 0.0f;
		for (const auto& Ingredient : MatchedRecipe->Ingredients)
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
		MissingPieceDeduction = WeightedShortageSeverity * MissingPenaltyStrength;
	}
	float DishAverage = DishQuality;
	DishQuality = FMath::Clamp(DishQuality - MissingPieceDeduction, 0.0f, 1.0f);

	// Debug display
	if (bShowCookingDebug && MatchedRecipe)
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
	if (MatchedRecipe && MatchedRecipe->ResultCookableClass)
	{
		for (AA_Cookable* Ingredient : Ingredients)
		{
			Ingredient->Destroy();
		}

		Ingredients.Empty();
		IngredientCountMap.Empty();

		AA_Cookable* NewDish = GetWorld()->SpawnActor<AA_Cookable>(MatchedRecipe->ResultCookableClass, CookedResultSpawnPoint->GetComponentLocation(), CookedResultSpawnPoint->GetComponentRotation());
		NewDish->CookingTime = static_cast<int32>(NewDish->DefaultCookingTime * DishQuality * 2.0f);
		NewDish->Quality = DishQuality;

		EDishRating FinalRating = NewDish->GetDishRating(DishQuality, MatchedRecipe->RatingThresholds);

		if (bShowCookingDebug)
		{
			GEngine->AddOnScreenDebugMessage(-1, 40.0f, FColor::Green, FString::Printf(TEXT("Final Rating: %s"), *StaticEnum<EDishRating>()->GetDisplayNameTextByValue(static_cast<int64>(FinalRating)).ToString()));
		}

		NewDish->ShowFinalDishRating(FinalRating);
	}

	bIsOnRecipeChecking = false;
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
		//Calculate AmountQuality and TimingQuality
		//
		//

		FPourEvent PourEvent;
		PourEvent.LiquidType = Type;
		PourEvent.AmountAdded = Amount;
		PourEvent.TimeStart = CookingTimerValue;
		PourEvent.TimeEnd = CookingTimerValue;

		FLiquidStepOnPour LiquidStepOnPour;
		LiquidStepOnPour.PourEvents.Add(PourEvent);
		LiquidStepsOnPour.Add(LiquidStepOnPour);

		++CurrentStepIndexInRecipe;
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
			LiquidStepsOnPour.Last().PourEvents.Add(PourEvent);
		}
		else
		{
			LiquidStepsOnPour.Last().PourEvents.Last().AmountAdded += Amount;
			LiquidStepsOnPour.Last().PourEvents.Last().TimeEnd = CookingTimerValue;
		}
	}

	LastLiquidType = Type;
	PrevPourTime = CurrentPourTime;
}

void AA_Dishes::UpdateCookingSession()
{
	bCurrentHasIngredientsState = Ingredients.Num() > 0;
	if (!bPrevHasIngredientsState && bCurrentHasIngredientsState)
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
