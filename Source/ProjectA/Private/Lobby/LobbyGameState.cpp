// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyGameState.h"
#include "Net/UnrealNetwork.h"

void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyGameState, LeftTime);
}

void ALobbyGameState::OnRep_LeftTime()
{
	OnLeftTime.Broadcast(LeftTime);
}

void ALobbyGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	OnPlayerNum.Broadcast(PlayerArray.Num());
}

void ALobbyGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	OnPlayerNum.Broadcast(PlayerArray.Num());
}
