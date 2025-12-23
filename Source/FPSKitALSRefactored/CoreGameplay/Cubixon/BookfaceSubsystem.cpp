#include "BookfaceSubsystem.h"
#include "BookfaceSaveGame.h"
#include "Kismet/GameplayStatics.h"

void UBookfaceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogTemp, Log, TEXT("BookfaceSubsystem initialized"));
}

void UBookfaceSubsystem::Deinitialize()
{
    Super::Deinitialize();

    UE_LOG(LogTemp, Log, TEXT("BookfaceSubsystem deinitialized"));
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

UBookfaceMessageObject* UBookfaceSubsystem::AddMessageToProfile(
    const FString& FromUserId,
    const FString& ToUserId,
    const FText& MessageText,
    UBookfaceMessageObject* ParentMessage,
    bool /*IsTopLevel*/)
{
    FBookfaceProfileStructure* TargetProfile = UserProfiles.FindByPredicate(
        [&](const FBookfaceProfileStructure& Profile) { return Profile.UserId == ToUserId; });

    if (!TargetProfile)
    {
        UE_LOG(LogTemp, Warning, TEXT("Profile %s not found"), *ToUserId);
        return nullptr;
    }

    UBookfaceMessageObject* NewMessage = NewObject<UBookfaceMessageObject>(this);
    NewMessage->MessageId = FGuid::NewGuid().ToString();
    NewMessage->FromUserId = FromUserId;
    NewMessage->ToUserId = ToUserId;
    NewMessage->MessageContent = MessageText;
    NewMessage->Timestamp = FDateTime::UtcNow();
    NewMessage->ParentMessage = ParentMessage;
    NewMessage->ReplyMessages.Reset();


    auto RootMessage = FindRootMessage(NewMessage);
    //RootMessage->SubscribedUserIDs.AddUnique(FromUserId);

    if (!ParentMessage && FromUserId != ToUserId)
    {
		RootMessage->SubscribedUserIDs.AddUnique(ToUserId);
        RootMessage->SubscribedUserIDs.AddUnique(FromUserId);
		RootMessage->UnSubscribedUserIDs.Remove(ToUserId);
    }

    ProcessingSubscriptions(NewMessage, FromUserId);

    if (ParentMessage)
    {
        ParentMessage->ReplyMessages.Add(NewMessage);
    }
    else
    {
        TargetProfile->UserMessages.Add(NewMessage);
    }

    OnMessageAdded.Broadcast(NewMessage);
    return NewMessage;
}

bool UBookfaceSubsystem::RemoveMessageFromProfile(UBookfaceMessageObject* MessageToRemove)
{
    if (!MessageToRemove) return false;

    if (MessageToRemove->ParentMessage)
    {
        return MessageToRemove->ParentMessage->ReplyMessages.Remove(MessageToRemove) > 0;
    }

    for (auto& Profile : UserProfiles)
    {
        if (Profile.UserId == MessageToRemove->ToUserId)
        {
            return Profile.UserMessages.Remove(MessageToRemove) > 0;
        }
    }

    return false;
}
void UBookfaceSubsystem::NoticeSubscribeProfile(const FString& ProfileID,  UBookfaceMessageObject* ParentMessage, UBookfaceMessageObject* Message)
{
    if(ProfileID.IsEmpty())
    {
        return;
    }

    /*
    if(!RootMessage)
    {
        return;
	}
    */

    FBookfaceMessageNoticeStructure NewNotice;
	NewNotice.MessageType = ENotifyType::MessageActivityNotify;
    NewNotice.ProfileID = ProfileID;
    NewNotice.RootMessage = FindRootMessage(Message);
	NewNotice.ParentMessage = Message->ParentMessage;
    NewNotice.Message = Message;

    OnMessageNotice.Broadcast(NewNotice);
}

void UBookfaceSubsystem::NoticeLikeMessage(const FString& ProfileID, const FString& LikerUserID, UBookfaceMessageObject* Message)
{
    if(ProfileID.IsEmpty() || LikerUserID.IsEmpty() || !Message)
    {
        return;
    }
    FBookfaceMessageNoticeStructure NewNotice;
    NewNotice.MessageType = ENotifyType::MessageLikeNotify;
	NewNotice.LikeUserId = LikerUserID;
    NewNotice.ProfileID = ProfileID;
    NewNotice.RootMessage = FindRootMessage(Message);
    NewNotice.ParentMessage = Message->ParentMessage;
    NewNotice.Message = Message;
    OnMessageNotice.Broadcast(NewNotice);
}

UBookfaceMessageObject* UBookfaceSubsystem::FindRootMessage(UBookfaceMessageObject* Message) const
{
    if (!Message)
    {
        return nullptr;
    }

    UBookfaceMessageObject* Current = Message;

    // поднимаемся вверх по цепочке родителей
    while (Current->ParentMessage != nullptr)
    {
        Current = Current->ParentMessage;
    }

    return Current; // это корневое сообщение
}

void UBookfaceSubsystem::ProcessingSubscriptions(UBookfaceMessageObject* Message, const FString& UserId)
{
    if (!Message)
    {
        return;
    }

    UBookfaceMessageObject* Current = Message;

    Current->SubscribedUserIDs.AddUnique(UserId);
    Current->UnSubscribedUserIDs.Remove(UserId);

    while (Current->ParentMessage != nullptr)
    {
        Current = Current->ParentMessage;

        Current->SubscribedUserIDs.AddUnique(UserId);
        Current->UnSubscribedUserIDs.Remove(UserId);
    }

    return;
}


void UBookfaceSubsystem::StoreMessageNotice(const FString& ProfileID, const FBookfaceMessageNoticeStructure& Notice)
{
    FBookfaceProfileStructure* Profile = UserProfiles.FindByPredicate(
        [&](const FBookfaceProfileStructure& P) { return P.UserId == ProfileID; });

    if (Profile)
    {
        FBookfaceMessageNoticeStructure FixedNotice = Notice;

        // 🔹 гарантируем, что MessageId заполнен
        if (FixedNotice.Message && FixedNotice.Message->MessageId.IsEmpty() == false)
        {
            FixedNotice.MessageId = FixedNotice.Message->MessageId;
        }

        // 🔹 гарантируем, что RootMessageId заполнен
        if (FixedNotice.RootMessage && FixedNotice.RootMessage->MessageId.IsEmpty() == false)
        {
            FixedNotice.RootMessageId = FixedNotice.RootMessage->MessageId;
        }

        // 🔹 гарантируем, что ParentMessageId заполнен
        if (FixedNotice.ParentMessage && FixedNotice.ParentMessage->MessageId.IsEmpty() == false)
        {
            FixedNotice.ParentMessageId = FixedNotice.ParentMessage->MessageId;
        }
        /*
        if (Profile->MessageNotices.Contains(FixedNotice) && Notice.MessageType == ENotifyType::MessageLikeNotify)
        {
            Profile->MessageNotices.RemoveAll([&](const FBookfaceMessageNoticeStructure& ExistingNotice)
                {
                    return ExistingNotice == FixedNotice;
				});
        }
        */
        Profile->MessageNotices.AddUnique(FixedNotice);
    }
}

void UBookfaceSubsystem::RemoveMessageNotice(const FString& ProfileId, const FBookfaceMessageNoticeStructure& Notice)
{
    FBookfaceProfileStructure* Profile = UserProfiles.FindByPredicate(
        [&](const FBookfaceProfileStructure& P) { return P.UserId == ProfileId; });
    if (Profile)
    {
        Profile->MessageNotices.RemoveAll([&](const FBookfaceMessageNoticeStructure& ExistingNotice)
            {
                return ExistingNotice == Notice;
            });
	}
}

void UBookfaceSubsystem::ClearAllMessageNotices(const FString& ProfileId)
{
    FBookfaceProfileStructure* Profile = UserProfiles.FindByPredicate(
        [&](const FBookfaceProfileStructure& P) { return P.UserId == ProfileId; });
    if (Profile)
    {
        Profile->MessageNotices.Empty();
	}
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
            //NewSave->SavedAllPosts.Empty();

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
    }

    // Сохраняем профили и заявки
    SaveGame->SavedProfiles = UserProfiles;
    SaveGame->SavedFriendRequests = FriendRequests;

    // 🔹 Глобальное множество всех валидных ID сообщений
    TSet<FString> GlobalValidIds;

    // Сериализуем сообщения для каждого профиля
    for (int32 i = 0; i < UserProfiles.Num(); ++i)
    {
        SaveGame->SavedProfiles[i].SavedMessages.Empty();

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
                Data.SubscribedUserIDs = Msg->SubscribedUserIDs;
                Data.UnSubscribedUserIDs = Msg->UnSubscribedUserIDs;

                SaveGame->SavedProfiles[i].SavedMessages.Add(Data);

                // Добавляем в глобальный список валидных ID
                GlobalValidIds.Add(Data.MessageId);

                for (UBookfaceMessageObject* Reply : Msg->ReplyMessages)
                {
                    Self(Reply, Msg->MessageId, Self);
                }
            };

        for (UBookfaceMessageObject* RootPost : UserProfiles[i].UserMessages)
        {
            SaveMessageRecursive(RootPost, TEXT(""), SaveMessageRecursive);
        }

        // 🔹 Перед сохранением уведомлений гарантируем, что ID заполнены
        for (auto& Notice : UserProfiles[i].MessageNotices)
        {
            if (Notice.Message && !Notice.Message->MessageId.IsEmpty())
                Notice.MessageId = Notice.Message->MessageId;

            if (Notice.RootMessage && !Notice.RootMessage->MessageId.IsEmpty())
                Notice.RootMessageId = Notice.RootMessage->MessageId;

            if (Notice.ParentMessage && !Notice.ParentMessage->MessageId.IsEmpty())
                Notice.ParentMessageId = Notice.ParentMessage->MessageId;
        }
    }

    // 🔹 Теперь чистим уведомления по глобальному множеству ID
    for (int32 i = 0; i < SaveGame->SavedProfiles.Num(); ++i)
    {
        SaveGame->SavedProfiles[i].MessageNotices.RemoveAll([&](const FBookfaceMessageNoticeStructure& Notice)
            {
                return !GlobalValidIds.Contains(Notice.MessageId);
            });
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

    // сериализация сообщений в SavedMessages каждого профиля
    for (int32 i = 0; i < UserProfiles.Num(); ++i)
    {
        SaveGame->SavedProfiles[i].SavedMessages.Empty();

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
                Data.SubscribedUserIDs = Msg->SubscribedUserIDs;
                Data.UnSubscribedUserIDs = Msg->UnSubscribedUserIDs;

                SaveGame->SavedProfiles[i].SavedMessages.Add(Data);

                for (UBookfaceMessageObject* Reply : Msg->ReplyMessages)
                {
                    Self(Reply, Msg->MessageId, Self);
                }
            };

        for (UBookfaceMessageObject* RootPost : UserProfiles[i].UserMessages)
        {
            SaveMessageRecursive(RootPost, TEXT(""), SaveMessageRecursive);
        }
    }

    UGameplayStatics::AsyncSaveGameToSlot(
        SaveGame, TEXT("BookfaceSlot"), 0,
        FAsyncSaveGameToSlotDelegate::CreateLambda([](const FString& SlotName, const int32, bool bSuccess)
            {
                UE_LOG(LogTemp, Log, TEXT("Profiles/requests/messages save %s for slot '%s'"),
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

                // Загружаем профили и заявки
                UserProfiles = Loaded->SavedProfiles;
                FriendRequests = Loaded->SavedFriendRequests;

                // Глобальная карта всех сообщений по ID
                TMap<FString, UBookfaceMessageObject*> GlobalIdToMessage;

                // Восстанавливаем сообщения для каждого профиля
                for (auto& Profile : UserProfiles)
                {
                    Profile.UserMessages.Empty();

                    TMap<FString, UBookfaceMessageObject*> IdToMessage;

                    // Создаём объекты сообщений из сериализованных данных
                    for (const FBookfaceMessageData& Data : Profile.SavedMessages)
                    {
                        UBookfaceMessageObject* Msg = NewObject<UBookfaceMessageObject>(this);
                        Msg->MessageId = Data.MessageId;
                        Msg->FromUserId = Data.FromUserId;
                        Msg->ToUserId = Data.ToUserId;
                        Msg->MessageContent = Data.MessageContent;
                        Msg->Timestamp = Data.Timestamp;
                        Msg->LikesUserIds = Data.LikesUserIds;
                        Msg->SubscribedUserIDs = Data.SubscribedUserIDs;
                        Msg->UnSubscribedUserIDs = Data.UnSubscribedUserIDs;
                        Msg->ParentMessage = nullptr;
                        Msg->ReplyMessages.Reset();

                        IdToMessage.Add(Msg->MessageId, Msg);
                        GlobalIdToMessage.Add(Msg->MessageId, Msg);
                    }

                    // Восстанавливаем связи родитель ↔ ответы
                    for (const FBookfaceMessageData& Data : Profile.SavedMessages)
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
                            Profile.UserMessages.Add(Msg);
                        }
                    }
                }

                // 🔹 Восстанавливаем уведомления для всех профилей
                for (auto& Profile : UserProfiles)
                {
                    for (auto& Notice : Profile.MessageNotices)
                    {
                        if (Notice.MessageId.IsEmpty())
                        {
                            UE_LOG(LogTemp, Warning, TEXT("Notice for profile '%s' has empty MessageId"),
                                *Profile.UserId);
                            continue;
                        }

                        Notice.Message = GlobalIdToMessage.FindRef(Notice.MessageId);
                        Notice.RootMessage = GlobalIdToMessage.FindRef(Notice.RootMessageId);
                        Notice.ParentMessage = GlobalIdToMessage.FindRef(Notice.ParentMessageId);
                    }
                }

                bHasLoadedSave = true;

                int32 RootCount = 0;
                for (const auto& P : UserProfiles) RootCount += P.UserMessages.Num();

                UE_LOG(LogTemp, Log, TEXT("Loaded %d profiles, %d requests, %d root posts"),
                    UserProfiles.Num(), FriendRequests.Num(), RootCount);
            })
    );
}

