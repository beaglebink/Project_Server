#include "CoreBlueprintFunctionLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputTriggers.h"
#include "Engine/LocalPlayer.h"

void UCoreBlueprintFunctionLibrary::SimulateKeyPress(APlayerController* PlayerController, UInputAction* InputAction)
{
	if (PlayerController && InputAction)
	{
		// Получаем локальный игрок
		ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
		if (LocalPlayer)
		{
			// Получаем подсистему Enhanced Input
			UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
			if (InputSubsystem)
			{
				// Создаем значение ввода
				FInputActionValue InputValue(true); // Для boolean действия

				// Инжектируем действие через подсистему
				InputSubsystem->InjectInputForAction(InputAction, InputValue, {}, {});
			}
		}
	}
}

bool UCoreBlueprintFunctionLibrary::IsPIE()
{
#if WITH_EDITOR
	return true;
#else
	return false;
#endif
}

USceneComponent* UCoreBlueprintFunctionLibrary::GetSceneComponentCopy(USceneComponent* Component, AActor* Outer, FText Name)
{
	if (!Component || !Outer)
	{
		return nullptr;
	}

	USceneComponent* NewComponent = DuplicateObject<USceneComponent>(Component, Outer, *Name.ToString());

	Outer->AddInstanceComponent(NewComponent);
	NewComponent->RegisterComponent();
	NewComponent->AttachToComponent(Outer->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

	return NewComponent;
}

void UCoreBlueprintFunctionLibrary::RemoveSceneComponent(USceneComponent* Component)
{
	if (!Component)
	{
		return;
	}

	AActor* Owner = Component->GetOwner();
	if (!Owner)
	{
		return;
	}

	Owner->RemoveInstanceComponent(Component);
	Component->DestroyComponent();
}

FFluidPoints UCoreBlueprintFunctionLibrary::GenerateFluidPointsGrid(UStaticMeshComponent* MeshComponent, float PointSpacing, bool bDrawDebug)
{
	if (!MeshComponent)
	{
		return FFluidPoints();
	}

	UWorld* World = MeshComponent->GetWorld();

	FVector Origin = MeshComponent->Bounds.Origin;
	FVector Extent = MeshComponent->Bounds.BoxExtent;
	int NumX = FMath::CeilToInt(Extent.X * 2 / PointSpacing);
	int NumY = FMath::CeilToInt(Extent.Y * 2 / PointSpacing);
	int NumZ = FMath::CeilToInt(Extent.Z * 2 / PointSpacing);

	FFluidPoints FluidPoints;
	FluidPoints.LocalPositions.Reserve(NumX * NumY * NumZ);
	FluidPoints.DistancesToWall.Reserve(NumX * NumY * NumZ);

	for (int x = 0; x < NumX; ++x)
	{
		for (int y = 0; y < NumY; ++y)
		{
			for (int z = 0; z < NumZ; ++z)
			{
				FVector Point = Origin + FVector(x * PointSpacing - Extent.X, y * PointSpacing - Extent.Y, z * PointSpacing - Extent.Z);

				FHitResult HitResultRightX;
				FHitResult HitResultLeftX;
				World->LineTraceSingleByChannel(HitResultRightX, Point, Point + FVector(Extent.X * 2.0f, 0.0f, 0.0f),ECC_GameTraceChannel10);
				World->LineTraceSingleByChannel(HitResultLeftX, Point, Point - FVector(Extent.X * 2.0f, 0.0f, 0.0f), ECC_GameTraceChannel10);
				if (HitResultRightX.Component == MeshComponent && HitResultLeftX.Component == MeshComponent)
				{
					if (bDrawDebug)
					{
						DrawDebugPoint(World, Point, 3.0f, FColor::Green, true);
					}

					FluidPoints.LocalPositions.Add(MeshComponent->GetComponentTransform().InverseTransformPosition(Point));
					FluidPoints.DistancesToWall.Add(0.0f);
				}
			}
		}
	}

	FluidPoints.LocalPositions.Shrink();
	FluidPoints.DistancesToWall.Shrink();

	return FluidPoints;
}