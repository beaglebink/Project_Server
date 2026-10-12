#include "ScenarioManagerSubsystem.h"

#include "PayloadBatchAsset.h"
#include "PayloadBatchEntry.h"
#include "OutcomeConditionAsset.h"
#include "Engine/GameInstance.h"
#include "Engine/AssetManager.h"
#include "JsonObjectConverter.h"
#include "SaveGame/GameSaveSubsystem.h"
#include <WorldStateSubsystem.h>
#include <MissionSubsystem.h>
#include <SpawnGroupSubsystem.h>
#include <TerminalSystem/TerminalSubsystem.h>

DEFINE_LOG_CATEGORY_STATIC(LogScenario, Log, All);

// =============================================================================
// Lifecycle
// =============================================================================

void UScenarioManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Collection.InitializeDependency<UEventBusSubsystem>();
    Collection.InitializeDependency<UGameSaveSubsystem>();
    Collection.InitializeDependency<UWorldStateSubsystem>();
    Collection.InitializeDependency<UMissionSubsystem>();
    Collection.InitializeDependency<USpawnGroupSubsystem>();
    Collection.InitializeDependency<UTerminalSubsystem>();

    LoadAllSteps();
    SubscribeToEventBus();
    SubscribeToInitialLevelLoaded();

    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->RegisterSaveableSubsystem(this);
    }

    // ---- Fallback-триггер ----
    // Если LevelLoaded не придёт за InitialReevaluateFallbackDelay секунд
    // (например, стартовая карта грузится без публикации LevelLoaded),
    // стартуем сценарий по таймеру.
    //
    // 1 секунда — эмпирика: она гарантированно покрывает BeginPlay всех
    // актёров, спавн динамических актёров, но меньше времени до первого
    // перехода между уровнями.
    if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
    {
        World->GetTimerManager().SetTimer(
            InitialReevaluateFallbackTimer,
            FTimerDelegate::CreateWeakLambda(this, [this]()
                {
                    if (!IsValid(this) || bFirstReevaluateDone) return;
                    UE_LOG(LogScenario, Log,
                        TEXT("ScenarioManager: LevelLoaded didn't arrive — fallback timer fired, running initial ReevaluateSteps"));
                    TryRunInitialReevaluate(TEXT("FallbackTimer"));
                }),
            InitialReevaluateFallbackDelay,
            /*bLoop=*/false);
    }

    UE_LOG(LogScenario, Log,
        TEXT("ScenarioManager: Initialize done. Steps loaded = %d, waiting for Interior::LevelLoaded or fallback timer (%0.1fs)..."),
        Steps.Num(), InitialReevaluateFallbackDelay);
}

void UScenarioManagerSubsystem::Deinitialize()
{
    if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
    {
        World->GetTimerManager().ClearTimer(InitialReevaluateFallbackTimer);
    }

    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        if (TagChangeHandler.IsValid())
        {
            EventBus->UnregisterHandler(TagChangeHandler);
            TagChangeHandler.Invalidate();
        }
        if (InitialLevelLoadedHandler.IsValid())
        {
            EventBus->UnregisterHandler(InitialLevelLoadedHandler);
            InitialLevelLoadedHandler.Invalidate();
        }
    }

    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->UnregisterSaveableSubsystem(this);
    }

    Steps.Empty();
    TagStorage.Empty();
    ActiveStepIds.Empty();

    Super::Deinitialize();
}

void UScenarioManagerSubsystem::SubscribeToInitialLevelLoaded()
{
    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    InitialLevelLoadedCondition = NewObject<UOutcomeConditionAsset>(this);
    InitialLevelLoadedCondition->OperatorType = EConditionOperator::Composite;
    InitialLevelLoadedCondition->FilterRow.OutcomeType = EOutcomeType::Interior;
    InitialLevelLoadedCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    InitialLevelLoadedCondition->FilterRow.InteriorType = EOutcomeInterior::LevelLoaded;
    InitialLevelLoadedCondition->FilterRow.InteriorComparison = EConditionComparison::Equals;
    InitialLevelLoadedCondition->CompileCondition();

    if (!InitialLevelLoadedCondition->GetCondition().IsValid())
    {
        UE_LOG(LogScenario, Warning,
            TEXT("ScenarioManager: failed to compile LevelLoaded condition"));
        return;
    }

    InitialLevelLoadedHandler = EventBus->RegisterHandler(
        InitialLevelLoadedCondition,
        FOutcomeHandlerDelegate::CreateUObject(
            this, &UScenarioManagerSubsystem::OnInitialLevelLoaded));
}

