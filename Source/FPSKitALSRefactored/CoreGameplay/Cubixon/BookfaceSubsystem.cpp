#include "BookfaceSubsystem.h"
#include "BookfaceSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "GameSaveSubsystem.h"
#include <GameSaveSubsystem.h>

void UBookfaceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogTemp, Log, TEXT("BookfaceSubsystem initialized"));
    
    TryRegisterBookfaceSubsystem();
}

void UBookfaceSubsystem::TryRegisterBookfaceSubsystem()
{
    // Если уже зарегистрированы, очищаем таймер и выходим
    if (bIsRegistered)
    {
        if (RegistrationTimerHandle.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(RegistrationTimerHandle);
        }
        return;
    }

    UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>();
    if (SaveSys)
    {
        // Регистрируем подсистему
        SaveSys->RegisterSaveableSubsystem(this);
        bIsRegistered = true;

        // Очищаем таймер, если он был запущен
        if (RegistrationTimerHandle.IsValid())
        {
            GetWorld()->GetTimerManager().ClearTimer(RegistrationTimerHandle);
        }

        UE_LOG(LogTemp, Log, TEXT("BookfaceSubsystem successfully registered in GameSaveSubsystem"));
    }
    else
    {
        // Если SaveSys ещё не доступен, запускаем таймер (если ещё не запущен)
        if (!RegistrationTimerHandle.IsValid())
        {
            UWorld* World = GetWorld();
            if (World)
            {
                World->GetTimerManager().SetTimer(
                    RegistrationTimerHandle,
                    this,
                    &UBookfaceSubsystem::TryRegisterBookfaceSubsystem,
                    0.1f,
                    true // циклический
                );
                UE_LOG(LogTemp, Log, TEXT("BookfaceSubsystem: registration timer started (0.1s interval)"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("BookfaceSubsystem: no World available, cannot start timer"));
            }
        }
    }
}

void UBookfaceSubsystem::Deinitialize()
{
    Super::Deinitialize();

    if (RegistrationTimerHandle.IsValid())
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(RegistrationTimerHandle);
        }
    }
    bIsRegistered = false;

    Super::Deinitialize();

    UE_LOG(LogTemp, Log, TEXT("BookfaceSubsystem deinitialized"));
}

