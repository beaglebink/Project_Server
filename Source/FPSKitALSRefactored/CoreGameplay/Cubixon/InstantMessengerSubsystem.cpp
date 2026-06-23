#include "InstantMessengerSubsystem.h"
#include "Engine/Engine.h"
#include <GameSaveSubsystem.h>

void UInstantMessengerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("InstantMessengerSubsystem initialized"));

    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
        SaveSys->RegisterSaveableSubsystem(this);
}

void UInstantMessengerSubsystem::Deinitialize()
{
    Super::Deinitialize();
    UE_LOG(LogTemp, Log, TEXT("InstantMessengerSubsystem deinitialized"));
}

void UInstantMessengerSubsystem::AddContact(const FContactsStructure& NewContact)
{
    if (NewContact.ContactName.IsEmpty())
        return;

    Contacts.AddUnique(NewContact);
    UE_LOG(LogTemp, Log, TEXT("Contact added: %s"), *NewContact.ContactName.ToString());
}

void UInstantMessengerSubsystem::SetActiveContact(const FText& ContactName, bool IsActive)
{
	for (FContactsStructure& Contact : Contacts)
	{
		if (Contact.ContactName.EqualTo(ContactName))
		{
			Contact.bIsOnline = IsActive;
			UE_LOG(LogTemp, Log, TEXT("Contact %s is now %s"),
				*ContactName.ToString(),
				IsActive ? TEXT("online") : TEXT("offline"));
			OnChangeActiveContact.Broadcast(Contact);
			return;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("Contact %s not found"), *ContactName.ToString());
}

const TArray<FContactsStructure>& UInstantMessengerSubsystem::GetContacts() 
{
    //return Contacts;
    TArray<FContactsStructure> SortedContacts = Contacts; // создаём копию

    SortedContacts.Sort([](const FContactsStructure& A, const FContactsStructure& B)
        {
            return A.ContactName.ToString() < B.ContactName.ToString();
        });

	Contacts = SortedContacts; // обновляем оригинальный массив
    return Contacts; 
}

void UInstantMessengerSubsystem::AddMessage(const FMessageStructure& NewMessage)
{
    // Проверяем, существует ли уже сообщение с такими же полями (кроме bIsRead)
    for (const FMessageStructure& Existing : Messages)
    {
        if (Existing.FromContact.EqualTo(NewMessage.FromContact) &&
            Existing.ToContact.EqualTo(NewMessage.ToContact) &&
            Existing.Direct == NewMessage.Direct &&
            Existing.Message.EqualTo(NewMessage.Message))
        {
            // Сообщение уже есть — пропускаем добавление
            UE_LOG(LogTemp, Log, TEXT("Message already exists, skipped: [%s] %s"),
                *UEnum::GetValueAsString(NewMessage.Direct),
                *NewMessage.Message.ToString());
            return;
        }
    }

    // Совпадений не найдено — добавляем
    Messages.Add(NewMessage);
    UE_LOG(LogTemp, Log, TEXT("Message added: [%s] %s"),
        *UEnum::GetValueAsString(NewMessage.Direct),
        *NewMessage.Message.ToString());
    OnChangeMessages.Broadcast(NewMessage);
}

/*
void UInstantMessengerSubsystem::AddMessage(const FMessageStructure& NewMessage)
{
    Messages.Add(NewMessage);
    UE_LOG(LogTemp, Log, TEXT("Message added: [%s] %s"),
        *UEnum::GetValueAsString(NewMessage.Direct),
        *NewMessage.Message.ToString());
	OnChangeMessages.Broadcast(NewMessage);
}
*/
const TArray<FMessageStructure>& UInstantMessengerSubsystem::GetMessages() const
{
    return Messages;
}

void UInstantMessengerSubsystem::EditMessage(int32 Index, const FMessageStructure& NewData)
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

void UInstantMessengerSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
    OutData.SubsystemName = GetSaveSubsystemName();

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

    // Сериализуем Contacts
    TArray<TSharedPtr<FJsonValue>> ContactsArray;
    for (const FContactsStructure& Contact : Contacts)
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("ContactName"), Contact.ContactName.ToString());
        Obj->SetBoolField(TEXT("bIsOnline"), Contact.bIsOnline);

        // Сохраняем путь к текстуре аватара, если она есть
        if (Contact.AvatarImage.GetResourceObject())
        {
            UObject* Resource = Contact.AvatarImage.GetResourceObject();
            FString Path = Resource->GetPathName();
            Obj->SetStringField(TEXT("AvatarImagePath"), Path);
        }
        else
        {
            Obj->SetStringField(TEXT("AvatarImagePath"), TEXT(""));
        }

        ContactsArray.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("Contacts"), ContactsArray);

    // Сериализуем Messages
    TArray<TSharedPtr<FJsonValue>> MessagesArray;
    for (const FMessageStructure& Msg : Messages)
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("FromContact"), Msg.FromContact.ToString());
        Obj->SetStringField(TEXT("ToContact"), Msg.ToContact.ToString());
        Obj->SetStringField(TEXT("Direct"), UEnum::GetValueAsString(Msg.Direct));
        Obj->SetStringField(TEXT("Message"), Msg.Message.ToString());
        Obj->SetBoolField(TEXT("bIsRead"), Msg.bIsRead);
        MessagesArray.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("Messages"), MessagesArray);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    OutData.SerializedData = Output;
}

