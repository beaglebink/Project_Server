#include "BookfaceSubsystem.h"
#include "Internationalization/Regex.h"

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


    for (int32 i = 0; i < UserProfiles.Num(); ++i)
    {
        if (UserProfiles[i].UserId == NewProfile.UserId)
        {
            UserProfiles[i] = NewProfile;
            return;
        }
    }
    UserProfiles.Add(NewProfile);
}

const TArray<FBookfaceProfileStructure>& UBookfaceSubsystem::GetUserProfiles() const
{
    return UserProfiles;
}

FBookfaceProfileStructure UBookfaceSubsystem::GetUserProfileById(const FString& UserId) const
{
    for (const auto& P : UserProfiles)
    {
        if (P.UserId == UserId) return P;
    }
    return FBookfaceProfileStructure();
}

bool UBookfaceSubsystem::SetOnlineStatus(const FString& UserId, const bool bOnline)
{
    for (auto& P : UserProfiles)
    {
        if (P.UserId == UserId)
        {
            P.IsOnline = bOnline;
            OnlineStatusChange.Broadcast(UserId, bOnline);
            return true;
        }
    }
    return false;
}

bool UBookfaceSubsystem::GetOnlineStatus(const FString& UserId, bool& IsOnline) const
{
    for (const auto& P : UserProfiles)
    {
        if (P.UserId == UserId)
        {
            IsOnline = P.IsOnline;
            return true;
        }
    }
    return false;
}

bool UBookfaceSubsystem::RemoveFriend(const FString& UserId, const FString& FriendId)
{
    bool bAnyRemoved = false;

    for (auto& P : UserProfiles)
    {
        if (P.UserId == UserId)
        {
            if (P.FriendsList.Remove(FriendId) > 0)
            {
                OnRemoveFriend.Broadcast(UserId, FriendId);
                bAnyRemoved = true;
            }
            break;
        }
    }

    for (auto& P : UserProfiles)
    {
        if (P.UserId == FriendId)
        {
            if (P.FriendsList.Remove(UserId) > 0)
            {
                OnRemoveFriend.Broadcast(FriendId, UserId);
                bAnyRemoved = true;
            }
            break;
        }
    }

    return bAnyRemoved;
}

TArray<FBookfaceProfileStructure> UBookfaceSubsystem::SearchProfiles(const FString& InUserID, const FString& Query) const
{
    TArray<FBookfaceProfileStructure> Result;
    if (Query.IsEmpty()) return Result;

    const FString LowerQuery = Query.ToLower();
    TSet<FString> AddedIds;
    AddedIds.Reserve(UserProfiles.Num());

    for (const auto& P : UserProfiles)
    {
        if (P.UserId == InUserID)
        {
            continue;
        }

        if (!AddedIds.Contains(P.UserId))
        {
            if (P.UserId.ToLower().Contains(LowerQuery))
            {
                Result.Add(P);
                AddedIds.Add(P.UserId);
                continue;
            }
        }

        if (!AddedIds.Contains(P.UserId))
        {
            const FString NameStr = P.DisplayName.ToString();
            if (NameStr.ToLower().Contains(LowerQuery))
            {
                Result.Add(P);
                AddedIds.Add(P.UserId);
                continue;
            }
        }

        if (!AddedIds.Contains(P.UserId))
        {
            const FString BioStr = P.Bio.ToString();
            if (BioStr.ToLower().Contains(LowerQuery))
            {
                Result.Add(P);
                AddedIds.Add(P.UserId);
                continue;
            }
        }
    }

    return Result;
}

void UBookfaceSubsystem::AddFriendRequest(const FString& FromUserId, const FString& ToUserId)
{
	auto FromUserProfile = GetUserProfileById(FromUserId);
	auto ToUserProfile = GetUserProfileById(ToUserId);
   
    if(FromUserProfile.FriendsList.Contains(ToUserId))
    {
        return;
	}

    FBookfaceFriendRequestStructure NewRequest;
    NewRequest.FromUserId = FromUserId;
    NewRequest.ToUserId = ToUserId;
	FriendRequests.Add(NewRequest);
	OnAddFriendRequest.Broadcast(FromUserId, ToUserId);
}

TArray<FBookfaceFriendRequestStructure> UBookfaceSubsystem::GetMyFriendRequests(const FString& MyUserId) const
{
    TArray<FBookfaceFriendRequestStructure> MyRequests;
    for (const auto& Request : FriendRequests)
    {
        if (Request.FromUserId == MyUserId)
        {
            MyRequests.AddUnique(Request);
        }
    }
	return MyRequests;
}

TArray<FBookfaceFriendRequestStructure> UBookfaceSubsystem::GetSentFriendRequests(const FString& MyUserId) const
{
    TArray<FBookfaceFriendRequestStructure> SentRequests;
    for (const auto& Request : FriendRequests)
    {
        if (Request.ToUserId == MyUserId)
        {
            SentRequests.AddUnique(Request);
        }
    }
	return SentRequests;
}

void UBookfaceSubsystem::RemoveFriendRequest(const FString& FromUserId, const FString& ToUserId)
{
    for (int32 i = FriendRequests.Num() - 1; i >= 0; --i)
    {
        if (FriendRequests[i].FromUserId == FromUserId && FriendRequests[i].ToUserId == ToUserId)
        {
            FriendRequests.RemoveAt(i);
			OnRemoveFriendRequest.Broadcast(FromUserId, ToUserId);
            OnRemoveFriendRequest.Broadcast(ToUserId, FromUserId);
            return;
        }
	}
}

void UBookfaceSubsystem::AcceptFriendRequest(const FString& FromUserId, const FString& ToUserId)
{
    for (int32 i = FriendRequests.Num() - 1; i >= 0; --i)
    {
        if (FriendRequests[i].FromUserId == FromUserId && FriendRequests[i].ToUserId == ToUserId)
        {
            FriendRequests.RemoveAt(i);
            break;
        }
    }
    for (auto& P : UserProfiles)
    {
        if (P.UserId == FromUserId)
        {
            if (!P.FriendsList.Contains(ToUserId))
            {
                P.FriendsList.AddUnique(ToUserId);
            }
            break;
        }
    }
    for (auto& P : UserProfiles)
    {
        if (P.UserId == ToUserId)
        {
            if (!P.FriendsList.Contains(FromUserId))
            {
                P.FriendsList.AddUnique(FromUserId);
            }
            break;
        }
	}

    OnAcceptFriendRequest.Broadcast(FromUserId, ToUserId);
    OnAcceptFriendRequest.Broadcast(ToUserId, FromUserId);
}

void UBookfaceSubsystem::UpdateUserProfile(const FBookfaceProfileStructure& UpdatedProfile)
{
    for (auto& P : UserProfiles)
    {
        if (P.UserId == UpdatedProfile.UserId)
        {
            P = UpdatedProfile;
            return;
        }
	}
}

