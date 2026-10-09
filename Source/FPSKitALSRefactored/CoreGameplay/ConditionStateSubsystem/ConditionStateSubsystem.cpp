#include "ConditionStateSubsystem.h"
#include "../SaveGame/GameSaveSubsystem.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// ─────────────────────────────────────────────────────────────────────────────
// Initialize / Deinitialize
// ─────────────────────────────────────────────────────────────────────────────
void UConditionStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Форсируем инициализацию системы сохранения ДО нас. Без этого
    // GetSubsystem<UGameSaveSubsystem>() может вернуть nullptr, и регистрация
    // молча не сработает — окна условий не переживут save/load.
    Collection.InitializeDependency<UGameSaveSubsystem>();

    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->RegisterSaveableSubsystem(this);
        UE_LOG(LogTemp, Log,
            TEXT("ConditionStateSubsystem: registered as saveable subsystem."));
    }
    else
    {
        // Сюда попадать не должны — если попали, значит в проекте нет
        // UGameSaveSubsystem или его инициализация вынесена за пределы
        // GameInstance. Тогда окна условий работать не будут, но и молча
        // терять данные тоже плохо — поэтому Warning.
        UE_LOG(LogTemp, Warning,
            TEXT("ConditionStateSubsystem: UGameSaveSubsystem not found — "
                "rise times will NOT persist across saves."));
    }
}

void UConditionStateSubsystem::Deinitialize()
{
    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->UnregisterSaveableSubsystem(this);
    }

    RiseTimes.Empty();

    Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
// Запросы / мутации
// ─────────────────────────────────────────────────────────────────────────────
FDateTime UConditionStateSubsystem::FindRiseTime(FName Key) const
{
    if (const FDateTime* Found = RiseTimes.Find(Key))
        return *Found;
    return FDateTime::MinValue();
}

void UConditionStateSubsystem::SetRiseTime(FName Key, FDateTime Time)
{
    RiseTimes.Add(Key, Time);
}

void UConditionStateSubsystem::ClearRiseTime(FName Key)
{
    RiseTimes.Remove(Key);
}

// ─────────────────────────────────────────────────────────────────────────────
// Save / Load
// ─────────────────────────────────────────────────────────────────────────────
void UConditionStateSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
    OutData.SubsystemName = GetSaveSubsystemName();

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> Arr;

    for (const auto& Pair : RiseTimes)
    {
        // FDateTime::MinValue() не пишем — смысла нет.
        if (Pair.Value == FDateTime::MinValue()) continue;

        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("Key"), Pair.Key.ToString());
        Obj->SetStringField(TEXT("Time"), Pair.Value.ToIso8601());
        Arr.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("RiseTimes"), Arr);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    OutData.SerializedData = Output;
}

void UConditionStateSubsystem::ApplySaveData(const FSubsystemSaveData& InData)
{
    bLoadComplete = false;
    RiseTimes.Empty();

    if (InData.SerializedData.IsEmpty())
    {
        bLoadComplete = true;
        return;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InData.SerializedData);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        bLoadComplete = true;
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
    if (Root->TryGetArrayField(TEXT("RiseTimes"), Arr))
    {
        for (const TSharedPtr<FJsonValue>& Val : *Arr)
        {
            const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
            if (!Val->TryGetObject(ObjPtr)) continue;

            FString KeyStr;
            FString TimeStr;
            if (!(*ObjPtr)->TryGetStringField(TEXT("Key"), KeyStr)) continue;
            if (!(*ObjPtr)->TryGetStringField(TEXT("Time"), TimeStr)) continue;

            FDateTime Parsed;
            if (FDateTime::ParseIso8601(*TimeStr, Parsed))
            {
                RiseTimes.Add(FName(*KeyStr), Parsed);
            }
        }
    }

    bLoadComplete = true;
}