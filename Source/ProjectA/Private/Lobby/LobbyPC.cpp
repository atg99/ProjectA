// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyPC.h"
#include "Lobby/LobbyWidget.h"
#include "Utils/NetworkUtil.h"
#include "ATGHUDComponent.h"
#include "GameFramework/HUD.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"

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

				FInputModeGameAndUI InputMode;
				InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
				InputMode.SetHideCursorDuringCapture(false);
				SetInputMode(InputMode);

				SetShowMouseCursor(true);

				//SetInputMode(FInputModeUIOnly());
				if (HasAuthority())
				{
					LobbyWidgetObject->ShowStartBtn();
				}
			}
		}

	}
}

//deprecated
bool ALobbyPC::ServerSendMessage_Validate(const FText& Message)
{
	return true;
}

//deprecated
void ALobbyPC::ServerSendMessage_Implementation(const FText& Message)
{
	for (auto Iter = GetWorld()->GetPlayerControllerIterator(); Iter; ++Iter)
	{
		if (ALobbyPC* PC = Cast<ALobbyPC>(*Iter))
		{
			//NET_LOG(Message.ToString());
			PC->ClientSendMessage(Message);
		}
	}
}

//deprecated
void ALobbyPC::ClientSendMessage_Implementation(const FText& Message)
{
	//NET_LOG(Message.ToString());
	LobbyWidgetObject->AddMessage(Message);
}
