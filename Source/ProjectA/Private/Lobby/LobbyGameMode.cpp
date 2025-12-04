// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyGameMode.h"
#include "Lobby/LobbyGameState.h"

ALobbyGameMode::ALobbyGameMode()
{
}

void ALobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	//ErrorMessage 가 비어있지 않으면 접속 불가
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

APlayerController* ALobbyGameMode::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	APlayerController* PC = Super::Login(NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);

	return PC;
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	//GetWorld()->GetPlayerControllerIterator();
}

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->GetTimerManager().SetTimer(LeftTimeHandle,
		FTimerDelegate::CreateLambda([this]() {
			ALobbyGameState* GS = GetGameState<ALobbyGameState>();
			if (GS)
			{
				//Server
				GS->LeftTime--;
				GS->OnRep_LeftTime();
			}
			}),
		1.0f,
		true,
		0.0f
	);
}

void ALobbyGameMode::StartPlay()
{
	Super::StartPlay();
}

void ALobbyGameMode::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}
