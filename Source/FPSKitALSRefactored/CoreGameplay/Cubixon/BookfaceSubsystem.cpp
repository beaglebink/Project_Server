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


