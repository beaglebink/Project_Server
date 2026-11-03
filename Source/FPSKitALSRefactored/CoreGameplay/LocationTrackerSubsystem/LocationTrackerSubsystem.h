#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ISaveableSubsystem.h"

#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "LocationTypes.h"
#include "../EventBusSystem/EventBusSubsystem.h"

#include "LocationTrackerSubsystem.generated.h"

class UWorldMapAsset;
class UWorldRegionAsset;
class UStreetAsset;
class UInteriorSetAsset;
class UFloorAsset;

USTRUCT(BlueprintType)
struct FLocationVisitKey
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) ELocationLevel Level = ELocationLevel::Default;
    UPROPERTY(BlueprintReadOnly) FGuid LocationId;

    bool IsValid() const { return Level != ELocationLevel::Default && LocationId.IsValid(); }
    bool operator==(const FLocationVisitKey& Other) const
    {
        return Level == Other.Level && LocationId == Other.LocationId;
    }
};

USTRUCT(BlueprintType)
struct FLocationVisitState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) ELocationLevel Level = ELocationLevel::Default;
    UPROPERTY(BlueprintReadOnly) FGuid LocationId;
    UPROPERTY(BlueprintReadOnly) FText DisplayName;

    UPROPERTY(BlueprintReadOnly) FDateTime FirstEnteredAt;
    UPROPERTY(BlueprintReadOnly) FDateTime LastEnteredAt;
    UPROPERTY(BlueprintReadOnly) FDateTime LastLeftAt;
    UPROPERTY(BlueprintReadOnly) int32 EnterCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 LeaveCount = 0;
};

USTRUCT(BlueprintType)
struct FLocationVisitAddress
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FGuid MapId;
    UPROPERTY(BlueprintReadOnly) FGuid RegionId;
    UPROPERTY(BlueprintReadOnly) FGuid StreetId;
    UPROPERTY(BlueprintReadOnly) FGuid BuildingId;
    UPROPERTY(BlueprintReadOnly) FGuid FloorId;

    void Reset()
    {
        MapId.Invalidate();
        RegionId.Invalidate();
        StreetId.Invalidate();
        BuildingId.Invalidate();
        FloorId.Invalidate();
    }

    FGuid Get(ELocationLevel L) const
    {
        switch (L)
        {
        case ELocationLevel::Map:      return MapId;
        case ELocationLevel::Region:   return RegionId;
        case ELocationLevel::Street:   return StreetId;
        case ELocationLevel::Building: return BuildingId;
        case ELocationLevel::Floor:    return FloorId;
        default:                       return FGuid();
        }
    }

    void Set(ELocationLevel L, const FGuid& Id)
    {
        switch (L)
        {
        case ELocationLevel::Map:      MapId = Id; break;
        case ELocationLevel::Region:   RegionId = Id; break;
        case ELocationLevel::Street:   StreetId = Id; break;
        case ELocationLevel::Building: BuildingId = Id; break;
        case ELocationLevel::Floor:    FloorId = Id; break;
        }
    }

    bool operator==(const FLocationVisitAddress& O) const
    {
        return MapId == O.MapId && RegionId == O.RegionId
            && StreetId == O.StreetId && BuildingId == O.BuildingId && FloorId == O.FloorId;
    }
};

