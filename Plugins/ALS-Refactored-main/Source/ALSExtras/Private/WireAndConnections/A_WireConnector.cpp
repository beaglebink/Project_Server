#include "WireAndConnections/A_WireConnector.h"

AA_WireConnector::AA_WireConnector()
{
	PrimaryActorTick.bCanEverTick = true;
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
		OppositeConnector->OppositeConnector = nullptr;
		OppositeConnector->Destroy();
	}
}

void AA_WireConnector::SetPower(bool OnPower)
{
	bIsOnPower = OnPower;
	OnPowerChanged(bIsOnPower);
}

void AA_WireConnector::OnPowerChanged_Implementation(bool OnPower)
{

}
