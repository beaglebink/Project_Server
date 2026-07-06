#include "SaveSubsystems/GIS_CMail.h"
#include <GameSaveSubsystem.h>
#include "CoreGameplay/Cubixon/O_CubixonCMailContact.h"

void UGIS_CMail::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
	{
		SaveSys->RegisterSaveableSubsystem(this);
	}
}

void UGIS_CMail::Deinitialize()
{
	Super::Deinitialize();
}

void UGIS_CMail::CollectSaveData(FSubsystemSaveData& OutData)
{
	OutData.SubsystemName = GetSaveSubsystemName();

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	for (UObject* SaveableObject : SaveableObjects)
	{
		if (!IsValid(SaveableObject))
		{
			continue;
		}

		TMap<FString, FCubixonCMailConversationsWrap> SaveDataMap = II_SaveableObject::Execute_CollectCMailSaveData(SaveableObject);

		for (const auto& Pair : SaveDataMap)
		{
			const FCubixonCMailConversationsWrap& CMailData = Pair.Value;
			TArray<TSharedPtr<FJsonValue>> ConversationsArrayJson;

			for (const FCubixonCMailConversation& Conversation : CMailData.Conversations)
			{
				TSharedPtr<FJsonObject> ConversationJson = MakeShared<FJsonObject>();

				ConversationJson->SetNumberField(TEXT("ID"), Conversation.ID);
				ConversationJson->SetStringField(TEXT("Subject"), Conversation.Subject.ToString());

				TArray<TSharedPtr<FJsonValue>> MessagesArrayJson;
				for (const FCubixonCMailMessage& Message : Conversation.Messages)
				{
					TSharedPtr<FJsonObject> MessageJson = MakeShared<FJsonObject>();
					MessageJson->SetStringField(TEXT("FromClass"), Message.From ? Message.From->GetPathName() : TEXT(""));
					MessageJson->SetStringField(TEXT("ToClass"), Message.To ? Message.To->GetPathName() : TEXT(""));
					MessageJson->SetStringField(TEXT("Message"), Message.Message.ToString());
					MessageJson->SetStringField(TEXT("Time"), Message.Time.ToString());

					TSharedPtr<FJsonObject> AttachedFileJson = MakeShared<FJsonObject>();
					MessageJson->SetBoolField(TEXT("bHasAttachedFileData"), Message.AttachedFileData.ComponentClass != nullptr);
					if (Message.AttachedFileData.ComponentClass)
					{
						AttachedFileJson->SetStringField(TEXT("FileName"), Message.AttachedFileData.FileName.ToString());
						AttachedFileJson->SetStringField(TEXT("ComponentClass"), Message.AttachedFileData.ComponentClass->GetPathName());
						AttachedFileJson->SetStringField(TEXT("SerializedData"), FBase64::Encode(Message.AttachedFileData.SerializedData));
					}

					MessageJson->SetObjectField(TEXT("AttachedFileData"), AttachedFileJson);
					MessagesArrayJson.Add(MakeShared<FJsonValueObject>(MessageJson));
				}
				ConversationJson->SetArrayField(TEXT("Messages"), MessagesArrayJson);

				ConversationJson->SetStringField(TEXT("DefaultFromContact"), Conversation.DefaultFromContact ? Conversation.DefaultFromContact->GetPathName() : TEXT(""));
				ConversationJson->SetBoolField(TEXT("bFrom_IsRead"), Conversation.bFrom_IsRead);
				ConversationJson->SetBoolField(TEXT("bFrom_Inbox"), Conversation.bFrom_Inbox);
				ConversationJson->SetBoolField(TEXT("bFrom_Sent"), Conversation.bFrom_Sent);
				ConversationJson->SetBoolField(TEXT("bFrom_Spam"), Conversation.bFrom_Spam);
				ConversationJson->SetBoolField(TEXT("bFrom_Deleted"), Conversation.bFrom_Deleted);
				ConversationJson->SetBoolField(TEXT("bFrom_PermanentlyDeleted"), Conversation.bFrom_PermanentlyDeleted);

				ConversationJson->SetStringField(TEXT("DefaultToContact"), Conversation.DefaultToContact ? Conversation.DefaultToContact->GetPathName() : TEXT(""));
				ConversationJson->SetBoolField(TEXT("bTo_IsRead"), Conversation.bTo_IsRead);
				ConversationJson->SetBoolField(TEXT("bTo_Inbox"), Conversation.bTo_Inbox);
				ConversationJson->SetBoolField(TEXT("bTo_Sent"), Conversation.bTo_Sent);
				ConversationJson->SetBoolField(TEXT("bTo_Spam"), Conversation.bTo_Spam);
				ConversationJson->SetBoolField(TEXT("bTo_Deleted"), Conversation.bTo_Deleted);
				ConversationJson->SetBoolField(TEXT("bTo_PermanentlyDeleted"), Conversation.bTo_PermanentlyDeleted);

				ConversationsArrayJson.Add(MakeShared<FJsonValueObject>(ConversationJson));
			}

			Root->SetArrayField(Pair.Key, ConversationsArrayJson);
		}
		break;
	}

	FString JsonString;

	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);

	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

	OutData.SerializedData = JsonString;
}

