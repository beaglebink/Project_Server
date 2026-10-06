#include "LocationConditionAsset.h"
#include "../LocationTrackerSubsystem/LocationTrackerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

ULocationTrackerSubsystem* ULocationConditionAsset::FindTracker()
{
    if (!GEngine) return nullptr;
    for (const FWorldContext& C : GEngine->GetWorldContexts())
    {
        UWorld* W = C.World();
        if (!W || !W->IsGameWorld()) continue;
        UGameInstance* GI = W->GetGameInstance();
        if (!GI) continue;
        if (ULocationTrackerSubsystem* T = GI->GetSubsystem<ULocationTrackerSubsystem>())
            return T;
    }
    return nullptr;
}

void ULocationConditionAsset::CompileCondition()
{
    // Резолвим DisplayName в ключ один раз.
    CachedKey = FLocationVisitKey();
    if (!TargetDisplayName.IsEmpty())
        ULocationTrackerSubsystem::ResolveLocationKeyByDisplayName(TargetDisplayName, CachedKey);

    class FLocationCondition : public IOutcomeCondition
    {
    public:
        FLocationCondition(const ULocationConditionAsset* InAsset) : Asset(InAsset) {}
        virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
        {
            return Asset ? Asset->EvaluateCondition(Outcome) : false;
        }
        virtual FString Describe() const override
        {
            if (!Asset) return TEXT("Location: Invalid");

            const FString QueryStr = StaticEnum<ELocationQueryType>()
                ->GetValueAsString(Asset->QueryType);
            const FString Name = Asset->TargetDisplayName.ToString();

            if (!Asset->CachedKey.IsValid())
                return FString::Printf(TEXT("Location: [%s] '%s' [UNRESOLVED]"), *QueryStr, *Name);

            const FString KeyStr = FString::Printf(TEXT("%s:%s"),
                *StaticEnum<ELocationLevel>()->GetValueAsString(Asset->CachedKey.Level),
                *Asset->CachedKey.LocationId.ToString());

            switch (Asset->QueryType)
            {
            case ELocationQueryType::HasEnteredEver:
            case ELocationQueryType::LeftAndReturned:
                return FString::Printf(TEXT("Location: [%s] '%s' -> {%s}"),
                    *QueryStr, *Name, *KeyStr);

            case ELocationQueryType::VisitCountAtLeast:
                return FString::Printf(TEXT("Location: [%s] '%s' -> {%s}, count %s %d"),
                    *QueryStr, *Name, *KeyStr,
                    *StaticEnum<ECheckCompareOp>()->GetValueAsString(Asset->CompareOp),
                    Asset->VisitThreshold);

            default:
                return FString::Printf(TEXT("Location: [%s] '%s'"), *QueryStr, *Name);
            }
        }
    private:
        const ULocationConditionAsset* Asset;
    };

    CompiledCondition = MakeShared<FLocationCondition>(this);
    ConditionDescription = CompiledCondition->Describe();
}

bool ULocationConditionAsset::EvaluateCondition(const FOutcomeEventBase& /*Outcome*/) const
{
    if (!CachedKey.IsValid()) return false;

    ULocationTrackerSubsystem* Tracker = FindTracker();
    if (!Tracker) return false;

    switch (QueryType)
    {
    case ELocationQueryType::HasEnteredEver:
        return Tracker->HasEnteredEver(CachedKey);

    case ELocationQueryType::LeftAndReturned:
        return Tracker->HasLeftAndReturned(CachedKey);

    case ELocationQueryType::VisitCountAtLeast:
    {
        const int32 Count = Tracker->GetEnterCount(CachedKey);
        switch (CompareOp)
        {
        case ECheckCompareOp::Equal:          return Count == VisitThreshold;
        case ECheckCompareOp::NotEqual:       return Count != VisitThreshold;
        case ECheckCompareOp::Less:           return Count < VisitThreshold;
        case ECheckCompareOp::LessOrEqual:    return Count <= VisitThreshold;
        case ECheckCompareOp::Greater:        return Count > VisitThreshold;
        case ECheckCompareOp::GreaterOrEqual: return Count >= VisitThreshold;
        default:                              return false;
        }
    }

    default: return false;
    }
}