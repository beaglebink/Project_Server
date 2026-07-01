#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaveDataStructures.h"
#include "ISaveableSubsystem.generated.h"

// ??? ISaveableSubsystem ???????????????????????????????????????????????????????
// Интерфейс для подсистем, участвующих в сохранении/загрузке игры.
// GameSaveSubsystem итерируется по зарегистрированным подсистемам,
// вызывает CollectSaveData при сохранении и ApplySaveData при загрузке.
// Подсистема сама знает что сериализовать — GameSaveSubsystem хранит данные "вслепую".
UINTERFACE(MinimalAPI, BlueprintType)
class USaveableSubsystem : public UInterface
{
    GENERATED_BODY()
};

class FPSKITALSREFACTORED_API ISaveableSubsystem
{
    GENERATED_BODY()

public:
    // Собрать данные для сохранения на диск.
    // Подсистема заполняет OutData.SubsystemName и OutData.SerializedData.
    virtual void CollectSaveData(FSubsystemSaveData& OutData) = 0;

    // Применить данные после загрузки с диска.
    // Подсистема читает InData.SerializedData и восстанавливает своё состояние.
    virtual void ApplySaveData(const FSubsystemSaveData& InData) = 0;

	// Метод для очистки временных данных, которые не должны сохраняться между сессиями.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Save")
	void ClearTransientData();

    // Имя подсистемы для идентификации блока в файле сохранения.
    // По умолчанию — имя UClass. Переопределять не обязательно.
    virtual FString GetSaveSubsystemName() const = 0;

    virtual bool GetIsLoadComplete() const = 0;
};
