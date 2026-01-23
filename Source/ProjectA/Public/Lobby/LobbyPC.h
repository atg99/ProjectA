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

	UPROPERTY(EditAnywhere)
	TSubclassOf<ULobbyWidget> LobbyWidgetClass;

	UPROPERTY()
	ULobbyWidget* LobbyWidgetObject;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSendMessage(const FText& Message); //NetWwork 던지는 코드 생성 : 클라는 이거 호출
	//bool ServerSendMessage_Validate(const FText& Message); //서버에서 확인 
	//void ServerSendMessage_Implementation(const FText& Message); //실제동작 : 실제 서버에서 실행

	UFUNCTION(Client, Reliable)
	void ClientSendMessage(const FText& Message);
	

};
