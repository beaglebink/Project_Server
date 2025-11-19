#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BookfaceSubsystem.generated.h"

UENUM(BlueprintType)
enum class EPrivacyVisibility : uint8
{
	VisibleToMe       UMETA(DisplayName = "Visible to me"),
	VisibleToFriends  UMETA(DisplayName = "Visible to friends"),
	VisibleToEveryone UMETA(DisplayName = "Visible to everyone")
};

USTRUCT(BlueprintType)
struct FBookfaceFriendRequestStructure
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString FromUserId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString ToUserId;

	FORCEINLINE bool operator==(const FBookfaceFriendRequestStructure& Other) const
	{
		return FromUserId.Equals(Other.FromUserId) && ToUserId.Equals(Other.ToUserId);
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

public:
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
	int32 ReputationScore;

	FORCEINLINE bool operator==(const FBookfaceProfileStructure& Other) const
	{
		return UserId.Equals(Other.UserId);
	}

	bool IsOnline;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnlineStatusChange, const FString&, UserId, bool, bIsOnline);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRemoveFriend, const FString&, UserId, const FString&, UserFriend);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAddFriendRequest, const FString&, FromUserId, const FString&, ToUserId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRemoveFriendRequest, const FString&, FromUserId, const FString&, ToUserId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAcceptFriendRequest, const FString&, FromUserId, const FString&, ToUserId);

UCLASS()
class FPSKITALSREFACTORED_API UBookfaceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
	void AddUserProfile(const FBookfaceProfileStructure& NewProfile);

	UFUNCTION(BlueprintCallable)
	const TArray<FBookfaceProfileStructure>& GetUserProfiles() const;

	UFUNCTION(BlueprintCallable)
	FBookfaceProfileStructure GetUserProfileById(const FString& UserId) const;

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
	void UpdateUserProfile(const FBookfaceProfileStructure& UpdatedProfile);

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

private:
	UPROPERTY()
	TArray<FBookfaceProfileStructure> UserProfiles;

	UPROPERTY()
	TArray<FBookfaceFriendRequestStructure> FriendRequests;
};