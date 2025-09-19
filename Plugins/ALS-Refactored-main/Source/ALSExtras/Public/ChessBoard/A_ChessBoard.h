#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_ChessBoard.generated.h"

class UBoxComponent;
class AAlsCharacterExample;

USTRUCT(BlueprintType)
struct FCell
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UStaticMeshComponent* MeshComponent;

	UPROPERTY()
	UBoxComponent* Trigger;

	UPROPERTY()
	uint8 bIsBlack : 1{false};
};

UCLASS()
class ALSEXTRAS_API AA_ChessBoard : public AActor
{
	GENERATED_BODY()

public:
	AA_ChessBoard();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "10"))
	int32 Rows;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "10"))
	int32 Columns;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field", meta = (AllowPrivateAccess = "true", ClampMin = "0", ClampMax = "50"))
	float CellDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field", meta = (AllowPrivateAccess = "true"))
	UStaticMesh* CellMesh;

	UPROPERTY()
	TArray<FCell> ChessField;

	TMap<AAlsCharacterExample*, TSet<FCell*>> CharactersToCells;

	void BuildField();

	UFUNCTION()
	void OnCellBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnCellEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void DoCharacterDamage();
};
