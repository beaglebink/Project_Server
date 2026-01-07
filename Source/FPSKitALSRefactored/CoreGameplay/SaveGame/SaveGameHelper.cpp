#include "SaveGameHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

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
                Actor,
                Actor,
                PPF_None
            );
            JsonObject->SetStringField(Property->GetName(), ValueStr);
        }
    }

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
    return Output;
}

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
                Component,
                Component,
                PPF_None
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
    }
}

TArray<FComponentSaveData> USaveGameHelper::SerializeComponents(AActor* Actor)
{
    TArray<FComponentSaveData> SavedComponents;
    if (!Actor) return SavedComponents;

    for (UActorComponent* Comp : Actor->GetComponents())
    {
        if (USceneComponent* SceneComp = Cast<USceneComponent>(Comp))
        {
            FComponentSaveData Data;
            Data.UniqueID = SceneComp->GetName();
            Data.ClassName = SceneComp->GetClass()->GetPathName();
            Data.Transform = SceneComp->GetRelativeTransform();

            if (SceneComp->GetAttachParent())
            {
                Data.ParentID = SceneComp->GetAttachParent()->GetName();
            }

            Data.SerializedData = SerializeComponent(SceneComp);
            SavedComponents.Add(Data);
        }
    }

    return SavedComponents;
}

void USaveGameHelper::DeserializeComponents(AActor* Actor, const TArray<FComponentSaveData>& SavedComponents)
{
    if (!Actor) return;

    for (UActorComponent* Comp : Actor->GetComponents())
    {
        if (Comp != Actor->GetRootComponent())
        {
            Comp->DestroyComponent();
        }
    }

    for (const FComponentSaveData& Data : SavedComponents)
    {
        UClass* CompClass = LoadObject<UClass>(nullptr, *Data.ClassName);
        if (!CompClass) continue;

        USceneComponent* SceneComp = NewObject<USceneComponent>(Actor, CompClass, FName(*Data.UniqueID));
        SceneComp->RegisterComponent();
        SceneComp->SetRelativeTransform(Data.Transform);

        DeserializeComponent(SceneComp, Data.SerializedData);

        if (!Data.ParentID.IsEmpty())
        {
            USceneComponent* Parent = nullptr;
            for (UActorComponent* Comp : Actor->GetComponents())
            {
                if (Comp && Comp->GetName() == Data.ParentID)
                {
                    Parent = Cast<USceneComponent>(Comp);
                    break;
                }
            }

            if (Parent)
            {
                SceneComp->AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform);
            }
        }
        else
        {
            SceneComp->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        }
    }
}