void UBookfaceSubsystem::AddMessage(const FBF_MessageStructure& NewMessage)
{
    Messages.Add(NewMessage);
    UE_LOG(LogTemp, Log, TEXT("Bookface Message added: [From %s, To %s] %s"),
		*NewMessage.FromContact,
		*NewMessage.ToContact,
        *NewMessage.Message.ToString());
    OnChangeMessages.Broadcast(NewMessage);
}

const TArray<FBF_MessageStructure>& UBookfaceSubsystem::GetMessages() const
{
    return Messages;
}

TArray<FBF_MessageStructure> UBookfaceSubsystem::GetMessagesForUser(const FString& UserId) const
{
    TArray<FBF_MessageStructure> UserMessages;
    for (const auto& Msg : Messages)
    {
        if (Msg.ToContact == UserId || Msg.FromContact == UserId)
        {
            UserMessages.Add(Msg);
        }
    }
	return UserMessages;
}

TArray<FBF_MessageStructure> UBookfaceSubsystem::GetMessagesFromUser(const FString& UserId) const
{
    TArray<FBF_MessageStructure> UserMessages;
    for (const auto& Msg : Messages)
    {
        if (Msg.FromContact == UserId)
        {
            UserMessages.Add(Msg);
        }
    }
    return UserMessages;
}

