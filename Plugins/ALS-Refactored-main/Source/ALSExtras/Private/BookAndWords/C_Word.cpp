#include "BookAndWords/C_Word.h"
#include "BookAndWords/A_Book.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/AudioComponent.h"

AC_Word::AC_Word()
{
	PrimaryActorTick.bCanEverTick = true;

	AbsorbTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("AbsorbTimeline"));
	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudionComponent"));

	AudioComp->SetupAttachment(RootComponent);
	AudioComp->SetAutoActivate(false);
}

void AC_Word::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	TArray<UActorComponent*> BoxComponents;
	GetComponents(UBoxComponent::StaticClass(), BoxComponents);
	for (UActorComponent* Comp : BoxComponents) Comp->DestroyComponent();

	TArray<UActorComponent*> TextComponents;
	GetComponents(UTextRenderComponent::StaticClass(), TextComponents);
	for (UActorComponent* Comp : TextComponents) Comp->DestroyComponent();

	FString SWord = Word.ToString();

	for (int32 i = 0; i < SWord.Len(); ++i)
	{
		const FString Letter = SWord.Mid(i, 1);

		FString BoxName = FString::Printf(TEXT("LetterBox_%d"), i);
		UBoxComponent* Box = NewObject<UBoxComponent>(this, *BoxName);
		Box->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		Box->RegisterComponent();
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
		Box->SetCollisionProfileName("OverlapAllDynamic");
		Box->SetCollisionResponseToAllChannels(ECR_Overlap);

		Box->OnComponentBeginOverlap.AddDynamic(this, &AC_Word::OnLetterBeginOverlap);

		FString TextName = FString::Printf(TEXT("LetterText_%d"), i);
		UTextRenderComponent* TextComp = NewObject<UTextRenderComponent>(this, *TextName);
		TextComp->AttachToComponent(Box, FAttachmentTransformRules::KeepRelativeTransform);
		TextComp->RegisterComponent();

		TextComp->SetText(FText::FromString(Letter));
		TextComp->SetHorizontalAlignment(EHTA_Center);
		TextComp->SetVerticalAlignment(EVRTA_TextCenter);
		TextComp->SetRelativeLocation(FVector::ZeroVector);
		TextComp->SetWorldSize(50.0f);

		FVector TextSize = TextComp->GetTextLocalSize();
		TextSize.X = 4.0f;
		Box->SetBoxExtent(TextSize / 2.0f);
		Box->SetRelativeLocation(FVector(0.0f, -TextSize.Y * i, 0.0f));
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
