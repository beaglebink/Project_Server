#pragma once

#include "CoreMinimal.h"
#include "AlsCharacterExample.h"
#include "C_Word.generated.h"

class ALSEXTRAS_API AC_Word : public AAlsCharacterExample
{
	GENERATED_BODY()
	
public:
	AC_Word();

protected:
	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|Property", meta = (AllowPrivateAccess = true))
	int32 BookGroupCode = -1;

	void Absorbing();
};
