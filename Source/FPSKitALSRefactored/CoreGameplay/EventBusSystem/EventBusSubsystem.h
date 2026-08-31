#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomePayload.h"
#include "OutcomeQuery.h"
#include "EventBusSubsystem.generated.h"

class UOutcomeConditionAsset;

// C++ handler delegate
using FOutcomeHandlerDelegate = TDelegate<void(const FOutcomeEventBase&)>;

// Blueprint dynamic delegate
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnOutcomeEvent, const FOutcomeEventBase&, Outcome);

// Handle returned by RegisterHandler
USTRUCT(BlueprintType)
struct FOutcomeHandlerHandle
{
    GENERATED_BODY()

    static const uint32 Invalid = 0;

    FOutcomeHandlerHandle() : Id(Invalid) {}
    explicit FOutcomeHandlerHandle(uint32 InId) : Id(InId) {}

    bool IsValid() const { return Id != Invalid; }
    void Invalidate() { Id = Invalid; }
    uint32 GetId() const { return Id; }

    bool operator==(const FOutcomeHandlerHandle& Other) const { return Id == Other.Id; }
    bool operator!=(const FOutcomeHandlerHandle& Other) const { return Id != Other.Id; }

private:
    UPROPERTY()
    uint32 Id;
};

// Internal handler entry
struct FOutcomeHandlerEntry
{
    uint32 HandleId;
    FOutcomeHandlerDelegate Handler;       // C++ делегат
    FOnOutcomeEvent BlueprintDelegate;     // Blueprint делегат
    TSharedPtr<IOutcomeCondition> Query;

    // Конструктор для C++ обработчика
    FOutcomeHandlerEntry(uint32 InHandleId, FOutcomeHandlerDelegate InHandler, TSharedPtr<IOutcomeCondition> InQuery)
        : HandleId(InHandleId), Handler(MoveTemp(InHandler)), Query(InQuery)
    {
    }

    // Конструктор для Blueprint обработчика
    FOutcomeHandlerEntry(uint32 InHandleId, FOnOutcomeEvent InBlueprintDelegate, TSharedPtr<IOutcomeCondition> InQuery)
        : HandleId(InHandleId), BlueprintDelegate(InBlueprintDelegate), Query(InQuery)
    {
    }
};

UCLASS()
class FPSKITALSREFACTORED_API UEventBusSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Публикация события
    UFUNCTION(BlueprintCallable, Category = "EventBus")
    void PublishOutcome(const FOutcomeEventBase& Outcome);

    // Создание Payload
    UFUNCTION(BlueprintCallable, Category = "EventBus", meta = (DeterminesOutputType = "PayloadClass"))
    UOutcomePayload* CreatePayload(TSubclassOf<UOutcomePayload> PayloadClass);

    template<typename T>
    T* CreatePayload()
    {
        static_assert(TIsDerivedFrom<T, UOutcomePayload>::IsDerived,
            "T must derive from UOutcomePayload");
        return NewObject<T>(GetGameInstance());
    }

    // Регистрация C++ обработчика
    FOutcomeHandlerHandle RegisterHandler(
        UOutcomeConditionAsset* ConditionAsset,
        FOutcomeHandlerDelegate Handler);

    // Регистрация Blueprint обработчика
    UFUNCTION(BlueprintCallable, Category = "EventBus")
    FOutcomeHandlerHandle RegisterBlueprintHandler(
        UOutcomeConditionAsset* ConditionAsset,
        FOnOutcomeEvent Delegate);

    // Отписка от событий (работает для обоих типов)
    UFUNCTION(BlueprintCallable, Category = "EventBus")
    void UnregisterHandler(UPARAM(ref) FOutcomeHandlerHandle& Handle);

    UFUNCTION(BlueprintCallable, Category = "EventBus")
    int32 GetHandlerCount() const { return Handlers.Num(); }

    virtual void BeginDestroy() override;

private:
    TArray<FOutcomeHandlerEntry> Handlers;
    uint32 NextHandleId = 1;

    bool bDispatching = false;

    struct FPendingOperation
    {
        enum class EType : uint8 { Register, Unregister } Type = EType::Unregister;
        uint32 HandleId = 0;
        FOutcomeHandlerEntry Entry{ 0, FOutcomeHandlerDelegate{}, nullptr };
    };
    TArray<FPendingOperation> PendingOperations;
};