#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_Book.generated.h"

class AC_Word;

UCLASS()
class ALSEXTRAS_API AA_Book : public AActor
{
	GENERATED_BODY()

public:
	AA_Book();

protected:
	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* StaticMeshComponent;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|Property", meta = (AllowPrivateAccess = true))
	int32 BookGroupCode = -1;

	void AddWord(FText NewWord);
};
