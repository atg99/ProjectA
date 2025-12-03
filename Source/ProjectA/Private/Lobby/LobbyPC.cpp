// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyPC.h"
#include "Lobby/LobbyWidget.h"

ALobbyPC::ALobbyPC()
{
}

void ALobbyPC::BeginPlay()
{
	Super::BeginPlay();

	if (LobbyWidgetClass)
	{
		LobbyWidgetObject = CreateWidget<ULobbyWidget>(this, LobbyWidgetClass);
		if (LobbyWidgetObject)
		{
			LobbyWidgetObject->AddToViewport();
		}
	}
}
