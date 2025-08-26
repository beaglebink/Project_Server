#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DictionaryObjectBase.h"
#include "IPropertySupport.h"

#include "KeysActor.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API AKeysActor : public ADictionaryObjectBase, public IPropertySupport
{
	GENERATED_BODY()

public:
	void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "KeysActor")
	void ApplyProperty(const FName PropertyName, const FVariantProperty Value) override;

	UFUNCTION(BlueprintCallable, Category = "KeysActor")
	void AddPropertyDescription(const FName PropertyName, const FName ValueName);

	UFUNCTION(BlueprintCallable, Category = "KeysActor")
	void ParseText(const FText Text);

private:
	TArray<FString> ParseCommands(const FText& InputText) const;
	void ParseCommand(const FString& Command, FName& Type, FName& Key, FName& Value) const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keys")
	FName KeyActorName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keys")
	TMap<FName, FName> KeyValues;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keys")
	TMap<FName, FName> KeyTypes;

protected:

private:

};