#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PossibleTeleporting_I.generated.h"

UINTERFACE(BlueprintType)
class FPSKITALSREFACTORED_API UPossibleTeleporting_I : public UInterface
{
    GENERATED_BODY()
};

class FPSKITALSREFACTORED_API IPossibleTeleporting_I
{
    GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Teleportation")
	FString GetDestinationID() const;
};
