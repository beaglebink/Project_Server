#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenericJsonLibrary.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UGenericJsonLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Конвертирует любую структуру (Struct) в JSON-строку
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Serialization|Json", meta = (CustomStructureParam = "InStruct", DisplayName = "Struct To Json String"))
	static FString StructToJsonString(const int32& InStruct);

	DECLARE_FUNCTION(execStructToJsonString);

	// Восстанавливает структуру из JSON-строки. Возвращает true, если парсинг прошел успешно.
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Serialization|Json", meta = (CustomStructureParam = "OutStruct", DisplayName = "Json String To Struct"))
	static bool JsonStringToStruct(const FString& JsonString, int32& OutStruct);

	DECLARE_FUNCTION(execJsonStringToStruct);

	// шаблонные обертки для вызова из С++
	template<typename T>
	static FString StructToJsonStringCpp(const T& InStruct)
	{
		return StructToJsonStringInternal(T::StaticStruct(), &InStruct);
	}

	template<typename T>
	static bool JsonStringToStructCpp(const FString& JsonString, T& OutStruct)
	{
		return JsonStringToStructInternal(JsonString, T::StaticStruct(), &OutStruct);
	}

private:
	static FString StructToJsonStringInternal(UScriptStruct* StructType, const void* StructPtr);

	static bool JsonStringToStructInternal(const FString& JsonString, UScriptStruct* StructType, void* StructPtr);
};