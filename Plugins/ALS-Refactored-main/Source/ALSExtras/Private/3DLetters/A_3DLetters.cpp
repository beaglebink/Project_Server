#include "3DLetters/A_3DLetters.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/TextureRenderTarget2D.h"    

AA_3DLetters::AA_3DLetters()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	LettersToMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LettersToMeshComponent"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	SwapLettersTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SwapLettersTimelineComponent"));
	TransformLettersToMeshTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("TransformLettersToMeshTimelineComponent"));

	AudioComponent->SetupAttachment(RootComponent);
	LettersToMeshComponent->SetupAttachment(RootComponent);

	AudioComponent->bAutoActivate = false;
}

void AA_3DLetters::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (LettersText.IsEmpty() || LetterMeshes.Num() == 0 || !IsValid(LettersToMeshComponent->GetStaticMesh()))
	{
		return;
	}

	if (!IsValid(MeshMaterialInstanceDynamic))
	{
		MeshMaterialInstanceDynamic = LettersToMeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	}

	TargetMeshRenderTarget = NewObject<UTextureRenderTarget2D>(this);
	if (!TargetMeshRenderTarget)return;

	TargetMeshRenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA16f;
	TargetMeshRenderTarget->InitAutoFormat(1024, 1024);
	TargetMeshRenderTarget->ClearColor = FLinearColor::Black;
	TargetMeshRenderTarget->UpdateResourceImmediate(true);

	for (FLetter& Letter : LettersArray)
	{
		if (Letter.LetterMeshComponent)
		{
			Letter.LetterMeshComponent->DestroyComponent();
		}
		if (Letter.LetterDestroyFX)
		{
			Letter.LetterDestroyFX->DestroyComponent();
		}
	}
	LettersArray.Empty();

	LettersText = LettersText.ToLower();
	TArray<TCHAR> Chars = LettersText.GetCharArray();
	if (Chars.Num() > 0)
	{
		Chars.Pop();
	}

	for (int32 i = 0; i < Chars.Num(); ++i)
	{
		int32 j = FMath::RandRange(0, Chars.Num() - 1);
		Chars.Swap(i, j);
	}

	CurrentLettersText = Chars;

	const float LetterWidth = LetterMeshes[0]->GetBounds().BoxExtent.Y * 2.0f;

	for (int32 i = 0; i < Chars.Num(); ++i)
	{
		const TCHAR Symbol = Chars[i];
		int32 Index = -1;

		if (FChar::IsDigit(Symbol))       Index = Symbol - '0';
		else if (FChar::IsAlpha(Symbol))  Index = Symbol - 'a' + 10;

		if (!LetterMeshes.IsValidIndex(Index))
		{
			return;
		}

		const FName CompName = *FString::Printf(TEXT("Letter_%d_%c"), i, Symbol);
		UStaticMeshComponent* LetterComp = NewObject<UStaticMeshComponent>(this, CompName);
		if (!LetterComp) return;

		LetterComp->SetStaticMesh(LetterMeshes[Index]);
		LetterComp->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
		LetterComp->RegisterComponent();

		const FVector LetterOffset = -GetActorRightVector() * i * (LetterWidth + Spacing);
		LetterComp->SetWorldLocation(GetActorLocation() + LetterOffset);

		FLetter NewLetter;
		NewLetter.LetterMeshComponent = LetterComp;
		NewLetter.InitialLocation = LetterComp->GetComponentLocation();
		NewLetter.TargetLocation = NewLetter.InitialLocation;
		NewLetter.FloatAmplitude = FMath::RandRange(5.0f, 15.0f);
		NewLetter.FloatSpeed = FMath::RandRange(1.0f, 3.0f);
		NewLetter.FloatPhase = i * 0.3f;
		NewLetter.LetterChar = FName(*FString::Chr(Symbol));

		LettersArray.Add(NewLetter);
	}

	LettersToMeshComponent->SetWorldLocation(GetActorLocation() - GetActorRightVector() * ((LetterWidth + Spacing) * (LettersArray.Num() - 1) / 2.0f));
}


