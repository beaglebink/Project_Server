#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BookfaceSubsystem.generated.h"

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

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnlineStatusChange OnlineStatusChange;

private:
	TArray<FBookfaceProfileStructure> UserProfiles;
};