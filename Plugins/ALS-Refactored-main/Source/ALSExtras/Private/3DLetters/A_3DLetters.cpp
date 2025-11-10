#include "3DLetters/A_3DLetters.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

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

	if (LettersText.IsEmpty() || LetterMeshes.Num() == 0)
	{
		return;
	}

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
	const FVector RightDir = -GetActorRightVector();

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

		UMaterialInstanceDynamic* DynMat = nullptr;
		DynMat = LetterComp->CreateAndSetMaterialInstanceDynamic(0);

		UNiagaraComponent* FX = nullptr;
		if (LetterDestroyFXSystem)
		{
			const FName FXName = *FString::Printf(TEXT("FX_%d_%c"), i, Symbol);
			FX = UNiagaraFunctionLibrary::SpawnSystemAttached(LetterDestroyFXSystem, LetterComp, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
			FX->SetAutoActivate(false);
		}

		const FVector LetterOffset = -GetActorRightVector() * i * (LetterWidth + Spacing);
		LetterComp->SetWorldLocation(GetActorLocation() + LetterOffset);

		FLetter NewLetter;
		NewLetter.LetterMeshComponent = LetterComp;
		NewLetter.LetterMaterialInstanceDynamic = DynMat;
		NewLetter.LetterDestroyFX = FX;
		NewLetter.InitialLocation = LetterComp->GetComponentLocation();
		NewLetter.TargetLocation = NewLetter.InitialLocation;
		NewLetter.LetterChar = FName(*FString::Chr(Symbol));

		LettersArray.Add(NewLetter);
	}

	if (LettersToMeshComponent && LettersToMeshComponent->GetStaticMesh())
	{
		LettersToMeshComponent->SetRelativeLocation(-GetActorRightVector() * ((LetterWidth + Spacing) * (LettersArray.Num() - 1) / 2.f));
		if (!IsValid(MeshMaterialInstanceDynamic))
		{
			MeshMaterialInstanceDynamic = LettersToMeshComponent->CreateAndSetMaterialInstanceDynamic(0);
		}
		MeshMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Opacity"), 0.0f);
	}
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
}

void AA_3DLetters::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_3DLetters::HandleTextFromWeapon_Implementation(const FText& TextCommand)
{
	CommandLettersText = TextCommand.ToString().ToLower();

	if (bIsSwappingLetters || bIsTransformingLettersToMesh || CurrentLettersText == CommandLettersText || !AreWordsEqualIgnoreOrder(LettersText, CommandLettersText))
	{
		return;
	}
	bIsSwappingLetters = true;

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
		Letter.LetterMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Opacity"), 1 - Value);
	}
	MeshMaterialInstanceDynamic->SetScalarParameterValue(TEXT("Opacity"), Value);
}

void AA_3DLetters::TransformLettersToMeshTimelineFinished()
{
	bIsTransformingLettersToMesh = false;
	AudioComponent->Stop();
}