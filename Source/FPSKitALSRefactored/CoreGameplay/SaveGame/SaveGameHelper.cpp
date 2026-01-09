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
        if (!Actor) continue;
        if (!IsActorEligibleForSave(Actor)) continue;

        FActorSaveData Data;
        Data.ClassName = Actor->GetClass()->GetPathName();
        Data.Transform = Actor->GetActorTransform();

        if (Actor->Implements<USaveInterface>())
        {
            ISaveInterface* Saveable = Cast<ISaveInterface>(Actor);
            Saveable->OnPreSave();
            Data.UniqueID = Saveable->GetSaveID();
            Data.SerializedData = Saveable->SaveToJson();
        }
        else
        {
            Data.UniqueID = Actor->GetName();
            Data.SerializedData = SerializeActor(Actor);
        }

        Data.SavedComponents = SerializeComponents(Actor);

        if (AActor* Parent = Actor->GetAttachParentActor())
        {
            Data.AttachParentID = Parent->GetName();
            if (USceneComponent* RootComp = Actor->GetRootComponent())
            {
                Data.AttachSocketName = RootComp->GetAttachSocketName();
            }
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
void USaveGameHelper::DeserializeWorld(UWorld* World, const TArray<FActorSaveData>& SavedActors)
{
    if (!World) return;

    for (const FActorSaveData& Data : SavedActors)
    {
        AActor* Actor = nullptr;

        // ищем существующий актор
        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->GetName() == Data.UniqueID)
            {
                Actor = *It;
                break;
            }
        }

        // если не нашли — создаём новый
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

            // не трогаем статические акторы
            if (Actor->GetRootComponent() && Actor->GetRootComponent()->Mobility == EComponentMobility::Movable)
            {
                Actor->SetActorTransform(Data.Transform);
            }

            DeserializeComponents(Actor, Data.SavedComponents);

            if (!Data.AttachParentID.IsEmpty())
            {
                for (TActorIterator<AActor> It(World); It; ++It)
                {
                    if (It->GetName() == Data.AttachParentID)
                    {
                        AActor* Parent = *It;
                        if (USceneComponent* ParentComp = Parent->GetRootComponent())
                        {
                            Actor->AttachToComponent(
                                ParentComp,
                                FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                                Data.AttachSocketName
                            );
                        }
                        break;
                    }
                }
            }
        }
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

        for (UActorComponent* Comp : Actor->GetComponents())
        {
            if (Comp && Comp->GetName() == CompData.ComponentName)
            {
                FoundComp = Comp;
                break;
            }
        }

        if (!FoundComp) continue;
        if (!IsComponentEligibleForSave(FoundComp)) continue;

        DeserializeComponent(FoundComp, CompData.SerializedData);
    }
}