void UBookfaceSubsystem::AddUserProfile(const FBookfaceProfileStructure& NewProfile)
{
    for (int32 i = 0; i < UserProfiles.Num(); ++i)
    {
        if (UserProfiles[i].UserId == NewProfile.UserId)
        {
            //UserProfiles[i] = NewProfile;
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
        if (P.UserId == UserId) 
            return P;
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
        RequestSave();
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

    RequestSave();
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
            RequestSave();
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

void UBookfaceSubsystem::RequestSave()
{
    bSavePending = true;

    UE_LOG(LogTemp, Verbose,
        TEXT("BookfaceSubsystem: изменения помечены как несохранённые (уйдут в файл при следующем SaveGame())"));
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

void UBookfaceSubsystem::ApplyLoadedSaveData(UBookfaceSaveGame* LoadedGame)
{
    if (!LoadedGame) return;

    // Загружаем простые данные
    UserProfiles = LoadedGame->SavedProfiles;
    FriendRequests = LoadedGame->SavedFriendRequests;
    Messages = LoadedGame->SavedMessages;

    // Восстанавливаем иерархию сообщений
    TMap<FString, UBookfaceMessageObject*> GlobalIdToMessage;

    for (auto& Profile : UserProfiles)
    {
        Profile.UserMessages.Empty();
        TMap<FString, UBookfaceMessageObject*> IdToMessage;

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

    // Восстанавливаем уведомления
    for (auto& Profile : UserProfiles)
    {
        for (auto& Notice : Profile.MessageNotices)
        {
            Notice.Message = GlobalIdToMessage.FindRef(Notice.MessageId);
            Notice.RootMessage = GlobalIdToMessage.FindRef(Notice.RootMessageId);
            Notice.ParentMessage = GlobalIdToMessage.FindRef(Notice.ParentMessageId);
        }
    }

    bHasLoadedSave = true;
    UE_LOG(LogTemp, Log, TEXT("Applied loaded save data: %d profiles, %d requests, %d messages"),
        UserProfiles.Num(), FriendRequests.Num(), Messages.Num());
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
        UE_LOG(LogTemp, Log, TEXT("BookfaceSubsystem: data already loaded."));
        return;
    }
    // Загрузка теперь полностью управляется GameSaveSubsystem через ISaveableSubsystem::ApplySaveData.
    // Этот метод оставлен как BP-совместимый "no-op" (можно использовать как точку логирования).
    UE_LOG(LogTemp, Log, TEXT("BookfaceSubsystem: OnBookfaceAppOpened — data loading handled by GameSaveSubsystem"));
}

void UBookfaceSubsystem::SaveBookfaceMessagesAsync()
{
    RequestSave();
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

    RequestSave();
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
    RequestSave();
}

void UBookfaceSubsystem::SaveBookfaceDataAsync()
{
    RequestSave();
}

void UBookfaceSubsystem::LoadBookfaceDataAsync()
{
    // Загрузка теперь автоматическая через ApplySaveData()
    UE_LOG(LogTemp, Log, TEXT("BookfaceSubsystem: LoadBookfaceDataAsync — no-op (loading is handled by GameSaveSubsystem)"));
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
    // Частичное сохранение больше не поддерживается — сохраняем всё через интерфейс
    RequestSave();
}

void UBookfaceSubsystem::LoadMessagesOnly()
{
    UE_LOG(LogTemp, Log, TEXT("BookfaceSubsystem: LoadMessagesOnly — no-op (loading is handled by GameSaveSubsystem)"));
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

void UBookfaceSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
    OutData.SubsystemName = GetSaveSubsystemName();
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

    // ======================================================================
    // Шаг 0: Собираем глобальное множество валидных ID сообщений
    //         из деревьев UserMessages (источник истины в памяти).
    //         По нему ниже фильтруются осиротевшие уведомления.
    // ======================================================================
    TSet<FString> GlobalValidIds;

    TFunction<void(UBookfaceMessageObject*)> CollectIds =
        [&GlobalValidIds, &CollectIds](UBookfaceMessageObject* Msg)
        {
            if (!Msg) return;
            if (Msg->MessageId.IsEmpty())
            {
                Msg->MessageId = FGuid::NewGuid().ToString();
            }
            GlobalValidIds.Add(Msg->MessageId);
            for (UBookfaceMessageObject* Reply : Msg->ReplyMessages)
            {
                CollectIds(Reply);
            }
        };

    for (const FBookfaceProfileStructure& Profile : UserProfiles)
    {
        for (UBookfaceMessageObject* RootMsg : Profile.UserMessages)
        {
            CollectIds(RootMsg);
        }
    }

    // ======================================================================
    // UserProfiles (включая SavedMessages из UserMessages-дерева и MessageNotices)
    // ======================================================================
    TArray<TSharedPtr<FJsonValue>> ProfilesArray;
    for (const FBookfaceProfileStructure& Profile : UserProfiles)
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("UserId"), Profile.UserId);
        Obj->SetStringField(TEXT("DisplayName"), Profile.DisplayName.ToString());
        Obj->SetStringField(TEXT("AvatarImagePath"), Profile.AvatarImage.ToSoftObjectPath().ToString());
        Obj->SetStringField(TEXT("CoverImagePath"), Profile.CoverImage.ToSoftObjectPath().ToString());
        Obj->SetStringField(TEXT("Bio"), Profile.Bio.ToString());
        Obj->SetNumberField(TEXT("ReputationScore"), Profile.ReputationScore);
        Obj->SetStringField(TEXT("BIO_Privacy"), UEnum::GetValueAsString(Profile.BIO_Privacy));
        Obj->SetBoolField(TEXT("IsOnline"), Profile.IsOnline);

        // AboutInfo
        TArray<TSharedPtr<FJsonValue>> AboutArray;
        for (const FAboutInfoStructure& Info : Profile.AboutInfo)
        {
            TSharedPtr<FJsonObject> InfoObj = MakeShared<FJsonObject>();
            InfoObj->SetStringField(TEXT("InfoCaption"), Info.InfoCaption.ToString());
            InfoObj->SetStringField(TEXT("InfoDescription"), Info.InfoDescription.ToString());
            InfoObj->SetStringField(TEXT("PrivacyVisibility"), UEnum::GetValueAsString(Info.PrivacyVisibility));
            AboutArray.Add(MakeShared<FJsonValueObject>(InfoObj));
        }
        Obj->SetArrayField(TEXT("AboutInfo"), AboutArray);

        // FriendsList
        TArray<TSharedPtr<FJsonValue>> FriendsArray;
        for (const FString& FriendId : Profile.FriendsList)
            FriendsArray.Add(MakeShared<FJsonValueString>(FriendId));
        Obj->SetArrayField(TEXT("FriendsList"), FriendsArray);

        // --- SavedMessages: рекурсивно из UserMessages-дерева ---
        TArray<TSharedPtr<FJsonValue>> SavedMessagesArray;

        TFunction<void(UBookfaceMessageObject*, const FString&)> SaveMessageRecursive =
            [&SavedMessagesArray, &SaveMessageRecursive]
            (UBookfaceMessageObject* Msg, const FString& ParentId)
            {
                if (!Msg) return;

                if (Msg->MessageId.IsEmpty())
                {
                    Msg->MessageId = FGuid::NewGuid().ToString();
                }

                TSharedPtr<FJsonObject> MsgObj = MakeShared<FJsonObject>();
                MsgObj->SetStringField(TEXT("MessageId"), Msg->MessageId);
                MsgObj->SetStringField(TEXT("FromUserId"), Msg->FromUserId);
                MsgObj->SetStringField(TEXT("ToUserId"), Msg->ToUserId);
                MsgObj->SetStringField(TEXT("MessageContent"), Msg->MessageContent.ToString());
                MsgObj->SetStringField(TEXT("Timestamp"), Msg->Timestamp.ToString());
                MsgObj->SetStringField(TEXT("ParentMessageId"), ParentId);

                TArray<TSharedPtr<FJsonValue>> LikesArray;
                for (const FString& LikeId : Msg->LikesUserIds)
                    LikesArray.Add(MakeShared<FJsonValueString>(LikeId));
                MsgObj->SetArrayField(TEXT("LikesUserIds"), LikesArray);

                TArray<TSharedPtr<FJsonValue>> SubscribedArray;
                for (const FString& SubId : Msg->SubscribedUserIDs)
                    SubscribedArray.Add(MakeShared<FJsonValueString>(SubId));
                MsgObj->SetArrayField(TEXT("SubscribedUserIDs"), SubscribedArray);

                TArray<TSharedPtr<FJsonValue>> UnSubscribedArray;
                for (const FString& UnsubId : Msg->UnSubscribedUserIDs)
                    UnSubscribedArray.Add(MakeShared<FJsonValueString>(UnsubId));
                MsgObj->SetArrayField(TEXT("UnSubscribedUserIDs"), UnSubscribedArray);

                SavedMessagesArray.Add(MakeShared<FJsonValueObject>(MsgObj));

                for (UBookfaceMessageObject* Reply : Msg->ReplyMessages)
                {
                    SaveMessageRecursive(Reply, Msg->MessageId);
                }
            };

        for (UBookfaceMessageObject* RootMsg : Profile.UserMessages)
        {
            SaveMessageRecursive(RootMsg, TEXT(""));
        }
        Obj->SetArrayField(TEXT("SavedMessages"), SavedMessagesArray);

        // --- MessageNotices ---
        // 1) фиксируем ID из живых указателей;
        // 2) отбрасываем уведомления, чьё сообщение не принадлежит
        //    ни одному из деревьев UserMessages (GlobalValidIds).
        TArray<TSharedPtr<FJsonValue>> NoticesArray;
        for (const FBookfaceMessageNoticeStructure& Notice : Profile.MessageNotices)
        {
            FBookfaceMessageNoticeStructure FixedNotice = Notice;

            if (FixedNotice.Message && !FixedNotice.Message->MessageId.IsEmpty())
                FixedNotice.MessageId = FixedNotice.Message->MessageId;

            if (FixedNotice.RootMessage && !FixedNotice.RootMessage->MessageId.IsEmpty())
                FixedNotice.RootMessageId = FixedNotice.RootMessage->MessageId;

            if (FixedNotice.ParentMessage && !FixedNotice.ParentMessage->MessageId.IsEmpty())
                FixedNotice.ParentMessageId = FixedNotice.ParentMessage->MessageId;

            if (!GlobalValidIds.Contains(FixedNotice.MessageId))
                continue;

            TSharedPtr<FJsonObject> NoticeObj = MakeShared<FJsonObject>();
            NoticeObj->SetStringField(TEXT("MessageType"), UEnum::GetValueAsString(FixedNotice.MessageType));
            NoticeObj->SetStringField(TEXT("LikeUserId"), FixedNotice.LikeUserId);
            NoticeObj->SetStringField(TEXT("ProfileID"), FixedNotice.ProfileID);
            NoticeObj->SetStringField(TEXT("RootMessageId"), FixedNotice.RootMessageId);
            NoticeObj->SetStringField(TEXT("ParentMessageId"), FixedNotice.ParentMessageId);
            NoticeObj->SetStringField(TEXT("MessageId"), FixedNotice.MessageId);
            NoticesArray.Add(MakeShared<FJsonValueObject>(NoticeObj));
        }
        Obj->SetArrayField(TEXT("MessageNotices"), NoticesArray);

        ProfilesArray.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("UserProfiles"), ProfilesArray);

    // --- FriendRequests ---
    TArray<TSharedPtr<FJsonValue>> RequestsArray;
    for (const FBookfaceFriendRequestStructure& Req : FriendRequests)
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("FromUserId"), Req.FromUserId);
        Obj->SetStringField(TEXT("ToUserId"), Req.ToUserId);
        RequestsArray.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("FriendRequests"), RequestsArray);

    // --- Messages (FBF_MessageStructure) ---
    TArray<TSharedPtr<FJsonValue>> MessagesArray;
    for (const FBF_MessageStructure& Msg : Messages)
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("FromContact"), Msg.FromContact);
        Obj->SetStringField(TEXT("ToContact"), Msg.ToContact);
        Obj->SetStringField(TEXT("Message"), Msg.Message.ToString());
        Obj->SetBoolField(TEXT("bIsRead"), Msg.bIsRead);
        MessagesArray.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("Messages"), MessagesArray);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    OutData.SerializedData = Output;

    UE_LOG(LogTemp, Log, TEXT("BookfaceSubsystem: CollectSaveData done"));
}

