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
	void ApplyProperty(const FString& PropertyName, const FVariantProperty& Value) override;

	UFUNCTION(BlueprintCallable, Category = "KeysActor")
	void AddPropertyDescription(const FString& PropertyName, const FString& ValueName);

	UFUNCTION(BlueprintCallable, Category = "KeysActor")
	void ParseText(const FText Text);

	FString* FindStrict(TMap<FString, FString>& Map, const FString& Key);

private:
	TArray<FString> ParseCommands(const FText& InputText) const;
	void ParseCommand(const FString& Command, FString& Type, FString& Key, FString& Value) const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keys")
	FName KeyActorName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keys")
	TMap<FString, FString> KeyValues;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keys")
	TMap<FString, FString> KeyTypes;

protected:

private:

};