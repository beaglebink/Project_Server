#include "SaveGameHelper.h"
#include "SaveInterface.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/BrushComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


bool USaveGameHelper::IsActorEligibleForSave(const AActor* Actor)
{
    if (!Actor) return false;
    const FString ClassName = Actor->GetClass()->GetName();


    if (ClassName.Contains(TEXT("LightmassImportanceVolume"))) return false;
    if (ClassName.Contains(TEXT("NavMeshBoundsVolume"))) return false;
    if (ClassName.Contains(TEXT("PostProcessVolume"))) return false;
    if (Actor->IsA(APostProcessVolume::StaticClass())) return false;

    if (ClassName.Contains("Brush") ||
        ClassName.Contains("TriggerVolume") ||
        ClassName.Contains("DefaultPhysicsVolume") ||
        ClassName.Contains("NavMesh") ||
        ClassName.Contains("AbstractNavData") ||
        ClassName.Contains("RecastNavMesh") ||
        ClassName.Contains("ReflectionCapture") ||
        ClassName.Contains("SkyLight") ||
        ClassName.Contains("DirectionalLight") ||
        ClassName.Contains("SpotLight") ||
        ClassName.Contains("Atmosphere") ||
        ClassName.Contains("Fog") ||
        ClassName.Contains("Cloud") ||
        ClassName.Contains("PlayerStart") ||
        ClassName.Contains("CameraActor") ||
        ClassName.Contains("StaticMeshActor"))
    {
        return false;
    }

    return true;
}

bool USaveGameHelper::IsComponentEligibleForSave(const UActorComponent* Comp)
{
    if (!Comp) return false;

    if (Comp->IsA(UBrushComponent::StaticClass())) return false;

    const USceneComponent* SceneComp = Cast<USceneComponent>(Comp);
    if (!SceneComp) return false;

    if (SceneComp->Mobility != EComponentMobility::Movable)
        return false;

    const FString ClassName = Comp->GetClass()->GetName();
    if (ClassName.Contains("Light") ||
        ClassName.Contains("ReflectionCapture") ||
        ClassName.Contains("PostProcess") ||
        ClassName.Contains("Atmosphere") ||
        ClassName.Contains("Fog") ||
        ClassName.Contains("Cloud"))
    {
        return false;
    }

    return true;
}

// Сериализация мира
TArray<FActorSaveData> USaveGameHelper::SerializeWorld(UWorld* World)
{
    TArray<FActorSaveData> Result;
    if (!World) return Result;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor || !IsActorEligibleForSave(Actor)) continue;

        FActorSaveData Data;
        Data.ClassName = Actor->GetClass()->GetPathName();
        Data.UniqueID = Actor->GetName();

        // Трансформ
        if (USceneComponent* Root = Actor->GetRootComponent())
        {
            if (USceneComponent* AttachParent = Root->GetAttachParent())
            {
                Data.Transform = Root->GetRelativeTransform();
                if (AActor* ParentActor = AttachParent->GetOwner())
                {
                    Data.AttachParentID = ParentActor->GetName();
                }
                Data.AttachParentComponentPath = AttachParent->GetPathName();
                Data.AttachSocketName = Root->GetAttachSocketName();
            }
            else
            {
                Data.Transform = Actor->GetActorTransform();
            }
        }

        // Сериализация данных актора
        if (Actor->Implements<USaveInterface>())
        {
            ISaveInterface* Saveable = Cast<ISaveInterface>(Actor);
            Saveable->OnPreSave();
            Data.SerializedData = Saveable->SaveToJson();
        }
        else
        {
            Data.SerializedData = SerializeActor(Actor);
        }

        // Сериализация компонентов
        Data.SavedComponents = SerializeComponents(Actor);

        // Сохранение движения
        if (UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
        {
            if (RootComp->IsSimulatingPhysics())
            {
                Data.LinearVelocity = RootComp->GetPhysicsLinearVelocity();
                Data.AngularVelocity = RootComp->GetPhysicsAngularVelocityInDegrees();
            }
        }
        else if (UCharacterMovementComponent* MoveComp = Actor->FindComponentByClass<UCharacterMovementComponent>())
        {
            Data.LinearVelocity = MoveComp->Velocity;
            Data.AngularVelocity = FVector::ZeroVector;
        }

        Result.Add(Data);
    }

    return Result;
}

