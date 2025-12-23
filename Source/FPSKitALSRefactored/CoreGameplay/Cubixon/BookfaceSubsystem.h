#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/Texture2D.h"
#include "BookfaceMessageObject.h"
#include "BookfaceDataTypes.h"
#include "BookfaceSaveGame.h"
#include "BookfaceSubsystem.generated.h"
/*
UENUM(BlueprintType)
enum class EBF_DirectType : uint8
{
    Incoming UMETA(DisplayName = "Incoming"),
    Outgoing UMETA(DisplayName = "Outgoing")
};
*/
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnlineStatusChange, const FString&, UserId, bool, bIsOnline);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRemoveFriend, const FString&, UserId, const FString&, UserFriend);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAddFriendRequest, const FString&, FromUserId, const FString&, ToUserId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRemoveFriendRequest, const FString&, FromUserId, const FString&, ToUserId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAcceptFriendRequest, const FString&, FromUserId, const FString&, ToUserId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBookfaceMessageAdded, UBookfaceMessageObject*, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBookfaceMessageNotice, FBookfaceMessageNoticeStructure, MessageNotice);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBFMessageAdded, const FBF_MessageStructure&, Message);

UCLASS()
class FPSKITALSREFACTORED_API UBookfaceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable)
    void OnBookfaceAppOpened();

    UFUNCTION(BlueprintCallable)
    void SaveBookfaceDataAsync();

    UFUNCTION(BlueprintCallable)
    void SaveBookfaceMessagesAsync();

    UFUNCTION(BlueprintCallable)
    void LoadBookfaceDataAsync();

    UFUNCTION(BlueprintCallable)
    bool HasLoadedSave() const { return bHasLoadedSave; }

    // Работа с профилями
    UFUNCTION(BlueprintCallable)
    void AddUserProfile(const FBookfaceProfileStructure& NewProfile);

    UFUNCTION(BlueprintCallable)
    const TArray<FBookfaceProfileStructure>& GetUserProfiles() const;

    UFUNCTION(BlueprintCallable)
    FBookfaceProfileStructure GetUserProfileById(const FString& UserId) const;

    UFUNCTION(BlueprintCallable)
    void UpdateUserProfile(const FBookfaceProfileStructure& UpdatedProfile);

    UFUNCTION(BlueprintCallable)
    bool SetOnlineStatus(const FString& UserId, const bool bOnline);

    UFUNCTION(BlueprintCallable)
    bool GetOnlineStatus(const FString& UserId, bool& IsOnline) const;

    UFUNCTION(BlueprintCallable)
    bool RemoveFriend(const FString& UserId, const FString& FriendId);

    UFUNCTION(BlueprintCallable)
    TArray<FBookfaceProfileStructure> SearchProfiles(const FString& InUserID, const FString& Query) const;

    UFUNCTION(BlueprintCallable)
    void AddFriendRequest(const FString& FromUserId, const FString& ToUserId);

    UFUNCTION(BlueprintCallable)
    TArray<FBookfaceFriendRequestStructure> GetMyFriendRequests(const FString& MyUserId) const;

    UFUNCTION(BlueprintCallable)
    TArray<FBookfaceFriendRequestStructure> GetSentFriendRequests(const FString& MyUserId) const;

    UFUNCTION(BlueprintCallable)
    void RemoveFriendRequest(const FString& FromUserId, const FString& ToUserId);

    UFUNCTION(BlueprintCallable)
    void AcceptFriendRequest(const FString& FromUserId, const FString& ToUserId);

    UFUNCTION(BlueprintCallable)
    UBookfaceMessageObject* AddMessageToProfile(
        const FString& FromUserId,
        const FString& ToUserId,
        const FText& MessageText,
        UBookfaceMessageObject* ParentMessage = nullptr,
        bool IsTopLevel = false);

    UFUNCTION(BlueprintCallable)
    bool RemoveMessageFromProfile(UBookfaceMessageObject* MessageToRemove);

    UFUNCTION(BlueprintCallable)
	void NoticeSubscribeProfile(const FString& ProfileID, UBookfaceMessageObject* ParentMessage, UBookfaceMessageObject* Message);

    UFUNCTION(BlueprintCallable)
    void NoticeLikeMessage(const FString& ProfileID, const FString& LikerUserID, UBookfaceMessageObject* Message);

    UFUNCTION(BlueprintCallable)
    void StoreMessageNotice(const FString& ProfileId, const FBookfaceMessageNoticeStructure& Notice);

    UFUNCTION(BlueprintCallable)
	void RemoveMessageNotice(const FString& ProfileId, const FBookfaceMessageNoticeStructure& Notice);

    UFUNCTION(BlueprintCallable)
	void ClearAllMessageNotices(const FString& ProfileId);

    // Сообщения
    UFUNCTION(BlueprintCallable)
    void AddMessage(const FBF_MessageStructure& NewMessage);

    UFUNCTION(BlueprintCallable)
    const TArray<FBF_MessageStructure>& GetMessages() const;

    UFUNCTION(BlueprintCallable)
	TArray<FBF_MessageStructure> GetMessagesForUser(const FString& UserId) const;
    
private:
    UBookfaceMessageObject* FindRootMessage(UBookfaceMessageObject* Message) const;

    void ProcessingSubscriptions(UBookfaceMessageObject* Message, const FString& UserId);

public:
    UPROPERTY(BlueprintAssignable, BlueprintCallable)
    FOnlineStatusChange OnlineStatusChange;

    UPROPERTY(BlueprintAssignable, BlueprintCallable)
    FOnRemoveFriend OnRemoveFriend;

    UPROPERTY(BlueprintAssignable, BlueprintCallable)
    FOnAddFriendRequest OnAddFriendRequest;

    UPROPERTY(BlueprintAssignable, BlueprintCallable)
    FOnRemoveFriendRequest OnRemoveFriendRequest;

    UPROPERTY(BlueprintAssignable, BlueprintCallable)
    FOnAcceptFriendRequest OnAcceptFriendRequest;

    UPROPERTY(BlueprintAssignable, BlueprintCallable)
    FOnBookfaceMessageAdded OnMessageAdded;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
    FOnBookfaceMessageNotice OnMessageNotice;

    UPROPERTY(BlueprintAssignable, Category = "Messenger|Events")
    FOnBFMessageAdded OnChangeMessages;

private:
    UPROPERTY()
    TArray<FBookfaceProfileStructure> UserProfiles;

    UPROPERTY()
    TArray<FBookfaceFriendRequestStructure> FriendRequests;

    bool bHasLoadedSave = false;

    UPROPERTY()
    TArray<FBF_MessageStructure> Messages;
};