#include "TeleportDestination.h"
#include "Components/SceneComponent.h"
#include "SlotSceneComponent.h"
#include "UObject/UnrealType.h"          
#include "Components/TextRenderComponent.h"


#if WITH_EDITOR
#include "ScopedTransaction.h"           
#include "UObject/UObjectGlobals.h"      
#endif
#include "TeleportingSubsystem.h"

ATeleportDestination::ATeleportDestination()
{
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
#if WITH_EDITORONLY_DATA
    LabelComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Label"));
    LabelComponent->SetupAttachment(RootComponent);
    LabelComponent->bIsScreenSizeScaled = true;
    LabelComponent->SetHiddenInGame(true);
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

UTeleportingSubsystem* GetTeleportingSubsystem(UObject* Context)
{
    if (!Context) return nullptr;

    UWorld* World = Context->GetWorld();
    if (!World) return nullptr;

    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return nullptr;

    return GI->GetSubsystem<UTeleportingSubsystem>();
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

void ATeleportDestination::BeginPlay()
{
    Super::BeginPlay();

    UTeleportingSubsystem* TeleportSubsystem = GetTeleportingSubsystem(this);

    if (TeleportSubsystem)
    {
        TeleportSubsystem->RegistrationTeleportingDestination(this);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TeleportingSubsystem not found!"));
    }

}

void ATeleportDestination::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    UTeleportingSubsystem* TeleportSubsystem = GetTeleportingSubsystem(this);

    if (TeleportSubsystem)
    {
        TeleportSubsystem->UnregistrationTeleportingDestination(this);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TeleportingSubsystem not found during EndPlay!"));
    }

}

USlotSceneComponent* ATeleportDestination::AddSlot()
{
#if WITH_EDITOR
    const FScopedTransaction Tx(NSLOCTEXT("TeleportDestination", "AddSlotTx", "Add Teleport Slot"));
    Modify();

    const FName UniqueName = GenerateUniqueSlotName(TEXT("Slot"));

    USlotSceneComponent* NewSlot = NewObject<USlotSceneComponent>(this, USlotSceneComponent::StaticClass(), UniqueName, RF_Transactional);

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

TArray<USlotSceneComponent*> ATeleportDestination::GetSlots() const
{
    return Slots;
}

USlotSceneComponent* ATeleportDestination::GetSlotByName(const FName& SlotName) const
{
    for (USlotSceneComponent* Slot : Slots)
    {
        if (Slot && Slot->SlotName == SlotName)
        {
            return Slot;
        }
    }
    return nullptr;
}

USlotSceneComponent* ATeleportDestination::GetSlotByIndex(int32 Index) const
{
    if (Slots.IsValidIndex(Index))
    {
        return Slots[Index];
    }
    return nullptr;
}

void ATeleportDestination::SetActiveDestination(bool bActive)
{
    IsActiveDestination = bActive;
    OnChangeActiveDestination.Broadcast(this, IsActiveDestination);
}

bool ATeleportDestination::GetActiveDestination() const
{
    return IsActiveDestination;
}

void ATeleportDestination::StartCooldown()
{
    if (isCooldown) return;

    isCooldown = true;
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, [&]()
        {
            isCooldown = false;
            OnDestinationFinishCooldown.Broadcast(this);
        }, CoolDownTime, false);
}

void ATeleportDestination::RemoveSlot(USlotSceneComponent* SlotToRemove)
{
    if (!SlotToRemove)
    {
        UE_LOG(LogTemp, Warning, TEXT("RemoveSlot called with nullptr"));
        return;
    }

    const int32 Index = Slots.IndexOfByKey(SlotToRemove);
    if (Index == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning, TEXT("Slot not found in array, skipping removal"));
        return;
    }

    Modify();

    Slots.RemoveAt(Index);

    UE_LOG(LogTemp, Warning, TEXT("Removed slot at index %d"), Index);
#if WITH_EDITOR
    if (GEditor)
    {
        GEditor->NoteSelectionChange();
    }
#endif // WITH_EDITOR
}

#if WITH_EDITOR

FName ATeleportDestination::GenerateUniqueSlotName(const FName& Base) const
{
    TSet<FName> UsedNames;
    for (const USlotSceneComponent* S : Slots)
    {
        if (S)
        {
            UsedNames.Add(S->SlotName);
        }
    }

    int32 Index = 1;
    FName Candidate;
    do
    {
        Candidate = *FString::Printf(TEXT("%s%d"), *Base.ToString(), Index++);
    } while (UsedNames.Contains(Candidate));

    return Candidate;
}

void ATeleportDestination::EnsureSlotAttached(USlotSceneComponent* Slot)
{
    if (!Slot || Slot->GetOwner() != this)
    {
        return;
    }

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
    if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == FName("Slots"))
    {
        UE_LOG(LogTemp, Warning, TEXT("Slots array changed"));

        if (Slots.Num() > 0 && Slots.Last() == nullptr)
        {
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
        GEditor->NoteSelectionChange();
    }

    for (USlotSceneComponent* Slot : Slots)
    {
        EnsureSlotAttached(Slot);
    }

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

    for (USlotSceneComponent* Slot : Slots)
    {
        EnsureSlotAttached(Slot);
    }
}

#endif // WITH_EDITOR