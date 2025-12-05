#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BookfaceMessageObject.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UBookfaceMessageObject : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    FString MessageId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Message")
    FString FromUserId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Message")
    FString ToUserId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Message")
    UBookfaceMessageObject* ParentMessage;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Message")
    TArray<UBookfaceMessageObject*> ReplyMessages;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Message")
    FText MessageContent;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Message")
    FDateTime Timestamp;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Message")
    TArray<FString> LikesUserIds;

    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Message")
    TArray<FString> SubscribedProfiles;
};