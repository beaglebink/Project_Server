#include "BookfaceSubsystem.h"
#include "BookfaceSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Internationalization/Regex.h"

void UBookfaceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UBookfaceSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UBookfaceSubsystem::OnBookfaceAppOpened()
{
    if (bHasLoadedSave)
    {
        UE_LOG(LogTemp, Log, TEXT("Bookface data already loaded."));
        return;
    }

    if (UGameplayStatics::DoesSaveGameExist(TEXT("BookfaceSlot"), 0))
    {
        LoadBookfaceDataAsync();
    }
    else
    {
        UBookfaceSaveGame* NewSave = Cast<UBookfaceSaveGame>(
            UGameplayStatics::CreateSaveGameObject(UBookfaceSaveGame::StaticClass()));

        if (NewSave)
        {
            NewSave->SavedProfiles.Empty();
            NewSave->SavedFriendRequests.Empty();
            NewSave->SavedAllPosts.Empty();

            UGameplayStatics::AsyncSaveGameToSlot(
                NewSave,
                TEXT("BookfaceSlot"),
                0,
                FAsyncSaveGameToSlotDelegate::CreateLambda([](const FString& SlotName, const int32 UserIndex, bool bSuccess)
                    {
                        if (bSuccess)
                        {
                            UE_LOG(LogTemp, Log, TEXT("Created new Bookface save slot '%s'."), *SlotName);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Failed to create Bookface save slot '%s'."), *SlotName);
                        }
                    })
            );
        }

        bHasLoadedSave = true;
    }
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

    if (bAnyRemoved)
    {
        SaveBookfaceDataAsync();
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
        if (P.UserId == InUserID) continue;

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

    if (FromUserProfile.FriendsList.Contains(ToUserId))
    {
        return;
    }

    FBookfaceFriendRequestStructure NewRequest;
    NewRequest.FromUserId = FromUserId;
    NewRequest.ToUserId = ToUserId;

    FriendRequests.Add(NewRequest);
    OnAddFriendRequest.Broadcast(FromUserId, ToUserId);

    SaveBookfaceDataAsync();
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
            SaveBookfaceDataAsync();
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
            P.FriendsList.AddUnique(ToUserId);
            break;
        }
    }

    for (auto& P : UserProfiles)
    {
        if (P.UserId == ToUserId)
        {
            P.FriendsList.AddUnique(FromUserId);
            break;
        }
    }

    OnAcceptFriendRequest.Broadcast(FromUserId, ToUserId);
    OnAcceptFriendRequest.Broadcast(ToUserId, FromUserId);

    SaveBookfaceDataAsync();
}

void UBookfaceSubsystem::UpdateUserProfile(const FBookfaceProfileStructure& UpdatedProfile)
{
    for (auto& P : UserProfiles)
    {
        if (P.UserId == UpdatedProfile.UserId)
        {
            P = UpdatedProfile;
            SaveBookfaceDataAsync();
            return;
        }
    }
    UserProfiles.Add(UpdatedProfile);
    SaveBookfaceDataAsync();
}

void UBookfaceSubsystem::SaveBookfaceMessagesAsync()
{
    UBookfaceSaveGame* SaveGame = nullptr;

    if (UGameplayStatics::DoesSaveGameExist(TEXT("BookfaceSlot"), 0))
    {
        SaveGame = Cast<UBookfaceSaveGame>(
            UGameplayStatics::LoadGameFromSlot(TEXT("BookfaceSlot"), 0));
    }

    if (!SaveGame)
    {
        SaveGame = Cast<UBookfaceSaveGame>(
            UGameplayStatics::CreateSaveGameObject(UBookfaceSaveGame::StaticClass()));
        SaveGame->SavedProfiles = UserProfiles;
        SaveGame->SavedFriendRequests = FriendRequests;
    }

    // Обновляем только сообщения
    SaveGame->SavedAllPosts.Empty();

    auto SaveMessageRecursive = [&](UBookfaceMessageObject* Msg, const FString& ParentId, auto&& Self) -> void
        {
            if (!Msg) return;

            if (Msg->MessageId.IsEmpty())
            {
                Msg->MessageId = FGuid::NewGuid().ToString();
            }

            FBookfaceMessageData Data;
            Data.MessageId = Msg->MessageId;
            Data.FromUserId = Msg->FromUserId;
            Data.ToUserId = Msg->ToUserId;
            Data.MessageContent = Msg->MessageContent;
            Data.Timestamp = Msg->Timestamp;
            Data.LikesUserIds = Msg->LikesUserIds;
            Data.ParentMessageId = ParentId;

            SaveGame->SavedAllPosts.Add(Data);

            for (UBookfaceMessageObject* Reply : Msg->ReplyMessages)
            {
                Self(Reply, Msg->MessageId, Self);
            }
        };

    for (UBookfaceMessageObject* RootPost : AllPosts)
    {
        SaveMessageRecursive(RootPost, TEXT(""), SaveMessageRecursive);
    }

    UGameplayStatics::AsyncSaveGameToSlot(
        SaveGame, TEXT("BookfaceSlot"), 0,
        FAsyncSaveGameToSlotDelegate::CreateLambda([](const FString& SlotName, const int32, bool bSuccess)
            {
                UE_LOG(LogTemp, Log, TEXT("Messages save %s for slot '%s'"),
                    bSuccess ? TEXT("succeeded") : TEXT("failed"), *SlotName);
            })
    );
}

