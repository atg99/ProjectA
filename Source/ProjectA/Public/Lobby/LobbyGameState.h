// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LobbyGameState.generated.h"

/**
 * 
 */
//함수를 등록할수있는 자료형?
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyState, int32, InIntVal);

UCLASS()
class PROJECTA_API ALobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = "OnRep_LeftTime", EditAnywhere, BlueprintReadOnly, Category = "Data")
	int32 LeftTime = 60;

	UFUNCTION()
	void OnRep_LeftTime();

	FOnLobbyState OnLeftTime;
	FOnLobbyState OnPlayerNum;

	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
};
