#include "BookAndWords/C_Word.h"
#include "BookAndWords/A_Book.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
AC_Word::AC_Word()
{
	PrimaryActorTick.bCanEverTick = true;

	AbsorbTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("AbsorbTimeline"));
	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));

	AudioComp->SetupAttachment(RootComponent);
	AudioComp->SetAutoActivate(false);
}

void AC_Word::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (GetMesh())
	{
		GetMesh()->SetVisibility(false, true);
	}

	InitialLocation = GetActorLocation();
	FloatAmplitude = FVector(FMath::RandRange(10.f, 25.f), FMath::RandRange(10.f, 25.f), FMath::RandRange(5.f, 15.f));
	FloatFrequency = FVector(FMath::RandRange(0.3f, 0.7f), FMath::RandRange(0.3f, 0.7f), FMath::RandRange(0.5f, 1.0f));
	FloatPhase = FVector(FMath::RandRange(0.f, 2 * PI), FMath::RandRange(0.f, 2 * PI), FMath::RandRange(0.f, 2 * PI));

	TArray<UActorComponent*> TextComponents;
	GetComponents(UTextRenderComponent::StaticClass(), TextComponents);
	for (UActorComponent* Comp : TextComponents)
	{
		Comp->DestroyComponent();
	}

	FString SWord = Word.ToString();
	const int32 LetterCount = SWord.Len();
	if (LetterCount == 0)
	{
		return;
	}

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (Capsule)
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Capsule->SetCollisionObjectType(ECC_WorldDynamic);
		Capsule->SetCollisionResponseToAllChannels(ECR_Overlap);
		Capsule->OnComponentBeginOverlap.AddDynamic(this, &AC_Word::OnLetterBeginOverlap);

		Capsule->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f));
	}

	const float FontSize = 100.0f;
	const float LetterSpacing = FontSize * 0.6f;
	const float TotalWidth = LetterSpacing * LetterCount;

	if (Capsule)
	{
		Capsule->SetCapsuleHalfHeight(TotalWidth * 0.5f);
		Capsule->SetCapsuleRadius(FontSize * 0.4f);
	}

	const float StartOffset = -TotalWidth * 0.5f + LetterSpacing * 0.5f;

	for (int32 i = 0; i < LetterCount; ++i)
	{
		const FString Letter = SWord.Mid(i, 1);

		FString TextName = FString::Printf(TEXT("LetterText_%d"), i);
		UTextRenderComponent* TextComp = NewObject<UTextRenderComponent>(this, *TextName);
		TextComp->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		TextComp->RegisterComponent();

		TextComp->SetText(FText::FromString(Letter));
		TextComp->SetHorizontalAlignment(EHTA_Center);
		TextComp->SetVerticalAlignment(EVRTA_TextCenter);
		TextComp->SetTextRenderColor(FColor::Magenta);
		TextComp->SetWorldSize(FontSize);

		TextComp->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));

		const float ZOffset = StartOffset + i * LetterSpacing;
		TextComp->SetRelativeLocation(FVector(0.0f, 0.0f, ZOffset));
	}
}


void AC_Word::BeginPlay()
{
	Super::BeginPlay();

	if (AbsorbFloatCurve)
	{
		AbsorbProgressFunction.BindUFunction(this, FName("AbsorbTimelineProgress"));
		AbsorbTimeline->AddInterpFloat(AbsorbFloatCurve, AbsorbProgressFunction);

		AbsorbFinishedFunction.BindUFunction(this, FName("AbsorbTimelineFinished"));
		AbsorbTimeline->SetTimelineFinishedFunc(AbsorbFinishedFunction);

		AbsorbTimeline->SetLooping(false);
	}

}

void AC_Word::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsAbsorbing)
	{
		if (bIsDragged)
		{
			InitialLocation = GetActorLocation();
		}
		else
		{
			float Time = GetWorld()->GetTimeSeconds();

			FVector Offset;
			Offset.X = FMath::Sin(Time * FloatFrequency.X + FloatPhase.X) * FloatAmplitude.X;
			Offset.Y = FMath::Cos(Time * FloatFrequency.Y + FloatPhase.Y) * FloatAmplitude.Y;
			Offset.Z = FMath::Sin(Time * FloatFrequency.Z + FloatPhase.Z) * FloatAmplitude.Z;

			SetActorLocation(InitialLocation + Offset);
		}
	}
}

void AC_Word::OnLetterBeginOverlap(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || bIsAbsorbing)
	{
		return;
	}

	if (AA_Book* Book = Cast<AA_Book>(OtherActor))
	{
		if (Book->BookGroupCode == BookGroupCode)
		{
			Book->AddWord(Word);
			DefaultWordLocation = GetActorLocation();
			TargetBookLocation = Book->GetActorLocation() + Book->GetActorUpVector() * 20.0f;
			Absorbing();
		}
	}
}

void AC_Word::Absorbing()
{
	AudioComp->Play();
	bIsAbsorbing = true;
	AbsorbTimeline->PlayFromStart();
}

void AC_Word::AbsorbTimelineProgress(float Value)
{
	SetActorScale3D(FMath::Lerp(FVector(1.0f, 1.0f, 1.0f), FVector(0.1f, 0.1f, 1.0f), Value));
	SetActorLocation(FMath::Lerp(DefaultWordLocation, TargetBookLocation, Value));
}

void AC_Word::AbsorbTimelineFinished()
{
	AudioComp->Play();
	Destroy();
}

void AC_Word::HandleWeaponDrag_Implementation(bool bIsDragging)
{
	bIsDragged = bIsDragging;
}