// Сериализация актора
FString USaveGameHelper::SerializeActor(AActor* Actor)
{
    if (!Actor) return "";
    if (!IsActorEligibleForSave(Actor)) return "";

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    for (TFieldIterator<FProperty> PropIt(Actor->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (Property->HasMetaData(TEXT("SaveGame")))
        {
            // Игнорируем трансформы для неподвижных акторов
            if (Property->GetName() == TEXT("RelativeLocation") ||
                Property->GetName() == TEXT("RelativeRotation") ||
                Property->GetName() == TEXT("RelativeScale3D"))
            {
                continue;
            }

            FString ValueStr;
            Property->ExportTextItem_Direct(
                ValueStr,
                Property->ContainerPtrToValuePtr<void>(Actor),
                Actor, Actor, PPF_None
            );
            JsonObject->SetStringField(Property->GetName(), ValueStr);
        }
    }

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return Output;
}

// Десериализация актора
void USaveGameHelper::DeserializeActor(AActor* Actor, const FString& JsonString)
{
    if (!Actor || JsonString.IsEmpty()) return;
    if (!IsActorEligibleForSave(Actor)) return;

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        for (TFieldIterator<FProperty> PropIt(Actor->GetClass()); PropIt; ++PropIt)
        {
            FProperty* Property = *PropIt;
            if (Property->HasMetaData(TEXT("SaveGame")))
            {
                // Игнорируем трансформы для неподвижных акторов
                if (Property->GetName() == TEXT("RelativeLocation") ||
                    Property->GetName() == TEXT("RelativeRotation") ||
                    Property->GetName() == TEXT("RelativeScale3D"))
                {
                    continue;
                }

                if (JsonObject->HasField(Property->GetName()))
                {
                    FString ValueStr = JsonObject->GetStringField(Property->GetName());
                    Property->ImportText_Direct(
                        *ValueStr,
                        Property->ContainerPtrToValuePtr<void>(Actor),
                        Actor,
                        PPF_None,
                        nullptr
                    );
                }
            }
        }
    }
}

// Десериализация мира
USceneComponent* ResolveParentComponent(const FString& ComponentPath)
{
    UObject* Obj = StaticFindObject(UObject::StaticClass(), nullptr, *ComponentPath);
    return Cast<USceneComponent>(Obj);
}

void USaveGameHelper::DeserializeWorld(UWorld* World, const TArray<FActorSaveData>& SavedActors)
{
    if (!World) return;

    struct FPendingAttach
    {
        AActor* Child;
        FString ParentID;
        FString ParentComponentPath;
        FName SocketName;
        FTransform RelativeTransform;
    };

    TArray<FPendingAttach> PendingAttaches;

    // Первый проход: создаём акторы и восстанавливаем данные
    for (const FActorSaveData& Data : SavedActors)
    {
        AActor* Actor = nullptr;

        // ищем существующий актор
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->GetName() == Data.UniqueID) { Actor = *It; break; }
        }

        // если нет — спавним
        if (!Actor)
        {
            UClass* ActorClass = LoadObject<UClass>(nullptr, *Data.ClassName);
            if (ActorClass && IsActorEligibleForSave(ActorClass->GetDefaultObject<AActor>()))
            {
                FActorSpawnParameters Params;
                Actor = World->SpawnActor<AActor>(ActorClass, Data.Transform, Params);
            }
        }

        if (Actor)
        {
            // восстановление сериализованных данных
            if (Actor->Implements<USaveInterface>())
            {
                ISaveInterface* Saveable = Cast<ISaveInterface>(Actor);
                Saveable->LoadFromJson(Data.SerializedData);
                Saveable->OnPostLoad();
            }
            else
            {
                DeserializeActor(Actor, Data.SerializedData);
            }

            // восстановление компонентов
            DeserializeComponents(Actor, Data.SavedComponents);

            // движение
            if (UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
            {
                if (RootComp->IsSimulatingPhysics())
                {
                    RootComp->SetPhysicsLinearVelocity(Data.LinearVelocity);
                    RootComp->SetPhysicsAngularVelocityInDegrees(Data.AngularVelocity);
                }
            }
            else if (UCharacterMovementComponent* MoveComp = Actor->FindComponentByClass<UCharacterMovementComponent>())
            {
                MoveComp->Velocity = Data.LinearVelocity;
            }

            // attach
            if (!Data.AttachParentID.IsEmpty())
            {
                FPendingAttach AttachInfo;
                AttachInfo.Child = Actor;
                AttachInfo.ParentID = Data.AttachParentID;
                AttachInfo.ParentComponentPath = Data.AttachParentComponentPath;
                AttachInfo.SocketName = Data.AttachSocketName;
                AttachInfo.RelativeTransform = Data.Transform;
                PendingAttaches.Add(AttachInfo);
            }
            else
            {
                if (Actor->GetRootComponent() && Actor->GetRootComponent()->Mobility == EComponentMobility::Movable)
                {
                    Actor->SetActorTransform(Data.Transform);
                }
            }
        }
    }

    // Второй проход: attach
    for (const FPendingAttach& Info : PendingAttaches)
    {
        USceneComponent* ParentComp = ResolveParentComponent(Info.ParentComponentPath);

        if (!ParentComp)
        {
            AActor* Parent = nullptr;
            for (TActorIterator<AActor> It(World); It; ++It)
                if (It->GetName() == Info.ParentID) { Parent = *It; break; }
            ParentComp = Parent ? Parent->GetRootComponent() : nullptr;
        }

        if (!ParentComp) continue;

        if (USceneComponent* ChildRoot = Info.Child->GetRootComponent())
            ChildRoot->SetMobility(EComponentMobility::Movable);

        Info.Child->AttachToComponent(
            ParentComp,
            FAttachmentTransformRules::KeepRelativeTransform,
            Info.SocketName.IsNone() ? NAME_None : Info.SocketName
        );

        Info.Child->SetActorRelativeTransform(Info.RelativeTransform);
    }
}

