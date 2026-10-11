#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PayloadBatchRegistry.h"
#include "PayloadBatchEntry.generated.h"

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class FPSKITALSREFACTORED_API UPayloadBatchEntry : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "1. Subsystem")
    EBatchSubsystem Subsystem = EBatchSubsystem::Mission;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "2. Command",
        meta = (GetOptions = "GetCommandOptions"))
    FName Command;

    UPROPERTY(VisibleAnywhere, Instanced, BlueprintReadWrite, Category = "3. Data")
    TObjectPtr<UOutcomePayload> Payload;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. Control")
    bool bEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. Control",
        meta = (ClampMin = "0.0", UIMin = "0.0"))
    float DelaySeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "4. Control")
    FString Comment;

    UFUNCTION()
    TArray<FName> GetCommandOptions() const;

    const FCommandSpec* GetSpec() const;
    bool                IsValidEntry() const;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;

private:
    void SyncCommandForSubsystem();
    void RebuildPayload();
#endif
};