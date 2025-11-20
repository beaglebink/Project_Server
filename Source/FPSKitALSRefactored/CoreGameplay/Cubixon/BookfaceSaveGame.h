#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BookfaceSubsystem.h"
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
};