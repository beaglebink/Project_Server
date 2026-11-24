#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DA_OrderCatalog.generated.h"

USTRUCT(BlueprintType)
struct FCatalogItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OnlineOrdering")
	FName ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OnlineOrdering")
	FSlateBrush ItemPreview;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OnlineOrdering", meta = (ClampMin = 0.0f))
	float ItemPrice;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OnlineOrdering")
	FText Extras;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OnlineOrdering")
	TArray<FSlateBrush> Gallery;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OnlineOrdering")
	FText Description;
};

UCLASS()
class ALSEXTRAS_API UDA_OrderCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OnlineOrders")
	TMap<FGameplayTag, FCatalogItemData> CatalogItems;
};
