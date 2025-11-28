#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BookfaceDataTypes.h"
#include "BookfaceSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FBookfaceMessageData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString MessageId;          // уникальный ID сообщения

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
    FString ParentMessageId;    // ID родительского сообщения
};

UCLASS()
class FPSKITALSREFACTORED_API UBookfaceSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, Category = "SaveData")
    TArray<FBookfaceProfileStructure> SavedProfiles;

    UPROPERTY(VisibleAnywhere, Category = "SaveData")
    TArray<FBookfaceFriendRequestStructure> SavedFriendRequests;

    UPROPERTY(VisibleAnywhere, Category = "SaveData")
    TArray<FBookfaceMessageData> SavedAllPosts;
};