#include "WireAndConnections/A_WireConnector.h"
#include "ItemPlacement/A_DropZone.h"

AA_WireConnector::AA_WireConnector()
{
	PrimaryActorTick.bCanEverTick = true;

	StaticMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Ignore);
}

void AA_WireConnector::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AA_WireConnector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_WireConnector::BeginPlay()
{
	Super::BeginPlay();
}

void AA_WireConnector::Destroyed()
{
	Super::Destroyed();

	if (IsValid(OppositeConnector))
	{
		if (IsValid(OppositeConnection))
		{
			OppositeConnector->OppositeConnection = nullptr;
			OppositeConnection->Destroy();
		}

		OppositeConnector->OppositeConnector = nullptr;
		OppositeConnector->Destroy();
	}
}

void AA_WireConnector::OnPowerConnected_Implementation(bool IsConnected)
{
	if (bIsOnPower == IsConnected)
	{
		return;
	}

	bIsOnPower = IsConnected;

	if (IsValid(OppositeConnector) && IsValid(OppositeConnection))
	{
		OppositeConnector->bIsOnPower = bIsOnPower;
		if (OppositeConnector->AttachingDropZone)
		{
			if (OppositeConnector->AttachingDropZone->Implements<UI_PowerConnection>())
			{
				II_PowerConnection::Execute_OnPowerConnected(OppositeConnector->AttachingDropZone, bIsOnPower);
			}
		}
		OppositeConnection->bIsPowerOn = bIsOnPower;
		if (OppositeConnection->Implements<UI_PowerConnection>())
		{
			II_PowerConnection::Execute_OnPowerConnected(OppositeConnection, bIsOnPower);
		}
	}
}