void AA_3DLetters::BeginPlay()
{
	Super::BeginPlay();

	//SwapLetters timeline 
	if (SwapLettersFloatCurve)
	{
		SwapLettersProgressFunction.BindUFunction(this, FName("SwapLettersTimelineProgress"));
		SwapLettersTimeline->AddInterpFloat(SwapLettersFloatCurve, SwapLettersProgressFunction);

		SwapLettersFinishedFunction.BindUFunction(this, FName("SwapLettersTimelineFinished"));
		SwapLettersTimeline->SetTimelineFinishedFunc(SwapLettersFinishedFunction);

		SwapLettersTimeline->SetLooping(false);
	}

	//TransformLettersToMesh timeline 
	if (TransformLettersToMeshFloatCurve)
	{
		TransformLettersToMeshProgressFunction.BindUFunction(this, FName("TransformLettersToMeshTimelineProgress"));
		TransformLettersToMeshTimeline->AddInterpFloat(TransformLettersToMeshFloatCurve, TransformLettersToMeshProgressFunction);

		TransformLettersToMeshFinishedFunction.BindUFunction(this, FName("TransformLettersToMeshTimelineFinished"));
		TransformLettersToMeshTimeline->SetTimelineFinishedFunc(TransformLettersToMeshFinishedFunction);

		TransformLettersToMeshTimeline->SetLooping(false);
	}

	UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), TargetMeshRenderTarget, MeshMaterialInstanceDynamic);
	for (FLetter& Letter : LettersArray)
	{
		UMaterialInstanceDynamic* FXDynMat = nullptr;
		UNiagaraComponent* FX = nullptr;
		if (LetterDestroyFXSystem)
		{
			FX = UNiagaraFunctionLibrary::SpawnSystemAttached(LetterDestroyFXSystem, Letter.LetterMeshComponent, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false, false);
			FX->SetVariableObject(TEXT("User.TargetStaticMeshObject"), LettersToMeshComponent->GetStaticMesh());
			UMaterialInterface* BaseMat = Letter.LetterMeshComponent->GetMaterial(0);
			FXDynMat = UMaterialInstanceDynamic::Create(BaseMat, this);
			FXDynMat->SetTextureParameterValue(TEXT("TargetMeshRenderTarget"), TargetMeshRenderTarget);
			FXDynMat->SetScalarParameterValue(TEXT("IsNiagaraMaterial"), 1.0f);
			FX->SetVariableMaterial(TEXT("User.ParticlesMaterial"), FXDynMat);
			Letter.LetterDestroyFX = FX;

			UMaterialInstanceDynamic* DynMat = Letter.LetterMeshComponent->CreateAndSetMaterialInstanceDynamic(0);
			Letter.LetterMaterialInstanceDynamic = DynMat;
		}
	}

	MeshMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Opacity"), 0.0f);
}

void AA_3DLetters::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Floating effect
	const FVector RightDir = -GetActorRightVector();

	if (!bIsSwappingLetters && !bIsTransformingLettersToMesh)
	{
		for (FLetter& Letter : LettersArray)
		{
			float OffsetY = FMath::Sin(GetWorld()->TimeSeconds * Letter.FloatSpeed + Letter.FloatPhase) * Letter.FloatAmplitude;
			float OffsetZ = FMath::Cos(GetWorld()->TimeSeconds * Letter.FloatSpeed + Letter.FloatPhase * 0.5f) * (Letter.FloatAmplitude * 0.5f);

			FVector NewLocation = Letter.InitialLocation + RightDir * OffsetY + FVector(0, 0, OffsetZ);
			Letter.LetterMeshComponent->SetWorldLocation(NewLocation);
		}
	}
}