FString USaveGameHelper::SerializeComponent(UActorComponent* Component)
{
    if (!Component) return "";
    if (!IsComponentEligibleForSave(Component)) return "";

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    for (TFieldIterator<FProperty> PropIt(Component->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (Property->HasMetaData(TEXT("SaveGame")))
        {
            // Игнорируем трансформы для неподвижных компонентов
            if (Property->GetName() == TEXT("RelativeLocation") ||
                Property->GetName() == TEXT("RelativeRotation") ||
                Property->GetName() == TEXT("RelativeScale3D"))
            {
                continue;
            }

            FString ValueStr;
            Property->ExportTextItem_Direct(
                ValueStr,
                Property->ContainerPtrToValuePtr<void>(Component),
                Component, Component, PPF_None
            );
            JsonObject->SetStringField(Property->GetName(), ValueStr);
        }
    }

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return Output;
}

void USaveGameHelper::DeserializeComponent(UActorComponent* Component, const FString& JsonString)
{
    if (!Component || JsonString.IsEmpty()) return;
    if (!IsComponentEligibleForSave(Component)) return;

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        for (TFieldIterator<FProperty> PropIt(Component->GetClass()); PropIt; ++PropIt)
        {
            FProperty* Property = *PropIt;
            if (Property->HasMetaData(TEXT("SaveGame")))
            {
                // Игнорируем трансформы для неподвижных компонентов
                if (Property->GetName() == TEXT("RelativeLocation") ||
                    Property->GetName() == TEXT("RelativeRotation") ||
                    Property->GetName() == TEXT("RelativeScale3D"))
                {
                    continue;
                }

                if (JsonObject->HasField(Property->GetName()))
                {
                    FString ValueStr = JsonObject->GetStringField(Property->GetName());
                    Property->ImportText_Direct(
                        *ValueStr,
                        Property->ContainerPtrToValuePtr<void>(Component),
                        Component,
                        PPF_None,
                        nullptr
                    );
                }
            }
        }
    }
}

TArray<FComponentSaveData> USaveGameHelper::SerializeComponents(AActor* Actor)
{
    TArray<FComponentSaveData> Result;
    if (!Actor) return Result;

    for (UActorComponent* Comp : Actor->GetComponents())
    {
        if (!IsComponentEligibleForSave(Comp)) continue;

        FComponentSaveData CompData;
        CompData.ComponentName = Comp->GetName();
        CompData.SerializedData = SerializeComponent(Comp);

        if (USceneComponent* SceneComp = Cast<USceneComponent>(Comp))
        {
            if (USceneComponent* Parent = SceneComp->GetAttachParent())
            {
                CompData.AttachParentName = Parent->GetName();
                CompData.AttachSocketName = SceneComp->GetAttachSocketName();
                CompData.RelativeTransform = SceneComp->GetRelativeTransform();
            }
        }

        Result.Add(CompData);
    }

    return Result;
}

void USaveGameHelper::DeserializeComponents(AActor* Actor, const TArray<FComponentSaveData>& SavedComponents)
{
    if (!Actor) return;

    for (const FComponentSaveData& CompData : SavedComponents)
    {
        UActorComponent* FoundComp = nullptr;

        // ищем компонент по имени
        for (UActorComponent* Comp : Actor->GetComponents())
        {
            if (Comp && Comp->GetName() == CompData.ComponentName)
            {
                FoundComp = Comp;
                break;
            }
        }

        if (!FoundComp || !IsComponentEligibleForSave(FoundComp)) continue;

        // восстановление сериализованных данных
        DeserializeComponent(FoundComp, CompData.SerializedData);

        // если это SceneComponent — восстанавливаем attach
        if (USceneComponent* SceneComp = Cast<USceneComponent>(FoundComp))
        {
            if (!CompData.AttachParentName.IsEmpty())
            {
                UActorComponent* ParentComp = nullptr;
                for (UActorComponent* Comp : Actor->GetComponents())
                {
                    if (Comp && Comp->GetName() == CompData.AttachParentName)
                    {
                        ParentComp = Comp;
                        break;
                    }
                }

                if (USceneComponent* ParentScene = Cast<USceneComponent>(ParentComp))
                {
                    SceneComp->AttachToComponent(
                        ParentScene,
                        FAttachmentTransformRules::KeepRelativeTransform,
                        CompData.AttachSocketName
                    );

                    SceneComp->SetRelativeTransform(CompData.RelativeTransform);
                }
            }
        }
    }
}

void USaveGameHelper::ClearWorld(UWorld* World, const TArray<FActorSaveData>& SavedActors)
{
    if (!World) return;

    TSet<FString> SavedIDs;
    for (const FActorSaveData& Data : SavedActors)
    {
        SavedIDs.Add(Data.UniqueID);
    }

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor || !IsActorEligibleForSave(Actor)) continue;

        FString ID = Actor->GetName();
        if (!SavedIDs.Contains(ID))
        {
            Actor->Destroy();
        }
    }
}