void UScenarioManagerSubsystem::TryRunInitialReevaluate(const TCHAR* Reason)
{
    // Первый запуск уже был — молча выходим.
    if (bFirstReevaluateDone)
        return;

    // Сейв ещё не применён — ждём. И таймер, и событие могут пройти мимо;
    // повторит тот, кто придёт позже (или следующий LevelLoaded на переходе).
    if (!bLoadComplete)
    {
        UE_LOG(LogScenario, Log,
            TEXT("ScenarioManager: initial Reevaluate deferred (%s) — save not applied yet"), Reason);
        return;
    }

    bFirstReevaluateDone = true;

    // Снимаем оба триггера, чтобы второй не сработал впустую.
    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        if (InitialLevelLoadedHandler.IsValid())
        {
            EventBus->UnregisterHandler(InitialLevelLoadedHandler);
            InitialLevelLoadedHandler.Invalidate();
        }
    }
    if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
    {
        World->GetTimerManager().ClearTimer(InitialReevaluateFallbackTimer);
    }

    UE_LOG(LogScenario, Log,
        TEXT("ScenarioManager: running initial ReevaluateSteps (triggered by %s)"), Reason);

    ReevaluateSteps();
}


void UScenarioManagerSubsystem::OnInitialLevelLoaded(const FOutcomeEventBase& Outcome)
{
    TryRunInitialReevaluate(TEXT("Interior::LevelLoaded"));
}

// =============================================================================
// Loading
// =============================================================================

void UScenarioManagerSubsystem::LoadAllSteps()
{
    Steps.Empty();

    UAssetManager* AM = UAssetManager::GetIfInitialized();
    if (!AM)
    {
        UE_LOG(LogScenario, Warning, TEXT("ScenarioManager: AssetManager not initialized"));
        return;
    }

    TArray<FPrimaryAssetId> AssetIds;
    AM->GetPrimaryAssetIdList(FPrimaryAssetType(TEXT("ScenarioStep")), AssetIds);

    for (const FPrimaryAssetId& AssetId : AssetIds)
    {
        const FSoftObjectPath Path = AM->GetPrimaryAssetPath(AssetId);
        UPayloadBatchAsset* Step = Cast<UPayloadBatchAsset>(Path.TryLoad());
        if (Step)
        {
            Steps.Add(AssetId, Step);
            UE_LOG(LogScenario, Log, TEXT("ScenarioManager: loaded step '%s'"), *AssetId.ToString());
        }
        else
        {
            UE_LOG(LogScenario, Warning,
                TEXT("ScenarioManager: failed to load step '%s'"), *AssetId.ToString());
        }
    }
}

void UScenarioManagerSubsystem::SubscribeToEventBus()
{
    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    // Пустое Composite-условие матчит ЛЮБОЕ событие. Внутри handler'а
    // проверяем наличие тэгов и, если их нет, ничего не делаем.
    TagChangeCondition = NewObject<UOutcomeConditionAsset>(this);
    TagChangeCondition->OperatorType = EConditionOperator::Composite;
    TagChangeCondition->CompileCondition();

    TagChangeHandler = EventBus->RegisterHandler(
        TagChangeCondition,
        FOutcomeHandlerDelegate::CreateUObject(
            this, &UScenarioManagerSubsystem::HandleOutcomeWithTags));
}

// =============================================================================
// Public API
// =============================================================================

