#include "EditorServiceSubsystem.h"
#include "Editor.h"
#include "CoreGameplay/InteriorInstanceSystem/FloorAssignmentComponent.h"

void UEditorServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FEditorDelegates::OnNewActorsPlaced.AddUObject(this, &UEditorServiceSubsystem::OnNewActorsPlaced);
}

void UEditorServiceSubsystem::Deinitialize()
{
    FEditorDelegates::OnNewActorsPlaced.RemoveAll(this);
    Super::Deinitialize();
}

void UEditorServiceSubsystem::OnNewActorsPlaced(UObject* Context, const TArray<AActor*>& NewActors)
{
    for (AActor* NewActor : NewActors)
    {
        if (!NewActor)
            continue;

        UFloorAssignmentComponent* FloorComp = NewActor->FindComponentByClass<UFloorAssignmentComponent>();
        if (FloorComp)
        {
            FloorComp->ItemId = FGuid::NewGuid();
            FloorComp->ProtectedItemId = FloorComp->ItemId;
            FloorComp->MarkPackageDirty();
        }
    }
}