void UBookfaceSubsystem::SaveBookfaceDataAsync()
{
    UBookfaceSaveGame* SaveGame = nullptr;

    if (UGameplayStatics::DoesSaveGameExist(TEXT("BookfaceSlot"), 0))
    {
        SaveGame = Cast<UBookfaceSaveGame>(
            UGameplayStatics::LoadGameFromSlot(TEXT("BookfaceSlot"), 0));
    }

    if (!SaveGame)
    {
        SaveGame = Cast<UBookfaceSaveGame>(
            UGameplayStatics::CreateSaveGameObject(UBookfaceSaveGame::StaticClass()));
    }

    SaveGame->SavedProfiles = UserProfiles;
    SaveGame->SavedFriendRequests = FriendRequests;

    UGameplayStatics::AsyncSaveGameToSlot(
        SaveGame, TEXT("BookfaceSlot"), 0,
        FAsyncSaveGameToSlotDelegate::CreateLambda([](const FString& SlotName, const int32, bool bSuccess)
            {
                UE_LOG(LogTemp, Log, TEXT("Profiles/requests save %s for slot '%s'"),
                    bSuccess ? TEXT("succeeded") : TEXT("failed"), *SlotName);
            })
    );
}

void UBookfaceSubsystem::LoadBookfaceDataAsync()
{
    if (!UGameplayStatics::DoesSaveGameExist(TEXT("BookfaceSlot"), 0))
    {
        return;
    }

    UGameplayStatics::AsyncLoadGameFromSlot(
        TEXT("BookfaceSlot"), 0,
        FAsyncLoadGameFromSlotDelegate::CreateLambda([this](const FString&, const int32, USaveGame* LoadedGame)
            {
                UBookfaceSaveGame* Loaded = Cast<UBookfaceSaveGame>(LoadedGame);
                if (!Loaded) return;

                UserProfiles = Loaded->SavedProfiles;
                FriendRequests = Loaded->SavedFriendRequests;

                AllPosts.Empty();
                TMap<FString, UBookfaceMessageObject*> IdToMessage;
                IdToMessage.Reserve(Loaded->SavedAllPosts.Num());

                // 1) Создаём все объекты сообщений
                for (const FBookfaceMessageData& Data : Loaded->SavedAllPosts)
                {
                    UBookfaceMessageObject* Msg = NewObject<UBookfaceMessageObject>(this);
                    Msg->MessageId = Data.MessageId;
                    Msg->FromUserId = Data.FromUserId;
                    Msg->ToUserId = Data.ToUserId;
                    Msg->MessageContent = Data.MessageContent;
                    Msg->Timestamp = Data.Timestamp;
                    Msg->LikesUserIds = Data.LikesUserIds;
                    Msg->ParentMessage = nullptr;
                    Msg->ReplyMessages.Reset();

                    IdToMessage.Add(Msg->MessageId, Msg);
                }

                for (const FBookfaceMessageData& Data : Loaded->SavedAllPosts)
                {
                    UBookfaceMessageObject* Msg = IdToMessage.FindRef(Data.MessageId);
                    if (!Msg) continue;

                    if (!Data.ParentMessageId.IsEmpty())
                    {
                        if (UBookfaceMessageObject* Parent = IdToMessage.FindRef(Data.ParentMessageId))
                        {
                            Msg->ParentMessage = Parent;
                            Parent->ReplyMessages.Add(Msg);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Missing parent '%s' for message '%s'"),
                                *Data.ParentMessageId, *Data.MessageId);
                        }
                    }
                    else
                    {
                        AllPosts.Add(Msg);
                    }
                }

                bHasLoadedSave = true;
                UE_LOG(LogTemp, Log, TEXT("Loaded %d profiles, %d root posts, %d messages total"),
                    UserProfiles.Num(), AllPosts.Num(), Loaded->SavedAllPosts.Num());
            })
    );
}

UBookfaceMessageObject* UBookfaceSubsystem::AddMessageToProfile(
    const FString& FromUserId,
    const FString& ToUserId,
    const FText& MessageText,
    UBookfaceMessageObject* ParentMessage,
    bool IsTopLevel)
{
    UBookfaceMessageObject* NewMessage = NewObject<UBookfaceMessageObject>(this);
    NewMessage->MessageId = FGuid::NewGuid().ToString();
    NewMessage->FromUserId = FromUserId;
    NewMessage->ToUserId = ToUserId;
    NewMessage->MessageContent = MessageText;
    NewMessage->Timestamp = FDateTime::UtcNow();
    NewMessage->ParentMessage = ParentMessage;

    if (ParentMessage)
    {
        ParentMessage->ReplyMessages.Add(NewMessage);
    }
    else
    {
        AllPosts.Add(NewMessage);
    }

    OnMessageAdded.Broadcast(NewMessage);
    return NewMessage;
}

bool UBookfaceSubsystem::RemoveMessageFromProfile(UBookfaceMessageObject* MessageToRemove)
{
    if (!MessageToRemove)
    {
        return false;
    }

    if (AllPosts.Contains(MessageToRemove))
    {
        AllPosts.Remove(MessageToRemove);
        return true;
    }

    return false;
}

TArray<UBookfaceMessageObject*> UBookfaceSubsystem::GetAllPosts() const
{
    TArray<UBookfaceMessageObject*> Reversed;
    Reversed.Reserve(AllPosts.Num());

    for (int32 Index = AllPosts.Num(); Index-- > 0; )
    {
        Reversed.Add(AllPosts[Index]);
    }
    return Reversed;
}