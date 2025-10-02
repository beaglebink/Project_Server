#pragma once

#include "Interfaces/I_PortalInteraction.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_InteractableActor.generated.h"

UCLASS()
class ALSEXTRAS_API AA_InteractableActor : public AActor, public II_PortalInteraction
{
	GENERATED_BODY()
	
public:	
	AA_InteractableActor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Name")
	FName Name;

    UFUNCTION(BlueprintCallable, Category = "TextParsing")
	bool ParseAssignCommand(FText Command, FName& OutVarName, FName& OutActorName);

	virtual void PortalInteract_Implementation(const FHitResult& Hit, const FTransform& EnterTransform, const FTransform& ExitTransform) override;
};
