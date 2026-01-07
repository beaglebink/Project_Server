#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BookfaceDataTypes.h"
#include "BookfaceSaveGame.generated.h"

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
    TArray<FBF_MessageStructure> SavedMessages;

    UPROPERTY(VisibleAnywhere, Category = "SaveData")
    TArray<FBookfaceMessageNoticeStructure> SavedMessageNotices;
};