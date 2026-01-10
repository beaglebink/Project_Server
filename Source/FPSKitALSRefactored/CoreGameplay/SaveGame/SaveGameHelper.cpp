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

// Проверка акторов
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

// Проверка компонентов
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

// Сериализация актора
FString USaveGameHelper::SerializeActor(AActor* Actor)
{
    if (!Actor) return "";

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    for (TFieldIterator<FProperty> PropIt(Actor->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (Property->HasMetaData(TEXT("SaveGame")))
        {
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

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        for (TFieldIterator<FProperty> PropIt(Actor->GetClass()); PropIt; ++PropIt)
        {
            FProperty* Property = *PropIt;
            if (Property->HasMetaData(TEXT("SaveGame")))
            {
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

// Сериализация мира
TArray<FActorSaveData> USaveGameHelper::SerializeWorld(UWorld* World)
{
    TArray<FActorSaveData> Result;
    if (!World) return Result;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!Actor) continue;

        FActorSaveData Data;
        Data.ClassName = Actor->GetClass()->GetPathName();
        Data.UniqueID = Actor->GetName();

        const bool bIsNormal = IsActorEligibleForSave(Actor);
        Data.bSkipTransformRestore = !bIsNormal;

        if (bIsNormal)
        {
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

            // Сохраняем скорости для движущихся акторов
            if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
            {
                Data.LinearVelocity = RootPrimitive->GetPhysicsLinearVelocity();
                Data.AngularVelocity = RootPrimitive->GetPhysicsAngularVelocityInDegrees();
            }

            UE_LOG(LogTemp, Log, TEXT("[SerializeWorld] Actor %s: трансформ и скорости сохранены. LinVel=%s, AngVel=%s"),
                *Actor->GetName(),
                *Data.LinearVelocity.ToString(),
                *Data.AngularVelocity.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("[SerializeWorld] Actor %s: особый объект, трансформ и скорости НЕ сохраняются."),
                *Actor->GetName());
        }

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

        Data.SavedComponents = SerializeComponents(Actor);
        Result.Add(Data);
    }

    return Result;
}

USceneComponent* ResolveParentComponent(const FString& ComponentPath)
{
    UObject* Obj = StaticFindObject(USceneComponent::StaticClass(), nullptr, *ComponentPath);
    return Cast<USceneComponent>(Obj);
}

// Десериализация мира
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
        bool bSkipTransformRestore;
        FVector LinearVelocity;
        FVector AngularVelocity;
    };

    TArray<FPendingAttach> PendingAttaches;

    // Первый проход: создаём акторы и восстанавливаем данные
    for (const FActorSaveData& Data : SavedActors)
    {
        AActor* Actor = nullptr;

        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->GetName() == Data.UniqueID) { Actor = *It; break; }
        }

        if (!Actor)
        {
            UClass* ActorClass = LoadObject<UClass>(nullptr, *Data.ClassName);
            if (ActorClass)
            {
                FActorSpawnParameters Params;
                Actor = World->SpawnActor<AActor>(ActorClass, Data.Transform, Params);
            }
        }

        if (Actor)
        {
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

            DeserializeComponents(Actor, Data.SavedComponents);

            if (!Data.AttachParentID.IsEmpty())
            {
                FPendingAttach AttachInfo;
                AttachInfo.Child = Actor;
                AttachInfo.ParentID = Data.AttachParentID;
                AttachInfo.ParentComponentPath = Data.AttachParentComponentPath;
                AttachInfo.SocketName = Data.AttachSocketName;
                AttachInfo.RelativeTransform = Data.Transform;
                AttachInfo.bSkipTransformRestore = Data.bSkipTransformRestore;
                AttachInfo.LinearVelocity = Data.LinearVelocity;
                AttachInfo.AngularVelocity = Data.AngularVelocity;
                PendingAttaches.Add(AttachInfo);
            }
            else
            {
                if (!Data.bSkipTransformRestore)
                {
                    if (Actor->GetRootComponent() && Actor->GetRootComponent()->Mobility == EComponentMobility::Movable)
                    {
                        Actor->SetActorTransform(Data.Transform);

                        if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
                        {
                            RootPrimitive->SetPhysicsLinearVelocity(Data.LinearVelocity);
                            RootPrimitive->SetPhysicsAngularVelocityInDegrees(Data.AngularVelocity);
                        }

                        UE_LOG(LogTemp, Log, TEXT("[DeserializeWorld] Actor %s: трансформ и скорости восстановлены. LinVel=%s, AngVel=%s"),
                            *Actor->GetName(),
                            *Data.LinearVelocity.ToString(),
                            *Data.AngularVelocity.ToString());
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("[DeserializeWorld] Actor %s: особый объект, трансформ и скорости НЕ восстанавливаются."),
                        *Actor->GetName());
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

        if (!Info.bSkipTransformRestore)
        {
            Info.Child->SetActorRelativeTransform(Info.RelativeTransform);

            if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Info.Child->GetRootComponent()))
            {
                RootPrimitive->SetPhysicsLinearVelocity(Info.LinearVelocity);
                RootPrimitive->SetPhysicsAngularVelocityInDegrees(Info.AngularVelocity);
            }

            UE_LOG(LogTemp, Log, TEXT("[DeserializeWorld] Actor %s: attach + трансформ и скорости восстановлены."),
                *Info.Child->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("[DeserializeWorld] Actor %s: attach выполнен, трансформ и скорости НЕ восстановлены."),
                *Info.Child->GetName());
        }
    }
}

// Сериализация компонентов
TArray<FComponentSaveData> USaveGameHelper::SerializeComponents(AActor* Actor)
{
    TArray<FComponentSaveData> Result;
    if (!Actor) return Result;

    const bool bIsNormal = IsActorEligibleForSave(Actor);

    for (UActorComponent* Comp : Actor->GetComponents())
    {
        FComponentSaveData CompData;
        CompData.ComponentName = Comp->GetName();
        CompData.SerializedData = SerializeComponent(Comp);

        if (USceneComponent* SceneComp = Cast<USceneComponent>(Comp))
        {
            if (USceneComponent* Parent = SceneComp->GetAttachParent())
            {
                CompData.AttachParentName = Parent->GetName();
                CompData.AttachSocketName = SceneComp->GetAttachSocketName();

                if (bIsNormal)
                {
                    CompData.RelativeTransform = SceneComp->GetRelativeTransform();
                    CompData.bSkipTransformRestore = false;
                    UE_LOG(LogTemp, Log, TEXT("[SerializeComponents] %s.%s: относительный трансформ сохранён."),
                        *Actor->GetName(), *SceneComp->GetName());
                }
                else
                {
                    CompData.bSkipTransformRestore = true;
                    UE_LOG(LogTemp, Log, TEXT("[SerializeComponents] %s.%s: особый компонент, трансформ НЕ сохраняется."),
                        *Actor->GetName(), *SceneComp->GetName());
                }
            }
        }

        Result.Add(CompData);
    }

    return Result;
}

// Десериализация компонентов
void USaveGameHelper::DeserializeComponents(AActor* Actor, const TArray<FComponentSaveData>& SavedComponents)
{
    if (!Actor) return;

    for (const FComponentSaveData& CompData : SavedComponents)
    {
        UActorComponent* FoundComp = nullptr;

        for (UActorComponent* Comp : Actor->GetComponents())
        {
            if (Comp && Comp->GetName() == CompData.ComponentName)
            {
                FoundComp = Comp;
                break;
            }
        }

        if (!FoundComp) continue;

        DeserializeComponent(FoundComp, CompData.SerializedData);

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

                    if (!CompData.bSkipTransformRestore)
                    {
                        SceneComp->SetRelativeTransform(CompData.RelativeTransform);
                        UE_LOG(LogTemp, Log, TEXT("[DeserializeComponents] %s.%s: относительный трансформ восстановлен."),
                            *Actor->GetName(), *SceneComp->GetName());
                    }
                    else
                    {
                        UE_LOG(LogTemp, Log, TEXT("[DeserializeComponents] %s.%s: особый компонент, трансформ НЕ восстановлен."),
                            *Actor->GetName(), *SceneComp->GetName());
                    }
                }
            }
        }
    }
}

