#pragma once

#include "CoreMinimal.h"
#include "CubixonCoreData.generated.h"

class UO_CubixonCMailContact;

USTRUCT(BlueprintType)
struct FCubixonFileData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "File")
	USceneComponent* File = nullptr;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "File")
	FText FileName;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "File")
	TSubclassOf<USceneComponent> ComponentClass = nullptr;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "File")
	TArray<uint8> SerializedData;
};

USTRUCT(BlueprintType)
struct FCubixonCMailAnswer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	FText In;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	FText Out;
};

USTRUCT(BlueprintType)
struct FCubixonCMailMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	TSubclassOf<UO_CubixonCMailContact> From;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	TSubclassOf<UO_CubixonCMailContact> To;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	FText Message;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	FText Time;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	FCubixonFileData AttachedFileData;
};

USTRUCT(BlueprintType)
struct FCubixonCMailConversation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	int32 ID;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	FText Subject;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	TArray<FCubixonCMailMessage> Messages;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	TSubclassOf<UO_CubixonCMailContact> DefaultFromContact;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	uint8 bFrom_IsRead : 1{false};

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	uint8 bFrom_Inbox : 1{false};

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	uint8 bFrom_Sent : 1{false};

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	uint8 bFrom_Spam : 1{false};

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	uint8 bFrom_Deleted : 1{false};

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	uint8 bFrom_PermanentlyDeleted: 1{false};

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	TSubclassOf<UO_CubixonCMailContact> DefaultToContact;

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	uint8 bTo_IsRead : 1{false};

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	uint8 bTo_Inbox : 1{false};

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	uint8 bTo_Sent : 1{false};

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	uint8 bTo_Spam : 1{false};

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	uint8 bTo_Deleted : 1{false};

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	uint8 bTo_PermanentlyDeleted : 1{false};
};

USTRUCT(BlueprintType)
struct FCubixonCMailConversationsWrap
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "CMail")
	TArray<FCubixonCMailConversation> Conversations;
};