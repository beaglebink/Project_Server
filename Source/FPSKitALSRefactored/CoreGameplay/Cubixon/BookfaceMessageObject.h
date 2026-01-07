#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BookfaceMessageObject.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UBookfaceMessageObject : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString MessageId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString FromUserId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString ToUserId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText MessageContent;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FDateTime Timestamp;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FString> LikesUserIds;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FString> SubscribedUserIDs;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FString> UnSubscribedUserIDs;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Transient)
    UBookfaceMessageObject* ParentMessage;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Transient)
    TArray<UBookfaceMessageObject*> ReplyMessages;
};