// Сериализация компонента
FString USaveGameHelper::SerializeComponent(UActorComponent* Component)
{
    if (!Component) return "";

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

    for (TFieldIterator<FProperty> PropIt(Component->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (Property->HasMetaData(TEXT("SaveGame")))
        {
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

    UE_LOG(LogTemp, Log, TEXT("[SerializeComponent] Компонент %s: данные сериализованы."), *Component->GetName());

    return Output;
}

// Десериализация компонента
void USaveGameHelper::DeserializeComponent(UActorComponent* Component, const FString& JsonString)
{
    if (!Component || JsonString.IsEmpty()) return;

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        for (TFieldIterator<FProperty> PropIt(Component->GetClass()); PropIt; ++PropIt)
        {
            FProperty* Property = *PropIt;
            if (Property->HasMetaData(TEXT("SaveGame")))
            {
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
        UE_LOG(LogTemp, Log, TEXT("[DeserializeComponent] Компонент %s: данные восстановлены."), *Component->GetName());
    }
}

// Очистка мира
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
        if (!Actor) continue;

        FString ID = Actor->GetName();
        if (!SavedIDs.Contains(ID))
        {
            UE_LOG(LogTemp, Log, TEXT("[ClearWorld] Actor %s: не найден в сохранении, удаляется."), *Actor->GetName());
            Actor->Destroy();
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("[ClearWorld] Actor %s: найден в сохранении, остаётся."), *Actor->GetName());
        }
    }
}
