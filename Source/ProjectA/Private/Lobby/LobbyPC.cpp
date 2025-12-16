// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyPC.h"
#include "Lobby/LobbyWidget.h"
#include "Utils/NetworkUtil.h"

ALobbyPC::ALobbyPC()
{
}

void ALobbyPC::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		if (LobbyWidgetClass)
		{
			LobbyWidgetObject = CreateWidget<ULobbyWidget>(this, LobbyWidgetClass);
			if (LobbyWidgetObject)
			{
				LobbyWidgetObject->AddToViewport();

				bShowMouseCursor = true;
				SetInputMode(FInputModeUIOnly());
				if (HasAuthority())
				{
					LobbyWidgetObject->ShowStartBtn();
				}
			}
		}
	}
}

bool ALobbyPC::ServerSendMessage_Validate(const FText& Message)
{
	return true;
}

void ALobbyPC::ServerSendMessage_Implementation(const FText& Message)
{
	for (auto Iter = GetWorld()->GetPlayerControllerIterator(); Iter; ++Iter)
	{
		if (ALobbyPC* PC = Cast<ALobbyPC>(*Iter))
		{
			NET_LOG(Message.ToString());
			PC->ClientSendMessage(Message);
		}
	}
}

void ALobbyPC::ClientSendMessage_Implementation(const FText& Message)
{
	NET_LOG(Message.ToString());
	LobbyWidgetObject->AddMessage(Message);
}
