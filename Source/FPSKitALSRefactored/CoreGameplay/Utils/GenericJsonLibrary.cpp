#include "GenericJsonLibrary.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

FString UGenericJsonLibrary::StructToJsonString(const int32& InStruct)
{
	check(0);
	return FString();
}

DEFINE_FUNCTION(UGenericJsonLibrary::execStructToJsonString)
{
	Stack.StepCompiledIn<FStructProperty>(NULL);
	FStructProperty* StructProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	void* StructPtr = Stack.MostRecentPropertyAddress;

	P_FINISH;
	P_NATIVE_BEGIN;

	FString ResultJson = TEXT("{}");
	if (StructProp)
	{
		ResultJson = StructToJsonStringInternal(StructProp->Struct.Get(), StructPtr);
	}

	*(FString*)RESULT_PARAM = ResultJson;

	P_NATIVE_END;
}

bool UGenericJsonLibrary::JsonStringToStruct(const FString& JsonString, int32& OutStruct)
{
	check(0);
	return false;
}

FString UGenericJsonLibrary::StructToJsonStringInternal(UScriptStruct* StructType, const void* StructPtr)
{
	FString ResultJson = TEXT("{}");

	if (!StructType || !StructPtr)
	{
		return ResultJson;
	}

	FJsonObjectConverter::UStructToJsonObjectString(StructType, StructPtr, ResultJson);

	return ResultJson;
}

bool UGenericJsonLibrary::JsonStringToStructInternal(const FString& JsonString, UScriptStruct* StructType, void* StructPtr)
{
	if (!StructType || !StructPtr || JsonString.IsEmpty())
	{
		return false;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	return FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), StructType, StructPtr, 0, 0, false, nullptr);
}

DEFINE_FUNCTION(UGenericJsonLibrary::execJsonStringToStruct)
{
	P_GET_PROPERTY(FStrProperty, JsonString);

	Stack.StepCompiledIn<FStructProperty>(NULL);
	FStructProperty* StructProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	void* StructPtr = Stack.MostRecentPropertyAddress;

	P_FINISH;
	P_NATIVE_BEGIN;

	bool bSuccess = false;

	if (StructProp)
	{
		bSuccess = JsonStringToStructInternal(JsonString, StructProp->Struct.Get(), StructPtr);
	}

	*(bool*)RESULT_PARAM = bSuccess;

	P_NATIVE_END;
}