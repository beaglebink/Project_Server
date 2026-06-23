#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Styling/SlateBrush.h"
#include "Delegates/DelegateCombinations.h"
#include "ISaveableSubsystem.h"
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageChangeActive, const FContactsStructure&, Contact);

UCLASS()
class FPSKITALSREFACTORED_API UInstantMessengerSubsystem : public UGameInstanceSubsystem, public ISaveableSubsystem
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

    UFUNCTION(BlueprintCallable, BlueprintPure)
    const TArray<FContactsStructure>& GetContacts() ;

    // Сообщения
    UFUNCTION(BlueprintCallable)
    void AddMessage(const FMessageStructure& NewMessage);

    UFUNCTION(BlueprintCallable)
    const TArray<FMessageStructure>& GetMessages() const;

    UFUNCTION(BlueprintCallable)
    void EditMessage(int32 Index, const FMessageStructure& NewData);

    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("InstantMessengerSubsystem"); }
    virtual bool GetIsLoadComplete() const override { return bIsLoadComplete; }

public:
    UPROPERTY(BlueprintAssignable, Category = "Messenger|Events")
    FOnMessageAdded OnChangeMessages;

    UPROPERTY(BlueprintAssignable, Category = "Messenger|Events")
	FOnMessageChangeActive OnChangeActiveContact;

private:
    UPROPERTY()
    TArray<FContactsStructure> Contacts;

    UPROPERTY()
    TArray<FMessageStructure> Messages;

    bool bIsLoadComplete = false;
};