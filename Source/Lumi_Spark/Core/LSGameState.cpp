#include "Core/LSGameState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

ALSGameState::ALSGameState()
{
	bReplicates = true;
}

void ALSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALSGameState, ConnectedPlayerCount);
	DOREPLIFETIME(ALSGameState, bIsRaidWiped);
}

float ALSGameState::GetDynamicHealthMultiplier() const
{
	const int32 Index = FMath::Clamp(ConnectedPlayerCount - 1, 0, HealthScaleTable.Num() - 1);
	return HealthScaleTable.IsValidIndex(Index) ? HealthScaleTable[Index] : 1.0f;
}

float ALSGameState::GetDynamicShieldMultiplier() const
{
	const int32 Index = FMath::Clamp(ConnectedPlayerCount - 1, 0, ShieldScaleTable.Num() - 1);
	return ShieldScaleTable.IsValidIndex(Index) ? ShieldScaleTable[Index] : 1.0f;
}

float ALSGameState::GetCurrentHealthMultiplier(const UObject* WorldContextObject)
{
	if (!WorldContextObject) return 1.0f;
	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (const ALSGameState* GS = World->GetGameState<ALSGameState>())
		{
			return GS->GetDynamicHealthMultiplier();
		}
	}
	return 1.0f;
}

void ALSGameState::SetConnectedPlayerCount(int32 NewCount)
{
	if (HasAuthority())
	{
		ConnectedPlayerCount = FMath::Max(1, NewCount);
		OnPlayerCountChanged.Broadcast(ConnectedPlayerCount);
	}
}

void ALSGameState::SetRaidWiped(bool bWiped)
{
	if (HasAuthority())
	{
		bIsRaidWiped = bWiped;
		OnRaidWipeStateChanged.Broadcast(bIsRaidWiped);
	}
}

void ALSGameState::OnRep_ConnectedPlayerCount()
{
	OnPlayerCountChanged.Broadcast(ConnectedPlayerCount);
}

void ALSGameState::OnRep_IsRaidWiped()
{
	OnRaidWipeStateChanged.Broadcast(bIsRaidWiped);
}