void UInstantMessengerSubsystem::ApplySaveData(const FSubsystemSaveData& InData)
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
        return;
    }

    Contacts.Empty();
    Messages.Empty();

    // Восстанавливаем Contacts
    const TArray<TSharedPtr<FJsonValue>>* ContactsArray = nullptr;
    if (Root->TryGetArrayField(TEXT("Contacts"), ContactsArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *ContactsArray)
        {
            const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
            if (!Val->TryGetObject(ObjPtr)) continue;
            const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

            FContactsStructure Contact;
            Contact.ContactName = FText::FromString(Obj->GetStringField(TEXT("ContactName")));
            Contact.bIsOnline = Obj->GetBoolField(TEXT("bIsOnline"));

            // Восстанавливаем текстуру аватара
            FString AvatarPath = Obj->GetStringField(TEXT("AvatarImagePath"));
            if (!AvatarPath.IsEmpty())
            {
                UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *AvatarPath);
                if (Texture)
                {
                    Contact.AvatarImage.SetResourceObject(Texture);
                    // Можно также установить размер, если нужно:
                    // Contact.AvatarImage.ImageSize = FVector2D(Texture->GetSurfaceWidth(), Texture->GetSurfaceHeight());
                }
            }

            Contacts.Add(Contact);
        }
    }

    // Восстанавливаем Messages
    const TArray<TSharedPtr<FJsonValue>>* MessagesArray = nullptr;
    if (Root->TryGetArrayField(TEXT("Messages"), MessagesArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *MessagesArray)
        {
            const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
            if (!Val->TryGetObject(ObjPtr)) continue;
            const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

            FMessageStructure Msg;
            Msg.FromContact = FText::FromString(Obj->GetStringField(TEXT("FromContact")));
            Msg.ToContact = FText::FromString(Obj->GetStringField(TEXT("ToContact")));
            FString DirectStr = Obj->GetStringField(TEXT("Direct"));
            UEnum* EnumPtr = StaticEnum<EDirectType>();
            int64 EnumVal = EnumPtr->GetValueByNameString(DirectStr);
            Msg.Direct = (EDirectType)EnumVal;
            Msg.Message = FText::FromString(Obj->GetStringField(TEXT("Message")));
            Msg.bIsRead = Obj->GetBoolField(TEXT("bIsRead"));
            Messages.Add(Msg);
        }
    }

    bIsLoadComplete = true;
}