void UScenarioManagerSubsystem::ChangeTags(const TArray<FGameplayTag>& TagsToAdd,
                                           const TArray<FGameplayTag>& TagsToRemove)
{
    if (TagsToAdd.Num() == 0 && TagsToRemove.Num() == 0)
        return;

    // 1) Мутируем хранилище немедленно.
    for (const FGameplayTag& Tag : TagsToAdd)
    {
        if (Tag.IsValid())
            TagStorage.Add(Tag);
    }
    for (const FGameplayTag& Tag : TagsToRemove)
    {
        if (Tag.IsValid())
            TagStorage.Remove(Tag);
    }

    if (TagsToAdd.Num() > 0 || TagsToRemove.Num() > 0)
    {
        UE_LOG(LogScenario, Log,
            TEXT("ScenarioManager: ChangeTags(+%d, -%d). Storage size now %d"),
            TagsToAdd.Num(), TagsToRemove.Num(), TagStorage.Num());
    }

    // 2) Если мы уже в цикле переоценки — просто помечаем, что нужен ещё проход.
    //    Это защищает от бесконечной рекурсии при каскадах.
    if (bIsReevaluating)
    {
        bNeedsReevaluate = true;
        return;
    }

    // 3) Иначе запускаем цикл переоценки до стабилизации.
    bIsReevaluating = true;
    int32 Iteration = 0;
    do
    {
        bNeedsReevaluate = false;
        ReevaluateSteps();

        if (++Iteration >= MaxReevaluateIterations)
        {
            UE_LOG(LogScenario, Warning,
                TEXT("ScenarioManager: Reevaluate loop reached max iterations (%d), aborting to avoid infinite cascade."),
                MaxReevaluateIterations);
            break;
        }
    } while (bNeedsReevaluate);

    bIsReevaluating = false;
}

TArray<FGameplayTag> UScenarioManagerSubsystem::GetAllTags() const
{
    return TagStorage.Array();
}

TArray<FName> UScenarioManagerSubsystem::GetActiveStepNames() const
{
    TArray<FName> Result;
    for (const FPrimaryAssetId& Id : ActiveStepIds)
        Result.Add(Id.PrimaryAssetName);
    return Result;
}

FString UScenarioManagerSubsystem::GetDebugStateString() const
{
    FString TagsStr;
    for (const FGameplayTag& T : TagStorage)
        TagsStr += T.ToString() + TEXT(" ");
    FString StepsStr;
    for (const FPrimaryAssetId& Id : ActiveStepIds)
        StepsStr += Id.PrimaryAssetName.ToString() + TEXT(" ");

    return FString::Printf(TEXT("Tags [%s] | Active steps [%s]"), *TagsStr, *StepsStr);
}

bool UScenarioManagerSubsystem::IsStepActive(const UPayloadBatchAsset* Step) const
{
    if (!Step) return false;
    return ActiveStepIds.Contains(Step->GetPrimaryAssetId());
}
// =============================================================================
// Reevaluation
// =============================================================================

void UScenarioManagerSubsystem::ReevaluateSteps()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // 1) Собираем переходы (используя текущий TagStorage).
    TArray<UPayloadBatchAsset*> ToActivate;
    TArray<UPayloadBatchAsset*> ToDeactivate;

    for (const TPair<FPrimaryAssetId, TObjectPtr<UPayloadBatchAsset>>& Pair : Steps)
    {
        UPayloadBatchAsset* Step = Pair.Value;
        if (!Step) continue;

        const bool bShouldBeActive = Step->bEnabled && Step->IsConditionMet(TagStorage);
        const bool bIsActive = ActiveStepIds.Contains(Pair.Key);

        if (bShouldBeActive && !bIsActive)  ToActivate.Add(Step);
        if (!bShouldBeActive && bIsActive)  ToDeactivate.Add(Step);
    }

    if (ToActivate.Num() == 0 && ToDeactivate.Num() == 0)
        return;

    // 2) Меняем состояние ДО выполнения команд, чтобы реентерабельный
    //    ChangeTags видел консистентное ActiveStepIds.
    for (UPayloadBatchAsset* Step : ToDeactivate)
        ActiveStepIds.Remove(Step->GetPrimaryAssetId());
    for (UPayloadBatchAsset* Step : ToActivate)
        ActiveStepIds.Add(Step->GetPrimaryAssetId());

    // 3) Выполняем команды.
    for (UPayloadBatchAsset* Step : ToDeactivate)
    {
        UE_LOG(LogScenario, Log, TEXT("ScenarioManager: DEACTIVATE step '%s'"),
            *Step->GetName());
        ExecuteStepDeactivation(Step);
    }
    for (UPayloadBatchAsset* Step : ToActivate)
    {
        UE_LOG(LogScenario, Log, TEXT("ScenarioManager: ACTIVATE step '%s'"),
            *Step->GetName());
        ExecuteStepActivation(Step);
    }
}

