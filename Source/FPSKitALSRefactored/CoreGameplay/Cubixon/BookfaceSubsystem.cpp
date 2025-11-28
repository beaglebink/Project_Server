#include "BookfaceSubsystem.h"
#include "BookfaceSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Internationalization/Regex.h"

void UBookfaceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Не загружаем сохранение сразу, ждём открытия приложения
}

void UBookfaceSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UBookfaceSubsystem::OnBookfaceAppOpened()
{
    // Если сохранение уже было загружено ранее — ничего не делаем
    if (bHasLoadedSave)
    {
        UE_LOG(LogTemp, Log, TEXT("Bookface data already loaded."));
        return;
    }

    // Проверяем, существует ли слот
    if (UGameplayStatics::DoesSaveGameExist(TEXT("BookfaceSlot"), 0))
    {
        // Загружаем данные
        LoadBookfaceDataAsync();
    }
    else
    {
        // Создаём новое сохранение, если слота нет
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
        return; // уже друзья
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

void UBookfaceSubsystem::SaveBookfaceDataAsync()
{
    UBookfaceSaveGame* SaveGame = Cast<UBookfaceSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UBookfaceSaveGame::StaticClass()));

    SaveGame->SavedProfiles = UserProfiles;
    SaveGame->SavedFriendRequests = FriendRequests;

    SaveGame->SavedAllPosts.Empty();

    // Обход дерева: прежде чем сохранять детей, убеждаемся, что у родителя есть MessageId
    auto SaveMessageRecursive = [&](UBookfaceMessageObject* Msg, const FString& ParentId, auto&& Self) -> void
        {
            if (!Msg) return;

            // ВАЖНО: если у сообщения нет ID — присвоить (это защищает и корни, и ответы)
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

            // Сохраняем ответы, передавая текущий MessageId как ParentId
            for (UBookfaceMessageObject* Reply : Msg->ReplyMessages)
            {
                if (!Reply) continue;
                Self(Reply, Msg->MessageId, Self);
            }
        };

    // Корневые посты всегда сохраняем с пустым ParentId
    for (UBookfaceMessageObject* RootPost : AllPosts)
    {
        SaveMessageRecursive(RootPost, TEXT(""), SaveMessageRecursive);
    }

    UGameplayStatics::AsyncSaveGameToSlot(
        SaveGame, TEXT("BookfaceSlot"), 0,
        FAsyncSaveGameToSlotDelegate::CreateLambda([](const FString& SlotName, const int32, bool bSuccess)
            {
                UE_LOG(LogTemp, Log, TEXT("Bookface save %s for slot '%s'"),
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
                    Msg->MessageId = Data.MessageId;        // критично для восстановления
                    Msg->FromUserId = Data.FromUserId;
                    Msg->ToUserId = Data.ToUserId;
                    Msg->MessageContent = Data.MessageContent;
                    Msg->Timestamp = Data.Timestamp;
                    Msg->LikesUserIds = Data.LikesUserIds;
                    Msg->ParentMessage = nullptr;               // очистка, будем проставлять ниже
                    Msg->ReplyMessages.Reset();

                    IdToMessage.Add(Msg->MessageId, Msg);
                }

                // 2) Восстанавливаем связи. Корни — только с пустым ParentMessageId
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
                            // Страховка: нет родителя — не делать корнем, но залогировать
                            UE_LOG(LogTemp, Warning, TEXT("Missing parent '%s' for message '%s'"),
                                *Data.ParentMessageId, *Data.MessageId);
                        }
                    }
                    else
                    {
                        // Корневой пост — только здесь добавляем в AllPosts
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
        // Это комментарий/ответ
        ParentMessage->ReplyMessages.Add(NewMessage);
    }
    else
    {
        // Это корневой пост
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