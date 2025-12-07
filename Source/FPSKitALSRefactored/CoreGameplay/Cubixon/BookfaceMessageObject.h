#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BookfaceMessageObject.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UBookfaceMessageObject : public UObject
{
    GENERATED_BODY()

public:
    // Уникальный идентификатор сообщения
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString MessageId;

    // Автор сообщения
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString FromUserId;

    // Получатель (профиль, к которому относится сообщение)
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString ToUserId;

    // Текст сообщения
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FText MessageContent;

    // Время отправки
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FDateTime Timestamp;

    // Лайки
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FString> LikesUserIds;

    // 🔹 Подписчики на сообщение
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TArray<FString> SubscribedUserIDs;

    // Родительское сообщение (если это ответ)
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Transient)
    UBookfaceMessageObject* ParentMessage;

    // Ответы на сообщение
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Transient)
    TArray<UBookfaceMessageObject*> ReplyMessages;
};