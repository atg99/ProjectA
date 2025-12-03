// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPC.generated.h"

/**
 * 
 */
class ULobbyWidget;
UCLASS()
class PROJECTA_API ALobbyPC : public APlayerController
{
	GENERATED_BODY()

public:
	ALobbyPC();

	virtual void BeginPlay() override;

	UPROPERTY()
	TSubclassOf<ULobbyWidget> LobbyWidgetClass;

	UPROPERTY()
	ULobbyWidget* LobbyWidgetObject;
	
};