void UBookfaceSubsystem::ApplySaveData(const FSubsystemSaveData& InData)
{
    bIsLoadComplete = false;

    if (InData.SerializedData.IsEmpty())
    {
        bIsLoadComplete = true;
        return;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InData.SerializedData);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        bIsLoadComplete = true;
        UE_LOG(LogTemp, Warning, TEXT("BookfaceSubsystem: Failed to parse JSON"));
        return;
    }

    // Очищаем текущие данные
    UserProfiles.Empty();
    FriendRequests.Empty();
    Messages.Empty();

    // ======================================================================
    // Шаг 1: Десериализация всех данных из JSON
    // ======================================================================

    // --- Десериализация UserProfiles (включая SavedMessages и MessageNotices) ---
    const TArray<TSharedPtr<FJsonValue>>* ProfilesArray = nullptr;
    if (Root->TryGetArrayField(TEXT("UserProfiles"), ProfilesArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *ProfilesArray)
        {
            const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
            if (!Val->TryGetObject(ObjPtr)) continue;
            const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

            FBookfaceProfileStructure Profile;
            Profile.UserId = Obj->GetStringField(TEXT("UserId"));
            Profile.DisplayName = FText::FromString(Obj->GetStringField(TEXT("DisplayName")));
            Profile.AvatarImage = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Obj->GetStringField(TEXT("AvatarImagePath"))));
            Profile.CoverImage = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Obj->GetStringField(TEXT("CoverImagePath"))));
            Profile.Bio = FText::FromString(Obj->GetStringField(TEXT("Bio")));
            Profile.ReputationScore = Obj->GetIntegerField(TEXT("ReputationScore"));
            FString PrivacyStr = Obj->GetStringField(TEXT("BIO_Privacy"));
            UEnum* PrivacyEnumPtr = StaticEnum<EPrivacyVisibility>();
            int64 EnumVal = PrivacyEnumPtr->GetValueByNameString(PrivacyStr);
            Profile.BIO_Privacy = (EPrivacyVisibility)EnumVal;
            Profile.IsOnline = Obj->GetBoolField(TEXT("IsOnline"));

            // AboutInfo
            const TArray<TSharedPtr<FJsonValue>>* AboutArray = nullptr;
            if (Obj->TryGetArrayField(TEXT("AboutInfo"), AboutArray))
            {
                for (const TSharedPtr<FJsonValue>& InfoVal : *AboutArray)
                {
                    const TSharedPtr<FJsonObject>* InfoObjPtr = nullptr;
                    if (!InfoVal->TryGetObject(InfoObjPtr)) continue;
                    const TSharedPtr<FJsonObject>& InfoObj = *InfoObjPtr;
                    FAboutInfoStructure Info;
                    Info.InfoCaption = FText::FromString(InfoObj->GetStringField(TEXT("InfoCaption")));
                    Info.InfoDescription = FText::FromString(InfoObj->GetStringField(TEXT("InfoDescription")));
                    FString PrivStr = InfoObj->GetStringField(TEXT("PrivacyVisibility"));
                    UEnum* PrivEnumPtr = StaticEnum<EPrivacyVisibility>();
                    int64 PrivVal = PrivEnumPtr->GetValueByNameString(PrivStr);
                    Info.PrivacyVisibility = (EPrivacyVisibility)PrivVal;
                    Profile.AboutInfo.Add(Info);
                }
            }

            // FriendsList
            const TArray<TSharedPtr<FJsonValue>>* FriendsArray = nullptr;
            if (Obj->TryGetArrayField(TEXT("FriendsList"), FriendsArray))
            {
                for (const TSharedPtr<FJsonValue>& FVal : *FriendsArray)
                    Profile.FriendsList.Add(FVal->AsString());
            }

            // --- Десериализация SavedMessages ---
            const TArray<TSharedPtr<FJsonValue>>* SavedMessagesArray = nullptr;
            if (Obj->TryGetArrayField(TEXT("SavedMessages"), SavedMessagesArray))
            {
                for (const TSharedPtr<FJsonValue>& MsgVal : *SavedMessagesArray)
                {
                    const TSharedPtr<FJsonObject>* MsgObjPtr = nullptr;
                    if (!MsgVal->TryGetObject(MsgObjPtr)) continue;
                    const TSharedPtr<FJsonObject>& MsgObj = *MsgObjPtr;

                    FBookfaceMessageData Data;
                    Data.MessageId = MsgObj->GetStringField(TEXT("MessageId"));
                    Data.FromUserId = MsgObj->GetStringField(TEXT("FromUserId"));
                    Data.ToUserId = MsgObj->GetStringField(TEXT("ToUserId"));
                    Data.MessageContent = FText::FromString(MsgObj->GetStringField(TEXT("MessageContent")));
                    FString TimestampStr = MsgObj->GetStringField(TEXT("Timestamp"));
                    FDateTime::ParseIso8601(*TimestampStr, Data.Timestamp);
                    Data.ParentMessageId = MsgObj->GetStringField(TEXT("ParentMessageId"));

                    // Массивы
                    const TArray<TSharedPtr<FJsonValue>>* LikesArray = nullptr;
                    if (MsgObj->TryGetArrayField(TEXT("LikesUserIds"), LikesArray))
                    {
                        for (const TSharedPtr<FJsonValue>& LVal : *LikesArray)
                            Data.LikesUserIds.Add(LVal->AsString());
                    }

                    const TArray<TSharedPtr<FJsonValue>>* SubArray = nullptr;
                    if (MsgObj->TryGetArrayField(TEXT("SubscribedUserIDs"), SubArray))
                    {
                        for (const TSharedPtr<FJsonValue>& SVal : *SubArray)
                            Data.SubscribedUserIDs.Add(SVal->AsString());
                    }

                    const TArray<TSharedPtr<FJsonValue>>* UnsubArray = nullptr;
                    if (MsgObj->TryGetArrayField(TEXT("UnSubscribedUserIDs"), UnsubArray))
                    {
                        for (const TSharedPtr<FJsonValue>& UVal : *UnsubArray)
                            Data.UnSubscribedUserIDs.Add(UVal->AsString());
                    }

                    Profile.SavedMessages.Add(Data);
                }
            }

            // --- Десериализация MessageNotices ---
            const TArray<TSharedPtr<FJsonValue>>* NoticesArray = nullptr;
            if (Obj->TryGetArrayField(TEXT("MessageNotices"), NoticesArray))
            {
                for (const TSharedPtr<FJsonValue>& NoticeVal : *NoticesArray)
                {
                    const TSharedPtr<FJsonObject>* NoticeObjPtr = nullptr;
                    if (!NoticeVal->TryGetObject(NoticeObjPtr)) continue;
                    const TSharedPtr<FJsonObject>& NoticeObj = *NoticeObjPtr;

                    FBookfaceMessageNoticeStructure Notice;
                    FString TypeStr = NoticeObj->GetStringField(TEXT("MessageType"));
                    UEnum* NotifyEnumPtr = StaticEnum<ENotifyType>();
                    int64 TypeVal = NotifyEnumPtr->GetValueByNameString(TypeStr);
                    Notice.MessageType = (ENotifyType)TypeVal;
                    Notice.LikeUserId = NoticeObj->GetStringField(TEXT("LikeUserId"));
                    Notice.ProfileID = NoticeObj->GetStringField(TEXT("ProfileID"));
                    Notice.RootMessageId = NoticeObj->GetStringField(TEXT("RootMessageId"));
                    Notice.ParentMessageId = NoticeObj->GetStringField(TEXT("ParentMessageId"));
                    Notice.MessageId = NoticeObj->GetStringField(TEXT("MessageId"));

                    Profile.MessageNotices.Add(Notice);
                }
            }

            UserProfiles.Add(Profile);
        }
    }

    // --- Десериализация FriendRequests ---
    const TArray<TSharedPtr<FJsonValue>>* RequestsArray = nullptr;
    if (Root->TryGetArrayField(TEXT("FriendRequests"), RequestsArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *RequestsArray)
        {
            const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
            if (!Val->TryGetObject(ObjPtr)) continue;
            const TSharedPtr<FJsonObject>& Obj = *ObjPtr;
            FBookfaceFriendRequestStructure Req;
            Req.FromUserId = Obj->GetStringField(TEXT("FromUserId"));
            Req.ToUserId = Obj->GetStringField(TEXT("ToUserId"));
            FriendRequests.Add(Req);
        }
    }

    // --- Десериализация Messages (FBF_MessageStructure) ---
    const TArray<TSharedPtr<FJsonValue>>* MessagesArray = nullptr;
    if (Root->TryGetArrayField(TEXT("Messages"), MessagesArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *MessagesArray)
        {
            const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
            if (!Val->TryGetObject(ObjPtr)) continue;
            const TSharedPtr<FJsonObject>& Obj = *ObjPtr;
            FBF_MessageStructure Msg;
            Msg.FromContact = Obj->GetStringField(TEXT("FromContact"));
            Msg.ToContact = Obj->GetStringField(TEXT("ToContact"));
            Msg.Message = FText::FromString(Obj->GetStringField(TEXT("Message")));
            Msg.bIsRead = Obj->GetBoolField(TEXT("bIsRead"));
            Messages.Add(Msg);
        }
    }

    // ======================================================================
    // Шаг 2: Подготовка временного контейнера (без записи в слот!)
    //         BookfaceSaveGame используется только как in-memory DTO,
    //         через который ApplyLoadedSaveData построит UserMessages.
    // ======================================================================
    UBookfaceSaveGame* SyncSaveGame = NewObject<UBookfaceSaveGame>();
    SyncSaveGame->SavedProfiles = UserProfiles;          // содержат SavedMessages, но не UserMessages
    SyncSaveGame->SavedFriendRequests = FriendRequests;
    SyncSaveGame->SavedMessages = Messages;

    // ======================================================================
    // Шаг 3: Восстановление UserMessages и объектов UBookfaceMessageObject
    //         из SavedMessages (для работы в памяти)
    // ======================================================================
    ApplyLoadedSaveData(SyncSaveGame); // теперь UserProfiles[].UserMessages заполнены

    // ======================================================================
    // Завершение
    // ======================================================================
    bIsLoadComplete = true;
    bHasLoadedSave = true;

    UE_LOG(LogTemp, Log, TEXT("BookfaceSubsystem: ApplySaveData completed (in-memory only)"));
}

void UBookfaceSubsystem::SyncSaveBookfaceData()
{
    RequestSave();
}
