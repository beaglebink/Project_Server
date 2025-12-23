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

UENUM(BlueprintType)
enum class ENotifyType : uint8
{
    MessageActivityNotify   UMETA(DisplayName = "Message activity"),
    MessageLikeNotify       UMETA(DisplayName = "Message like"),
};

// 🔹 Сериализуемая структура сообщения
USTRUCT(BlueprintType)
struct FBookfaceMessageData
{
    GENERATED_BODY()

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
    FString ParentMessageId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FString> SubscribedUserIDs;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FString> UnSubscribedUserIDs;
};

// 🔹 Уведомления
USTRUCT(BlueprintType)
struct FBookfaceMessageNoticeStructure
{
    GENERATED_BODY()

    // 🔹 Сериализуемые поля
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ENotifyType MessageType;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString LikeUserId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString ProfileID;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString RootMessageId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString ParentMessageId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString MessageId;

    // 🔹 Рабочие ссылки (не сериализуются)
    UPROPERTY(BlueprintReadOnly, Transient)
    UBookfaceMessageObject* RootMessage = nullptr;

    UPROPERTY(BlueprintReadOnly, Transient)
    UBookfaceMessageObject* ParentMessage = nullptr;

    UPROPERTY(BlueprintReadOnly, Transient)
    UBookfaceMessageObject* Message = nullptr;

    bool operator==(const FBookfaceMessageNoticeStructure& Other) const
    {
        return ProfileID == Other.ProfileID &&
            MessageId == Other.MessageId &&
            RootMessageId == Other.RootMessageId &&
            ParentMessageId == Other.ParentMessageId &&
            MessageType == Other.MessageType &&
            LikeUserId == Other.LikeUserId;
    }

};
// 🔹 Заявка в друзья
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

// 🔹 Доп. инфо
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

// 🔹 Профиль
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

    UPROPERTY(Transient)
    bool IsOnline = false;

    // 🔹 Сериализуемые сообщения
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FBookfaceMessageData> SavedMessages;

    // 🔹 Рабочие объекты сообщений (не сериализуются)
    UPROPERTY(BlueprintReadWrite, Transient)
    TArray<UBookfaceMessageObject*> UserMessages;

    // 🔹 Уведомления (сохраняются по ID)
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FBookfaceMessageNoticeStructure> MessageNotices;
};

USTRUCT(BlueprintType)
struct FBF_MessageStructure
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString FromContact;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString ToContact;

    //UPROPERTY(BlueprintReadWrite, EditAnywhere)
    //EBF_DirectType Direct = EBF_DirectType::Incoming;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText Message;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bIsRead = false;

};