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
	if (!NewProfile.UserId.IsEmpty())
	{
		UserProfiles.AddUnique(NewProfile);
	}
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


