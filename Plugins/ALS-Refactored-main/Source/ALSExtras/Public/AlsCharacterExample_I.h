#pragma once  

#include "CoreMinimal.h"  
#include "UObject/Interface.h"  
#include "AlsCharacterExample_I.generated.h"  

UINTERFACE(Blueprintable)  
class ALSEXTRAS_API UAlsCharacter_I : public UInterface  
{  
    GENERATED_BODY()  
};  

class ALSEXTRAS_API IAlsCharacter_I  
{  
    GENERATED_BODY()  

public:  
    /** Returns the NetGrenadeParalyseTime value */  
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Als Character Interface")  
    float GetNetGrenadeParalyseTime() const;

    //UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Als Character Interface")
	//virtual void ParalyzeNPC(float Time) = 0;
};