void UGIS_CMail::ApplySaveData(const FSubsystemSaveData& InData)
{
	bIsLoadComplete = false;

	CachedCMailData.Empty();

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
		return;
	}

	for (const auto& AccountPair : Root->Values)
	{
		const TArray<TSharedPtr<FJsonValue>>* ConversationsArrayJson;

		if (!Root->TryGetArrayField(AccountPair.Key, ConversationsArrayJson))
		{
			continue;
		}

		FCubixonCMailConversationsWrap ConversationsData;

		for (const auto& ConversationJson : *ConversationsArrayJson)
		{
			FCubixonCMailConversation Conversation;

			TSharedPtr<FJsonObject> ConversationJsonObject = ConversationJson->AsObject();
			if (!ConversationJsonObject.IsValid())
			{
				continue;
			}

			double ID;
			if (ConversationJsonObject->TryGetNumberField(TEXT("ID"), ID))
			{
				Conversation.ID = static_cast<int32>(ID);
			}

			FString Subject;
			if (ConversationJsonObject->TryGetStringField(TEXT("Subject"), Subject))
			{
				Conversation.Subject = FText::FromString(Subject);
			}


			const TArray<TSharedPtr<FJsonValue>>* MessagesArrayJson;
			if (ConversationJsonObject->TryGetArrayField(TEXT("Messages"), MessagesArrayJson))
			{
				for (const auto& MessageJson : *MessagesArrayJson)
				{
					FCubixonCMailMessage Message;

					TSharedPtr<FJsonObject> MessageJsonObject = MessageJson->AsObject();
					if (!MessageJsonObject.IsValid())
					{
						continue;
					}

					FString FromClassPath;
					if (MessageJsonObject->TryGetStringField(TEXT("FromClass"), FromClassPath) && !FromClassPath.IsEmpty())
					{
						Message.From = LoadClass<UO_CubixonCMailContact>(nullptr, *FromClassPath);
					}

					FString ToClassPath;
					if (MessageJsonObject->TryGetStringField(TEXT("ToClass"), ToClassPath) && !ToClassPath.IsEmpty())
					{
						Message.To = LoadClass<UO_CubixonCMailContact>(nullptr, *ToClassPath);
					}

					FString MessageText;
					if (MessageJsonObject->TryGetStringField(TEXT("Message"), MessageText))
					{
						Message.Message = FText::FromString(MessageText);
					}

					FString TimeText;
					if (MessageJsonObject->TryGetStringField(TEXT("Time"), TimeText))
					{
						Message.Time = FText::FromString(TimeText);
					}

					bool bHasAttachedFileData = false;

					if (MessageJsonObject->TryGetBoolField(TEXT("bHasAttachedFileData"), bHasAttachedFileData) && bHasAttachedFileData)
					{
						const TSharedPtr<FJsonObject>* AttachedFileJson;
						if (MessageJsonObject->TryGetObjectField(TEXT("AttachedFileData"), AttachedFileJson))
						{
							FString FileName;
							if ((*AttachedFileJson)->TryGetStringField(TEXT("FileName"), FileName))
							{
								Message.AttachedFileData.FileName = FText::FromString(FileName);
							}

							FString ComponentClassPath;
							if ((*AttachedFileJson)->TryGetStringField(TEXT("ComponentClass"), ComponentClassPath) && !ComponentClassPath.IsEmpty())
							{
								Message.AttachedFileData.ComponentClass = LoadClass<USceneComponent>(nullptr, *ComponentClassPath);
							}

							FString EncodedData;
							if ((*AttachedFileJson)->TryGetStringField(TEXT("SerializedData"), EncodedData))
							{
								FBase64::Decode(EncodedData, Message.AttachedFileData.SerializedData);
							}
						}
					}
					Conversation.Messages.Add(Message);
				}
			}

			FString DefaultFromContactPath;
			if (ConversationJsonObject->TryGetStringField(TEXT("DefaultFromContact"), DefaultFromContactPath) && !DefaultFromContactPath.IsEmpty())
			{
				Conversation.DefaultFromContact = LoadClass<UO_CubixonCMailContact>(nullptr, *DefaultFromContactPath);
			}

			bool bFrom_IsRead = false;
			if (ConversationJsonObject->TryGetBoolField(TEXT("bFrom_IsRead"), bFrom_IsRead))
			{
				Conversation.bFrom_IsRead = bFrom_IsRead;
			}

			bool bFrom_Inbox = false;
			if (ConversationJsonObject->TryGetBoolField(TEXT("bFrom_Inbox"), bFrom_Inbox))
			{
				Conversation.bFrom_Inbox = bFrom_Inbox;
			}

			bool bFrom_Sent = false;
			if (ConversationJsonObject->TryGetBoolField(TEXT("bFrom_Sent"), bFrom_Sent))
			{
				Conversation.bFrom_Sent = bFrom_Sent;
			}

			bool bFrom_Spam = false;
			if (ConversationJsonObject->TryGetBoolField(TEXT("bFrom_Spam"), bFrom_Spam))
			{
				Conversation.bFrom_Spam = bFrom_Spam;
			}

			bool bFrom_Deleted = false;
			if (ConversationJsonObject->TryGetBoolField(TEXT("bFrom_Deleted"), bFrom_Deleted))
			{
				Conversation.bFrom_Deleted = bFrom_Deleted;
			}

			bool bFrom_PermanentlyDeleted = false;
			if (ConversationJsonObject->TryGetBoolField(TEXT("bFrom_PermanentlyDeleted"), bFrom_PermanentlyDeleted))
			{
				Conversation.bFrom_PermanentlyDeleted = bFrom_PermanentlyDeleted;
			}

			FString DefaultToContactPath;
			if (ConversationJsonObject->TryGetStringField(TEXT("DefaultToContact"), DefaultToContactPath) && !DefaultToContactPath.IsEmpty())
			{
				Conversation.DefaultToContact = LoadClass<UO_CubixonCMailContact>(nullptr, *DefaultToContactPath);
			}

			bool bTo_IsRead = false;
			if (ConversationJsonObject->TryGetBoolField(TEXT("bTo_IsRead"), bTo_IsRead))
			{
				Conversation.bTo_IsRead = bTo_IsRead;
			}

			bool bTo_Inbox = false;
			if (ConversationJsonObject->TryGetBoolField(TEXT("bTo_Inbox"), bTo_Inbox))
			{
				Conversation.bTo_Inbox = bTo_Inbox;
			}

			bool bTo_Sent = false;
			if (ConversationJsonObject->TryGetBoolField(TEXT("bTo_Sent"), bTo_Sent))
			{
				Conversation.bTo_Sent = bTo_Sent;
			}

			bool bTo_Spam = false;
			if (ConversationJsonObject->TryGetBoolField(TEXT("bTo_Spam"), bTo_Spam))
			{
				Conversation.bTo_Spam = bTo_Spam;
			}

			bool bTo_Deleted = false;
			if (ConversationJsonObject->TryGetBoolField(TEXT("bTo_Deleted"), bTo_Deleted))
			{
				Conversation.bTo_Deleted = bTo_Deleted;
			}

			bool bTo_PermanentlyDeleted = false;
			if (ConversationJsonObject->TryGetBoolField(TEXT("bTo_PermanentlyDeleted"), bTo_PermanentlyDeleted))
			{
				Conversation.bTo_PermanentlyDeleted = bTo_PermanentlyDeleted;
			}

			ConversationsData.Conversations.Add(Conversation);
		}
		CachedCMailData.Add(AccountPair.Key, MoveTemp(ConversationsData));
	}

	bIsLoadComplete = true;
}

void UGIS_CMail::ClearTransientData_Implementation()
{
	SaveableObjects.Empty();
	UE_LOG(LogTemp, Log, TEXT("CMailSubsystem transient data cleared"));
}

void UGIS_CMail::AddSaveableObject(UObject* NewSaveableObject)
{
	if (NewSaveableObject && NewSaveableObject->Implements<UI_SaveableObject>())
	{
		SaveableObjects.AddUnique(NewSaveableObject);
	}
}

void UGIS_CMail::ApplyCMailData_Implementation(UObject* ProfileObject)
{
	if (!ProfileObject || !ProfileObject->Implements<UI_SaveableObject>())
	{
		return;
	}

	II_SaveableObject::Execute_ApplyCMailSaveData(ProfileObject, CachedCMailData);
}