void AA_3DLetters::HandleTextFromWeapon_Implementation(const FText& TextCommand)
{
	CommandLettersText = TextCommand.ToString().ToLower();

	if (bIsSwappingLetters || bIsTransformingLettersToMesh || CurrentLettersText == CommandLettersText || !AreWordsEqualIgnoreOrder(LettersText, CommandLettersText))
	{
		return;
	}
	bIsSwappingLetters = true;

	for (FLetter& Letter : LettersArray)
	{
		Letter.InitialLocation = Letter.LetterMeshComponent->GetComponentLocation();
	}

	FVector Extent = LetterMeshes[0]->GetBounds().BoxExtent;
	float Width = Extent.Y * 2.0f;

	for (int32 i = 0; i < CommandLettersText.Len(); ++i)
	{
		TCHAR TargetChar = CommandLettersText[i];

		for (int32 j = i; j < LettersArray.Num(); ++j)
		{
			if (LettersArray[j].LetterChar == FName(*FString::Chr(TargetChar)))
			{
				if (i != j)
				{
					Swap(LettersArray[i], LettersArray[j]);
				}

				LettersArray[i].TargetLocation = GetActorLocation() - GetActorRightVector() * i * (Width + Spacing);
				break;
			}
		}
	}

	AudioComponent->SetSound(SwapSound);
	AudioComponent->Play();
	SwapLettersTimeline->PlayFromStart();
}

bool AA_3DLetters::AreWordsEqualIgnoreOrder(const FString& A, const FString& B)
{
	if (A.Len() != B.Len())
	{
		return false;
	}

	TArray<TCHAR> ArrA = A.GetCharArray();
	TArray<TCHAR> ArrB = B.GetCharArray();

	if (ArrA.Num() > 0) ArrA.Pop();
	if (ArrB.Num() > 0) ArrB.Pop();

	ArrA.Sort();
	ArrB.Sort();

	for (int32 i = 0; i < ArrA.Num(); ++i)
	{
		if (ArrA[i] != ArrB[i])
		{
			return false;
		}
	}

	return true;
}

void AA_3DLetters::StartTransformLettersToMesh()
{
	bIsTransformingLettersToMesh = true;
	AudioComponent->SetSound(TransformSound);
	AudioComponent->Play();

	for (FLetter& Letter : LettersArray)
	{
		Letter.LetterDestroyFX->SetVariableVec3(TEXT("User.TargetOffset"), LettersToMeshComponent->GetComponentLocation() - Letter.LetterMeshComponent->GetComponentLocation());
		Letter.LetterDestroyFX->ActivateSystem(true);
	}

	TransformLettersToMeshTimeline->PlayFromStart();
}

void AA_3DLetters::SwapLettersTimelineProgress(float Value)
{
	for (FLetter& Letter : LettersArray)
	{
		Letter.LetterMeshComponent->SetWorldLocation(FMath::Lerp(Letter.InitialLocation, Letter.TargetLocation, Value));
	}
}

void AA_3DLetters::SwapLettersTimelineFinished()
{
	bIsSwappingLetters = false;

	for (FLetter& Letter : LettersArray)
	{
		Letter.InitialLocation = Letter.LetterMeshComponent->GetComponentLocation();
		Letter.TargetLocation = Letter.InitialLocation;
	}

	AudioComponent->Stop();

	CurrentLettersText = CommandLettersText;
	if (CurrentLettersText == LettersText)
	{
		StartTransformLettersToMesh();
	}
}

void AA_3DLetters::TransformLettersToMeshTimelineProgress(float Value)
{
	for (FLetter& Letter : LettersArray)
	{
		Letter.LetterMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Opacity"), 1 - FMath::Clamp(Value, 0.0f, 1.0f));
	}
	MeshMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Opacity"), FMath::Clamp(Value - 2, 0.0f, 1.0f));
}

void AA_3DLetters::TransformLettersToMeshTimelineFinished()
{
	bIsTransformingLettersToMesh = false;
	AudioComponent->Stop();
}