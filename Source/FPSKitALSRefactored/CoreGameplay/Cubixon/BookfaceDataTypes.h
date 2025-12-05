#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "BookfaceDataTypes.generated.h"

class UBookfaceMessageObject;

UENUM(BlueprintType)
enum class EPrivacyVisibility : uint8
{
    VisibleToMe       UMETA(DisplayName = "Visible to me"),
    VisibleToFriends  UMETA(DisplayName = "Visible to friends"),
    VisibleToEveryone UMETA(DisplayName = "Visible to everyone")
};

USTRUCT(BlueprintType)
struct FBookfaceMessageNoticeStructure
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString ProfileID;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UBookfaceMessageObject* RootMessage;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UBookfaceMessageObject* ParentMessage;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    UBookfaceMessageObject* Message;

    bool operator==(const FBookfaceMessageNoticeStructure& Other) const
    {
        return ProfileID == Other.ProfileID && RootMessage == Other.RootMessage && ParentMessage == Other.ParentMessage && Message == Other.Message;
    }
};

USTRUCT(BlueprintType)
struct FBookfaceFriendRequestStructure
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString FromUserId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString ToUserId;

    bool operator==(const FBookfaceFriendRequestStructure& Other) const
    {
        return FromUserId == Other.FromUserId && ToUserId == Other.ToUserId;
    }
};

USTRUCT(BlueprintType)
struct FAboutInfoStructure
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText InfoCaption;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText InfoDescription;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EPrivacyVisibility PrivacyVisibility;
};

USTRUCT(BlueprintType)
struct FBookfaceProfileStructure
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString UserId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText DisplayName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSoftObjectPtr<UTexture2D> AvatarImage;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSoftObjectPtr<UTexture2D> CoverImage;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText Bio;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FAboutInfoStructure> AboutInfo;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FString> FriendsList;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 ReputationScore = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EPrivacyVisibility BIO_Privacy;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool IsOnline = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<UBookfaceMessageObject*> UserMessages;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FBookfaceMessageNoticeStructure> MessageNotices;
};

