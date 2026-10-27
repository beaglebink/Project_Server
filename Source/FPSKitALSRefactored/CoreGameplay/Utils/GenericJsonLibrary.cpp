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
    if (StructProp && StructPtr)
    {
        FJsonObjectConverter::UStructToJsonObjectString(StructProp->Struct, StructPtr, ResultJson);
    }

    *(FString*)RESULT_PARAM = ResultJson;

    P_NATIVE_END;
}

bool UGenericJsonLibrary::JsonStringToStruct(const FString& JsonString, int32& OutStruct)
{
    check(0);
    return false;
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
    if (StructProp && StructPtr && !JsonString.IsEmpty())
    {
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            bSuccess = FJsonObjectConverter::JsonObjectToUStruct(
                JsonObject.ToSharedRef(),
                StructProp->Struct.Get(),
                StructPtr,
                0, 0, false, nullptr
            );
        }
    }

    *(bool*)RESULT_PARAM = bSuccess;

    P_NATIVE_END;
}