void UScenarioManagerSubsystem::ExecuteStepActivation(UPayloadBatchAsset* Step)
{
    if (!Step) return;
    Step->ExecuteEntriesInternal(this, Step->Entries);
}

void UScenarioManagerSubsystem::ExecuteStepDeactivation(UPayloadBatchAsset* Step)
{
    if (!Step) return;
    Step->ExecuteEntriesInternal(this, Step->DeactivationEntries);
}

// =============================================================================
// EventBus handler
// =============================================================================

void UScenarioManagerSubsystem::HandleOutcomeWithTags(const FOutcomeEventBase& Outcome)
{
    if (Outcome.TagsToAdd.Num() == 0 && Outcome.TagsToRemove.Num() == 0)
        return;

    ChangeTags(Outcome.TagsToAdd, Outcome.TagsToRemove);
}

// =============================================================================
// Save / Load
// =============================================================================

void UScenarioManagerSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
    OutData.SubsystemName = GetSaveSubsystemName();

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

    // Tags
    TArray<TSharedPtr<FJsonValue>> TagsArray;
    for (const FGameplayTag& Tag : TagStorage)
        TagsArray.Add(MakeShared<FJsonValueString>(Tag.ToString()));
    Root->SetArrayField(TEXT("Tags"), TagsArray);

    // Active steps
    TArray<TSharedPtr<FJsonValue>> StepsArray;
    for (const FPrimaryAssetId& Id : ActiveStepIds)
        StepsArray.Add(MakeShared<FJsonValueString>(Id.ToString()));
    Root->SetArrayField(TEXT("ActiveSteps"), StepsArray);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    OutData.SerializedData = Output;
}

void UScenarioManagerSubsystem::ApplySaveData(const FSubsystemSaveData& InData)
{
    bLoadComplete = false;

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

    TagStorage.Empty();
    ActiveStepIds.Empty();

    // ---- Tags ----
    const TArray<TSharedPtr<FJsonValue>>* TagsArray = nullptr;
    if (Root->TryGetArrayField(TEXT("Tags"), TagsArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *TagsArray)
        {
            const FString TagStr = Val->AsString();
            const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(
                FName(*TagStr), /*ErrorIfNotFound=*/false);
            if (Tag.IsValid())
            {
                TagStorage.Add(Tag);
            }
            else
            {
                UE_LOG(LogScenario, Warning,
                    TEXT("ScenarioManager: unknown tag in save '%s' — skipped"), *TagStr);
            }
        }
    }

    // ---- Active steps ----
    const TArray<TSharedPtr<FJsonValue>>* StepsArray = nullptr;
    if (Root->TryGetArrayField(TEXT("ActiveSteps"), StepsArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *StepsArray)
        {
            const FString IdStr = Val->AsString();
            // UE 5.4: FromString возвращает значение, без out-параметра.
            const FPrimaryAssetId Id = FPrimaryAssetId::FromString(IdStr);
            if (Id.IsValid())
            {
                ActiveStepIds.Add(Id);
            }
            else
            {
                UE_LOG(LogScenario, Warning,
                    TEXT("ScenarioManager: invalid PrimaryAssetId in save '%s' — skipped"), *IdStr);
            }
        }
    }

    // ВАЖНО: ReevaluateSteps здесь НЕ вызываем.
    // Состояние объектов мира восстанавливают другие подсистемы,
    // а наши команды активации/деактивации при загрузке не должны выполняться повторно.
    // Активные шаги уже зафиксированы в ActiveStepIds — следующая ручная
    // переоценка (по ChangeTags) будет отталкиваться от этого состояния.

    UE_LOG(LogScenario, Log,
        TEXT("ScenarioManager: ApplySaveData complete. Tags=%d, ActiveSteps=%d"),
        TagStorage.Num(), ActiveStepIds.Num());

    bLoadComplete = true;
}
