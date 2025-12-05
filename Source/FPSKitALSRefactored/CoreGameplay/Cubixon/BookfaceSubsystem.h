#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/Texture2D.h"
#include "BookfaceMessageObject.h"
#include "BookfaceDataTypes.h"
#include "BookfaceSaveGame.h"
#include "BookfaceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnlineStatusChange, const FString&, UserId, bool, bIsOnline);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRemoveFriend, const FString&, UserId, const FString&, UserFriend);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAddFriendRequest, const FString&, FromUserId, const FString&, ToUserId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRemoveFriendRequest, const FString&, FromUserId, const FString&, ToUserId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAcceptFriendRequest, const FString&, FromUserId, const FString&, ToUserId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBookfaceMessageAdded, UBookfaceMessageObject*, Message);
//DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBookfaceNoticesLoaded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBookfaceMessageNotice, FBookfaceMessageNoticeStructure, MessageNotice);

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
    /*
    UFUNCTION(BlueprintCallable)
    void SaveMessageNoticesAsync();

	UFUNCTION(BlueprintCallable)
    void LoadMessageNoticesAsync();
    */
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

    // Работа с сообщениями
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
	void NoticeSubscribeProfile(const FString& ProfileID, UBookfaceMessageObject* RootMessage, UBookfaceMessageObject* ParentMessage, UBookfaceMessageObject* Message);

    UFUNCTION(BlueprintCallable)
    void StoreMessageNotice(const FString& ProfileId, const FBookfaceMessageNoticeStructure& Notice);

/*  UFUNCTION(BlueprintCallable, BlueprintPure)
    TArray<UBookfaceMessageObject*> GetAllPosts() const;

    UFUNCTION(BlueprintCallable)
    void StoreMessageNotice(const FBookfaceMessageNoticeStructure& Notice);

    UFUNCTION(BlueprintCallable)
	TArray<FBookfaceMessageNoticeStructure> GetMessageNotices() const { return MessageNotices; }
    */
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

private:
    UPROPERTY()
    TArray<FBookfaceProfileStructure> UserProfiles;

    UPROPERTY()
    TArray<FBookfaceFriendRequestStructure> FriendRequests;
/*
    UPROPERTY()
	TArray<FBookfaceMessageNoticeStructure> MessageNotices;
*/
    bool bHasLoadedSave = false;
};