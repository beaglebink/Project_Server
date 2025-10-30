#include "InstantMessengerSubsystem.h"
#include "Engine/Engine.h"

void UInstantMessengerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("InstantMessengerSubsystem initialized"));
}

void UInstantMessengerSubsystem::Deinitialize()
{
    Super::Deinitialize();
    UE_LOG(LogTemp, Log, TEXT("InstantMessengerSubsystem deinitialized"));
}

void UInstantMessengerSubsystem::AddContact(const FContactsStructure& NewContact)
{
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
			return;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("Contact %s not found"), *ContactName.ToString());
}

const TArray<FContactsStructure>& UInstantMessengerSubsystem::GetContacts() const
{
    return Contacts;
}

void UInstantMessengerSubsystem::AddMessage(const FMessageStructure& NewMessage)
{
    Messages.Add(NewMessage);
    UE_LOG(LogTemp, Log, TEXT("Message added: [%s] %s"),
        *UEnum::GetValueAsString(NewMessage.Direct),
        *NewMessage.Message.ToString());
	OnChangeMessages.Broadcast(NewMessage);
}

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