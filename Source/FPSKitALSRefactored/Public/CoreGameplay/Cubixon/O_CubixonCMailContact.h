#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Core/CubixonCoreData.h"
#include "O_CubixonCMailContact.generated.h"

UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UO_CubixonCMailContact : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CubixonCMailContact")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CubixonCMailContact")
	FText Mail;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CubixonCMailContact")
	FSlateBrush Avatar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CubixonCMailContact")
	TArray<FCubixonCMailAnswer> Answers{ {FText::FromString("Hello | Hi | Siema"), FText::FromString("Do I know you ? | Who are you ?")} };
};
