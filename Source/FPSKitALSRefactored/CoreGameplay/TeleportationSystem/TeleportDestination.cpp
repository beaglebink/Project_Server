#include "TeleportDestination.h"
#include "Components/SceneComponent.h"
#include "SlotSceneComponent.h"
#include "UObject/UnrealType.h"          // FPropertyChangedEvent, GET_MEMBER_NAME_CHECKED
#include "Components/TextRenderComponent.h"

#if WITH_EDITOR
#include "ScopedTransaction.h"           // FScopedTransaction (requires UnrealEd in Build.cs for editor builds)
#include "UObject/UObjectGlobals.h"      // NewObject
#endif

ATeleportDestination::ATeleportDestination()
{
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
#if WITH_EDITORONLY_DATA
    LabelComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Label"));
    LabelComponent->SetupAttachment(RootComponent);
    LabelComponent->bIsScreenSizeScaled = true;
    LabelComponent->SetHiddenInGame(true); // Только в редакторе
    LabelComponent->SetRelativeLocation(FVector(0, 0, 3));

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextLabel"));
    Label->SetIsVisualizationComponent(true);
    Label->SetHiddenInGame(true);
    Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextBottom);
    Label->SetTextRenderColor(FColor::Red);
    Label->SetWorldSize(14.f);
    Label->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
    Label->SetupAttachment(RootComponent);
    //Label->RegisterComponent();
    //AddInstanceComponent(Label);

#endif


#if WITH_EDITOR
    SetFlags(RF_Transactional);

    static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Engine/EditorResources/AI/S_NavLink"));
    if (IconTexture.Succeeded())
    {
        LabelComponent->SetSprite(IconTexture.Object);
        LabelComponent->SetRelativeScale3D(FVector(0.25f));
        LabelComponent->bIsScreenSizeScaled = true;
        LabelComponent->ScreenSize = 0.002f;
    }
#endif
}

void ATeleportDestination::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);

#if WITH_EDITOR
    if (Label)
    {
        const FText Text = DestinationID.IsEmpty() ? FText::FromString(TEXT("T_Point")) : FText::FromString(DestinationID);
        Label->SetText(Text);

        for (auto& Slot : Slots)
        {
            if (Slot)
            {
                Slot->SetOwnerName(DestinationID);
                Slot->UpdateVisualsFromName();
            }
        }
    }
#endif
}


// Важно: даем тело и для не-редакторских сборок, чтобы не было линковочных ошибок
USlotSceneComponent* ATeleportDestination::AddSlot()
{
#if WITH_EDITOR
    const FScopedTransaction Tx(NSLOCTEXT("TeleportDestination", "AddSlotTx", "Add Teleport Slot"));
    Modify();

    const FName UniqueName = GenerateUniqueSlotName(TEXT("Slot"));

    USlotSceneComponent* NewSlot = NewObject<USlotSceneComponent>(this, /*DesiredSlotType*/USlotSceneComponent::StaticClass(), UniqueName, RF_Transactional);


    NewSlot->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
    NewSlot->SetFlags(RF_Transactional);
    NewSlot->SlotName = UniqueName;
    NewSlot->SetOwnerName(UniqueName.ToString());

    NewSlot->OnComponentCreated();
    NewSlot->SetupAttachment(RootComponent);
    NewSlot->RegisterComponent();
    NewSlot->Modify();
    AddInstanceComponent(NewSlot);
    UE_LOG(LogTemp, Warning, TEXT("Slots.Add"));
    Slots.Add(NewSlot);

    return NewSlot;
#else
    return nullptr;
#endif
}

#if WITH_EDITOR

FName ATeleportDestination::GenerateUniqueSlotName(const FName& Base) const
{
    // Собираем занятые пользовательские имена
    TSet<FName> UsedNames;
    for (const USlotSceneComponent* S : Slots)
    {
        if (S)
        {
            UsedNames.Add(S->SlotName);
        }
    }

    // Подбираем “Base1”, “Base2”, ...
    int32 Index = 1;
    FName Candidate;
    do
    {
        Candidate = *FString::Printf(TEXT("%s%d"), *Base.ToString(), Index++);
    } while (UsedNames.Contains(Candidate));

    // Обеспечиваем уникальность имени UObject среди компонентов
    return Candidate;//MakeUniqueObjectName(this, USlotSceneComponent::StaticClass(), Candidate);
}

void ATeleportDestination::EnsureSlotAttached(USlotSceneComponent* Slot)
{
    if (!Slot || Slot->GetOwner() != this)
    {
        return;
    }

    // Безопасно: AddInstanceComponent сам проверяет наличие
    AddInstanceComponent(Slot);

    if (!Slot->GetAttachParent())
    {
        Slot->SetupAttachment(RootComponent);
    }

    if (!Slot->IsRegistered())
    {
        Slot->RegisterComponent();
    }
}

void ATeleportDestination::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    //if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ATeleportDestination, Slots))
    //{
        if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == FName("Slots"))
        {
            UE_LOG(LogTemp, Warning, TEXT("Slots array changed"));

            // Можно проверить последний элемент:
            if (Slots.Num() > 0 && Slots.Last() == nullptr)
            {
                // Автоматически создать слот
                USlotSceneComponent* NewSlot = NewObject<USlotSceneComponent>(this, USlotSceneComponent::StaticClass(), NAME_None, RF_Transactional);
                Slots[Slots.Num() - 1] = NewSlot;
                NewSlot->SetIsVisualizationComponent(false);
                NewSlot->SetOwnerName(DestinationID);
                NewSlot->UpdateVisualsFromName();
                Modify();
            }
        }

        if (GEditor)
        {
            GEditor->NoteSelectionChange(); // обновляет компонентную схему
        }
        // Страхуемся на случай ручных правок массива в Details
        for (USlotSceneComponent* Slot : Slots)
        {
            EnsureSlotAttached(Slot);
        }
    //}

    if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ATeleportDestination, DestinationID))
    {
        if (Label)
        {
            const FText Text = DestinationID.IsEmpty() ? FText::FromString(TEXT("T_Point")) : FText::FromString(DestinationID);
            Label->SetText(Text);

			for (auto& Slot : Slots)
			{
				if (Slot)
				{
					Slot->SetOwnerName(DestinationID);
					Slot->UpdateVisualsFromName();
				}
			}
        }
    }

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

void ATeleportDestination::PostEditUndo()
{
    Super::PostEditUndo();

    // Восстанавливаем корректную регистрацию и прикрепление после Undo/Redo
    for (USlotSceneComponent* Slot : Slots)
    {
        EnsureSlotAttached(Slot);
    }
}

#endif // WITH_EDITOR