#include "BookfaceSubsystem.h"
#include "Engine/Engine.h"

void UBookfaceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UBookfaceSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UBookfaceSubsystem::AddUserProfile(const FBookfaceProfileStructure& NewProfile)
{
	if (NewProfile.UserId.IsEmpty())
		return;
	
	UserProfiles.AddUnique(NewProfile);
}

const TArray<FBookfaceProfileStructure>& UBookfaceSubsystem::GetUserProfiles() const
{
	return UserProfiles;
}

FBookfaceProfileStructure UBookfaceSubsystem::GetUserProfileById(const FString& UserId) const
{
	for (const FBookfaceProfileStructure& Profile : UserProfiles)
	{
		if (Profile.UserId.Equals(UserId))
		{
			return Profile;
		}
	}
	return FBookfaceProfileStructure();
}

bool UBookfaceSubsystem::SetOnlineStatus(const FString& UserId, const bool bOnline)
{
	auto Profile = GetUserProfileById(UserId);
	if (Profile.UserId.IsEmpty())
		return false;

	UserProfiles.Remove(Profile);

	Profile.IsOnline = bOnline;

	UserProfiles.Add(Profile);

	OnlineStatusChange.Broadcast(UserId, bOnline);
	return true;
}

bool UBookfaceSubsystem::GetOnlineStatus(const FString& UserId, bool& IsOnline) const
{
	auto Profile = GetUserProfileById(UserId);
	if (Profile.UserId.IsEmpty())
		return false;

	IsOnline = Profile.IsOnline;
	return true;
}

bool UBookfaceSubsystem::RemoveFriend(const FString& UserId, const FString& FriendId)
{
	if (UserId == FriendId)
		return false;

	auto MyProfile = GetUserProfileById(UserId);
	if (MyProfile.UserId.IsEmpty())
		return false;

	auto FriendProfile = GetUserProfileById(FriendId);
	if (FriendProfile.UserId.IsEmpty())
		return false;
/*
	if (!MyProfile.FriendsList.Contains(FriendId))
		return false;

	if (!FriendProfile.FriendsList.Contains(UserId))
		return false;
*/
	UserProfiles.Remove(MyProfile);
	UserProfiles.Remove(FriendProfile);

	MyProfile.FriendsList.Remove(FriendId);
	UserProfiles.Add(MyProfile);

	FriendProfile.FriendsList.Remove(UserId);
	UserProfiles.Add(FriendProfile);

	OnRemoveFriend.Broadcast(UserId, FriendId);

	return true;
}



