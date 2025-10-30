#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Styling/SlateBrush.h"
#include "Delegates/DelegateCombinations.h"
#include "InstantMessengerSubsystem.generated.h"

UENUM(BlueprintType)
enum class EDirectType : uint8
{
    Incoming UMETA(DisplayName = "Incoming"),
    Outgoing UMETA(DisplayName = "Outgoing")
};

USTRUCT(BlueprintType)
struct FContactsStructure
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FSlateBrush AvatarImage;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText ContactName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bIsOnline = false;

    FORCEINLINE bool operator==(const FContactsStructure& Other) const
    {
        return ContactName.EqualTo(Other.ContactName);
    }
};

USTRUCT(BlueprintType)
struct FMessageStructure
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText FromContact;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText ToContact;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EDirectType Direct = EDirectType::Incoming;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText Message;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bIsRead = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageAdded, const FMessageStructure&, Message);

UCLASS()
class FPSKITALSREFACTORED_API UInstantMessengerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // Контакты
    UFUNCTION(BlueprintCallable)
    void AddContact(const FContactsStructure& NewContact);

    UFUNCTION(BlueprintCallable)
	void SetActiveContact(const FText& ContactName, bool IsActive);

    UFUNCTION(BlueprintCallable)
    const TArray<FContactsStructure>& GetContacts() const;

    // Сообщения
    UFUNCTION(BlueprintCallable)
    void AddMessage(const FMessageStructure& NewMessage);

    UFUNCTION(BlueprintCallable)
    const TArray<FMessageStructure>& GetMessages() const;

    UFUNCTION(BlueprintCallable)
    void EditMessage(int32 Index, const FMessageStructure& NewData);

public:
    UPROPERTY(BlueprintAssignable, Category = "Messenger|Events")
    FOnMessageAdded OnChangeMessages;

private:
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    TArray<FContactsStructure> Contacts;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    TArray<FMessageStructure> Messages;
};