UCLASS()
class FPSKITALSREFACTORED_API ULocationTrackerSubsystem
    : public UGameInstanceSubsystem
    , public ISaveableSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ---- ISaveableSubsystem ----
    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("LocationTracker"); }
    virtual bool GetIsLoadComplete() const override { return bLoadComplete; }

    // Нормализация имени пакета сцены (убирает PIE-префикс, lowercase).
    static FString NormalizeLevelName(const FString& InPath);

    // Запросы по адресу локации.
    bool HasEnteredEver(const FLocationVisitKey& Key) const;
    bool HasLeftAndReturned(const FLocationVisitKey& Key) const;
    int32 GetEnterCount(const FLocationVisitKey& Key) const;
    int32 GetLeaveCount(const FLocationVisitKey& Key) const;
    const FLocationVisitState* FindVisitState(const FLocationVisitKey& Key) const;

    // Текущий адрес — для отладки и для других подсистем.
    const FLocationVisitAddress& GetCurrentAddress() const { return CurrentAddress; }

    // ── Текущее местоположение ────────────────────────────────────────────
    //
    // CurrentAddress хранит GUID'ы всех уровней, которые сейчас «заполнены»
    // в адресе. GetCurrentDeepestLevel возвращает самый глубокий валидный
    // уровень — тот, в котором игрок реально находится:
    //   • Floor    — игрок на этаже (в доме);
    //   • Building — игрок в доме, но этаж неизвестен;
    //   • Street   — игрок на открытой улице;
    //   • Region   — игрок на региональной сцене (без конкретной улицы);
    //   • Map      — игрок на сцене карты;
    //   • Default  — игрок ещё нигде (загрузка/телепорт до LevelLoaded).
    //
    // GetCurrentLocationAsset возвращает ассет соответствующего уровня
    // (UFloorAsset / UInteriorSetAsset / UStreetAsset / UWorldRegionAsset /
    // UWorldMapAsset), либо nullptr.

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Query")
    UObject* GetLocationAsset(ELocationLevel Level, const FGuid& Id) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Query")
    UObject* GetCurrentLocationAsset(ELocationLevel Level) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Query")
    FText GetCurrentLocationDisplayName(ELocationLevel Level) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Query")
    ELocationLevel GetCurrentDeepestLevel() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Query")
    UObject* GetCurrentLocationAssetDeepest() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Query")
    FText GetCurrentLocationDisplayNameDeepest() const;

    // Строка вида "Deep town / Port region / Fish street / Fish street 2 / FishStreet 2-1"
    // (только заполненные уровни). Удобно для отладки и логов.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Debug")
    FString GetCurrentLocationDebugString() const;

    // Преобразовать DisplayName ассета локации в ключ.
    // Ищет среди FloorAsset / InteriorSetAsset / StreetAsset / WorldRegionAsset / WorldMapAsset.
    //
    // PreferredLevel == Default  — прежнее поведение: обход всех типов,
    //                              приоритет от Floor к Map.
    // PreferredLevel != Default  — искать только среди ассетов указанного уровня.
    //                              Полезно, когда DisplayName уникален не глобально,
    //                              а только внутри своего уровня.
    //
    // Если найдено ровно одно совпадение — возвращает true и заполняет OutKey.
    // Если найдено несколько — логирует Warning и берёт первое.
    static bool ResolveLocationKeyByDisplayName(
        const FText& DisplayName,
        FLocationVisitKey& OutKey,
        ELocationLevel PreferredLevel = ELocationLevel::Default);

    // Возвращает количество заходов и уходов для локации, заданной её
    // DisplayName (совпадает с DisplayName ассета: Map / Region / Street /
    // Building / Floor).
    //
    // Возврат:
    //   true  — локация найдена, OutEnterCount/OutLeaveCount заполнены.
    //   false — либо DisplayName пуст, либо локация не найдена.
    //           В этом случае счётчики = 0.
    UFUNCTION(BlueprintCallable, Category = "LocationTracker|Query", meta = (AutoCreateRefTerm = "DisplayName"))
    bool GetLocationCountsByDisplayName(
        const FText& DisplayName,
        int32& OutEnterCount,
        int32& OutLeaveCount) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Query")
    ULocationVisitEventPayload* GetCurrentLocationPayload(ELocationLevel Level) const;

private:
    void HandleLocationEvent(const FOutcomeEventBase& Outcome);
    void HandleVisitReset(const FOutcomeEventBase& Outcome);
    void HandleStreetTransition(const FOutcomeEventBase& Outcome);

    void BuildSceneIndex();
    bool FindAddressForScene(const FString& NormalizedSceneName, FLocationVisitAddress& OutAddress) const;

    void ApplyAddressTransition(const FLocationVisitAddress& NewAddress);

    // Публикует LocationEntered / LocationLeft для указанной локации.
    // Вызывается из ApplyAddressTransition сразу после изменения счётчиков.
    void PublishLocationVisitEvent(ELocationLevel Level, const FGuid& Id, bool bEntered);

    // Обратный индекс: ключ локации → отображаемое имя.
    // Заполняется в BuildSceneIndex из ассетов.
    UPROPERTY() TMap<FString, FText> LocationDisplayNames;

    // Ключ локации → ассет. Заполняется в BuildSceneIndex рядом с
    // LocationDisplayNames. Нужен для событий LocationEntered / LocationLeft
    // и для GetCurrentLocationAsset — payload отдаёт именно ассет, а не GUID.
    UPROPERTY() TMap<FString, TObjectPtr<UObject>> LocationAssets;

    // Получить отображаемое имя локации по её уровню и Guid.
    // Если имени нет — возвращает короткий технический фоллбэк "<Level:XXXXXXXX>".
    FString GetLocationLabel(ELocationLevel Level, const FGuid& Id) const;

    FLocationVisitState& FindOrAddState(ELocationLevel Level, const FGuid& Id);
    FDateTime GetGameTimeNow() const;

    static FString MakeKey(ELocationLevel Level, const FGuid& Id);

    // Scene package name (normalized) → базовый адрес.
    UPROPERTY() TMap<FString, FLocationVisitAddress> SceneToAddress;

    // Ключ → состояние посещений.
    UPROPERTY() TMap<FString, FLocationVisitState> VisitHistory;

    // Текущий адрес игрока.
    UPROPERTY() FLocationVisitAddress CurrentAddress;

    UPROPERTY() TObjectPtr<UOutcomeConditionAsset> LocationEventCondition;
    FOutcomeHandlerHandle LocationEventHandler;

    UPROPERTY() TObjectPtr<UOutcomeConditionAsset> LocationResetCondition;
    FOutcomeHandlerHandle LocationResetHandler;

    // ---- StreetTransition ----
    // Подписка на команды «игрок перешёл на улицу» от триггеров на уровнях.
    UPROPERTY() TObjectPtr<UOutcomeConditionAsset> StreetTransitionCondition;
    FOutcomeHandlerHandle StreetTransitionHandler;

    bool bLoadComplete = true;
};