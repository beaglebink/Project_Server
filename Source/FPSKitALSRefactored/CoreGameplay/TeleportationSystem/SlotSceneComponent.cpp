#include "SlotSceneComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TeleportDestination.h"

#if WITH_EDITOR
#include "ScopedTransaction.h"
#include "UObject/UObjectGlobals.h"
#if WITH_EDITORONLY_DATA
#include "ObjectTools.h"
#endif
#endif

USlotSceneComponent::USlotSceneComponent()
{
#if WITH_EDITOR
    PrimaryComponentTick.bCanEverTick = false;
    SetMobility(EComponentMobility::Movable);
    bHiddenInGame = true; 

    Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
    Arrow->SetupAttachment(this); 
    Arrow->SetHiddenInGame(true);

    Arrow->SetIsVisualizationComponent(true);
    Arrow->SetUsingAbsoluteRotation(false);

    Arrow->SetArrowColor(FColor::Cyan);
    Arrow->SetRelativeLocation(FVector(0.f, 0.f, 5.f));

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(this);
    Label->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
    Label->SetWorldSize(14.f);
    Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextBottom);
    Label->SetHiddenInGame(true);
    Label->SetTextRenderColor(FColor::Red);
#endif
}


void USlotSceneComponent::SetOwnerName(const FString Name)
{
    OwnerName = Name;
}

void USlotSceneComponent::OnRegister()
{
    Super::OnRegister();

    if (!GetAttachParent())
    {
        if (AActor* Owner = GetOwner())
        {
            if (USceneComponent* Root = Owner->GetRootComponent())
            {
				AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);

                Modify();
#if WITH_EDITOR
                Owner->PostEditChange();
                Owner->RerunConstructionScripts();
#endif // WITH_EDITOR
            }
        }
    }

#if WITH_EDITOR
    EnsureHelpers();
    UpdateVisualsFromName();

    if (GEditor)
    {
        GEditor->NoteSelectionChange();
    }
#endif
}

void USlotSceneComponent::SetActiveSlot(bool bActive)
{
    IsActiveSlot = bActive;
    OnChangeActive.Broadcast(this, IsActiveSlot);
}

bool USlotSceneComponent::GetActiveSlot() const
{
    return IsActiveSlot;
}

void USlotSceneComponent::StartCooldown()
{
    if (isCooldown) return;

    isCooldown = true;
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, [&]()
        {
            isCooldown = false;
            ATeleportDestination* Destination = Cast<ATeleportDestination>(GetOwner());
            OnStopSlotCooldown.Broadcast(Destination, this);
        }, CoolDownTime, false);
}

void USlotSceneComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    Super::OnComponentDestroyed(bDestroyingHierarchy);

    if (ATeleportDestination* Owner = Cast<ATeleportDestination>(GetOwner()))
    {
        Owner->RemoveSlot(this);
    }
#if WITH_EDITOR
    if (Arrow)
    {
        Arrow->DestroyComponent();
        Arrow = nullptr;
    }

    if (Label)
    {
        Label->DestroyComponent();
        Label = nullptr;
    }
#endif // WITH_EDITOR
}


#if WITH_EDITOR
void USlotSceneComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropName == GET_MEMBER_NAME_CHECKED(USlotSceneComponent, SlotName))
    {
        UpdateVisualsFromName();

        TryRenameObjectToMatchSlotName();
    }
}

void USlotSceneComponent::PostDuplicate(bool bDuplicateForPIE)
{
    Super::PostDuplicate(bDuplicateForPIE);

    AActor* Owner = GetOwner();
    ATeleportDestination* Destination = Cast<ATeleportDestination>(Owner);
    if (Destination)
    {
        OwnerName = Destination->DestinationID;
    }

    TryRenameObjectToMatchSlotName();
    UpdateVisualsFromName();

    if (Destination)
    {
        Destination->Slots.Add(this);
    }

}

void USlotSceneComponent::PostEditUndo()
{
    Super::PostEditUndo();
    UpdateVisualsFromName();
}

void USlotSceneComponent::EnsureHelpers()
{
#if WITH_EDITORONLY_DATA

    if (!Arrow)
    {
        Arrow = NewObject<UArrowComponent>(this, TEXT("Arrow"));
        Arrow->SetIsVisualizationComponent(true);
        Arrow->SetUsingAbsoluteRotation(false);
        Arrow->SetHiddenInGame(true);
        Arrow->SetArrowColor(FColor::Cyan);
        Arrow->SetupAttachment(this);
        Arrow->RegisterComponent();
		Arrow->SetRelativeLocation(FVector(0.f, 0.f, 5.f));
        GetOwner()->AddInstanceComponent(Arrow);
    }

    if (!Label)
    {
        Label = NewObject<UTextRenderComponent>(this, TEXT("Label"));
        Label->SetIsVisualizationComponent(true);
        Label->SetHiddenInGame(true);
        Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
        Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextBottom);
        Label->SetTextRenderColor(FColor::Red);
        Label->SetWorldSize(14.f);
        Label->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
        Label->SetupAttachment(this);
        Label->RegisterComponent();
        GetOwner()->AddInstanceComponent(Label);

    }

#endif
}

void USlotSceneComponent::UpdateVisualsFromName()
{
#if WITH_EDITORONLY_DATA
    if (Label)
    {
        FString T = SlotName.IsNone() ? FString(TEXT("(Slot)")) : SlotName.ToString();
        const FText Text = FText::FromString(OwnerName + FString(TEXT(" (")) + T + FString(TEXT(")")));
        Label->SetText(Text);
    }
#endif
}

FName USlotSceneComponent::MakeSafeUniqueName(UObject* Outer, UClass* Class, const FName& Desired)
{
    FString Base = Desired.ToString();
#if WITH_EDITORONLY_DATA
    Base = ObjectTools::SanitizeObjectName(Base);
#else
    Base.ReplaceInline(TEXT(" "), TEXT("_"));
#endif
    const FName CleanBase(*Base);
    const FName Unique = MakeUniqueObjectName(Outer, Class, CleanBase);
    return Unique;
}

void USlotSceneComponent::TryRenameObjectToMatchSlotName()
{
    if (IsTemplate() || !GetOuter()) return;

    if (SlotName.IsNone()) return;

    const FName Desired = MakeSafeUniqueName(GetOuter(), GetClass(), SlotName);

    if (GetFName() == Desired) return;

    const FScopedTransaction Tx(NSLOCTEXT("NamedSlot", "RenameSlotComponent", "Rename Slot Component"));
    Modify();

    Rename(*Desired.ToString(), GetOuter(), REN_DoNotDirty | REN_DontCreateRedirectors);
}

#endif // WITH_EDITOR
