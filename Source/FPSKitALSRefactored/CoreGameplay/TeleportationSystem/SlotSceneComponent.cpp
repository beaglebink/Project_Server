#include "SlotSceneComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#if WITH_EDITOR
#include "ScopedTransaction.h"
#include "UObject/UObjectGlobals.h"
#if WITH_EDITORONLY_DATA
// ��� ��������� ����� � � .Build.cs �������� �������� ����������� �� UnrealEd � Editor-�������
#include "ObjectTools.h"
#endif
#endif

USlotSceneComponent::USlotSceneComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetMobility(EComponentMobility::Movable);
    bHiddenInGame = true; // ����� � ����������� ���������
}

void USlotSceneComponent::OnRegister()
{
    Super::OnRegister();

    // ��������������� ������������� � ����� ������, ���� ��������� �� ���������� ����
    if (!GetAttachParent())
    {
        if (AActor* Owner = GetOwner())
        {
            if (USceneComponent* Root = Owner->GetRootComponent())
            {
                //SetupAttachment(Root);
				AttachToComponent(Root, FAttachmentTransformRules::KeepRelativeTransform);
            }
        }
    }

#if WITH_EDITOR
    EnsureHelpers();
    UpdateVisualsFromName();

    if (GEditor)
    {
        UE_LOG(LogTemp, Warning, TEXT("AddSlot-- %s"), *GetName());
        GEditor->NoteSelectionChange(); // ��������� Details � ������������ �����
    }
#endif
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
        // ������ UI ������������������
        UpdateVisualsFromName();

        // ����������� � ������������� ��� ������ ���������� ��� SlotName (��� ������ ��������)
        TryRenameObjectToMatchSlotName();
    }
}

void USlotSceneComponent::PostDuplicate(bool bDuplicateForPIE)
{
    Super::PostDuplicate(bDuplicateForPIE);
    // ������������ � ��������� � ��������� ���������� �����
    TryRenameObjectToMatchSlotName();
    UpdateVisualsFromName();
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
        GetOwner()->AddInstanceComponent(Arrow);
    }

    if (!Label)
    {
        Label = NewObject<UTextRenderComponent>(this, TEXT("Label"));
        Label->SetIsVisualizationComponent(true);
        Label->SetHiddenInGame(true);
        Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
        Label->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
        Label->SetTextRenderColor(FColor::White);
        Label->SetWorldSize(24.f);
        Label->SetRelativeLocation(FVector(0.f, 0.f, 28.f));
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
        const FText Text = SlotName.IsNone() ? FText::FromString(TEXT("(Slot)")) : FText::FromName(SlotName);
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
    if (IsTemplate() || !GetOuter()) return; // �� ������� CDO/�������

    // ������ ��� �� �������
    if (SlotName.IsNone()) return;

    const FName Desired = MakeSafeUniqueName(GetOuter(), GetClass(), SlotName);

    if (GetFName() == Desired) return;

    const FScopedTransaction Tx(NSLOCTEXT("NamedSlot", "RenameSlotComponent", "Rename Slot Component"));
    Modify();

    // Rename ������ ���� �� Outer, ����� ��������� �������
    Rename(*Desired.ToString(), GetOuter(), REN_DoNotDirty | REN_DontCreateRedirectors);
}

#endif // WITH_EDITOR