void UBookfaceSubsystem::EditMessage(int32 Index, const FBF_MessageStructure& NewData)
{
    if (Messages.IsValidIndex(Index))
    {
        Messages[Index] = NewData;
        UE_LOG(LogTemp, Log, TEXT("Message at index %d updated."), Index);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid message index: %d"), Index);
    }
}

void UBookfaceSubsystem::SaveMessagesOnly()
{
    UBookfaceSaveGame* SaveGameInstance = nullptr;

    // Загружаем существующий слот
    if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(TEXT("BookfaceSlot"), 0))
    {
        SaveGameInstance = Cast<UBookfaceSaveGame>(Loaded);
    }
    else
    {
        SaveGameInstance = Cast<UBookfaceSaveGame>(
            UGameplayStatics::CreateSaveGameObject(UBookfaceSaveGame::StaticClass()));
    }

    if (SaveGameInstance)
    {
        SaveGameInstance->SavedMessages = Messages;

        // Важно: профили и заявки остаются нетронутыми
        UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("BookfaceSlot"), 0);
    }
}

void UBookfaceSubsystem::LoadMessagesOnly()
{
    if (USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(TEXT("BookfaceSlot"), 0))
    {
        if (UBookfaceSaveGame* SaveGameInstance = Cast<UBookfaceSaveGame>(Loaded))
        {
            Messages = SaveGameInstance->SavedMessages;
            bHasLoadedSave = true;
        }
    }
}

int32 UBookfaceSubsystem::GetUnreadMessageCountForUser(const FString& FromUserId, const FString& ToUserId) const
{
    int32 UnreadCount = 0;

    const TArray<FBF_MessageStructure> UserMessages = GetMessagesForUser(FromUserId);
    for (const FBF_MessageStructure& Message : UserMessages)
    {
        if (!Message.bIsRead && Message.FromContact == FromUserId && Message.ToContact == ToUserId)
        {
            ++UnreadCount;
        }
    }

    return UnreadCount;
}