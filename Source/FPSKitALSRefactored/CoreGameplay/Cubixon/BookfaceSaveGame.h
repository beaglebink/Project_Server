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
    // 🔹 Сохраняемые профили (включая SavedMessages внутри каждого профиля)
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SaveData")
    TArray<FBookfaceProfileStructure> SavedProfiles;

    // 🔹 Сохраняемые заявки в друзья
    UPROPERTY(VisibleAnywhere, Category = "SaveData")
    TArray<FBookfaceFriendRequestStructure> SavedFriendRequests;

    // 🔹 Сохраняемые уведомления
    UPROPERTY(VisibleAnywhere, Category = "SaveData")
    TArray<FBookfaceMessageNoticeStructure> SavedMessageNotices;
};