#include "TeleportDestination.h"
#include "Components/SceneComponent.h"
#include "SlotSceneComponent.h"
#include "UObject/UnrealType.h"          // FPropertyChangedEvent, GET_MEMBER_NAME_CHECKED

#if WITH_EDITOR
#include "ScopedTransaction.h"           // FScopedTransaction (requires UnrealEd in Build.cs for editor builds)
#include "UObject/UObjectGlobals.h"      // NewObject
#endif

ATeleportDestination::ATeleportDestination()
{
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

#if WITH_EDITOR
    SetFlags(RF_Transactional);
#endif
}

// Важно: даем тело и для не-редакторских сборок, чтобы не было линковочных ошибок
USlotSceneComponent* ATeleportDestination::AddSlot()
{
#if WITH_EDITOR
    const FScopedTransaction Tx(NSLOCTEXT("TeleportDestination", "AddSlotTx", "Add Teleport Slot"));
    Modify();

    const FName UniqueName = GenerateUniqueSlotName(TEXT("Slot"));

    USlotSceneComponent* NewSlot = NewObject<USlotSceneComponent>(
        this,
        USlotSceneComponent::StaticClass(),
        UniqueName,
        RF_Transactional | RF_Public
    );

    NewSlot->SetFlags(RF_Transactional);
    NewSlot->SlotName = UniqueName;

    NewSlot->OnComponentCreated();
    NewSlot->SetupAttachment(RootComponent);
    NewSlot->RegisterComponent();
    AddInstanceComponent(NewSlot);

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
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ATeleportDestination, Slots))
    {
        // Страхуемся на случай ручных правок массива в Details
        for (USlotSceneComponent* Slot : Slots)
        {
            EnsureSlotAttached(Slot);
        }
    }
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