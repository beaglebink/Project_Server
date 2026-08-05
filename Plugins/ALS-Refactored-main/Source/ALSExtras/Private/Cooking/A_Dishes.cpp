#include "Cooking/A_Dishes.h"
#include "GameFramework/Character.h"
#include "Components/SphereComponent.h"
#include "Cooking/A_Cookable.h"

AA_Dishes::AA_Dishes()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionShape = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CollisionShape"));
	CookedResultSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CookedResultSpawnPoint"));
	TossTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("TossTimelineComponent"));

	CollisionShape->SetupAttachment(RootComponent);
	CookedResultSpawnPoint->SetupAttachment(RootComponent);

	CollisionShape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionShape->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	CollisionShape->bHiddenInGame = true;
}

void AA_Dishes::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

}

void AA_Dishes::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Dishes attach to player
	if (AttachedMesh)
	{
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), IsValid(AttachedMesh) ? FRotator(0.0f, AttachedMesh->GetComponentRotation().Yaw, 0.0f) : FRotator::ZeroRotator, DeltaTime, 0.5f));

		if (!bIsOnAttaching && AttachedMesh->DoesSocketExist(AttachedSocketName))
		{
			if (FVector::Distance(GetActorLocation(), AttachedMesh->GetSocketLocation(AttachedSocketName)) <= 5.0f)
			{
				bIsOnAttaching = true;
				StaticMesh->AttachToComponent(AttachedMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachedSocketName);
			}
			SetActorLocation(FMath::VInterpTo(GetActorLocation(), AttachedMesh->GetSocketLocation(AttachedSocketName), GetWorld()->GetDeltaSeconds(), 2.0f));
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
			}
		}, 0.5f, true);
}

void AA_Dishes::Destroyed()
{
	Super::Destroyed();

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
	AddActorLocalRotation(FRotator(AngleDelta * 10.0f, 0.0f, 0.0f));

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
