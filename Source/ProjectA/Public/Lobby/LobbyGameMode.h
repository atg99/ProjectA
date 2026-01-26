// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Title/NetworkGameInstanceSubsystem.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTA_API ALobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALobbyGameMode();

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual APlayerController* Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void Logout(AController* Exiting) override;

	virtual void BeginPlay() override;

	virtual void StartPlay() override;

	virtual void PostInitializeComponents() override;
	
	FTimerHandle LeftTimeHandle;

	UFUNCTION(BlueprintCallable)
	void SaveInventoyData(AController* Controller);

	UFUNCTION(BlueprintCallable)
	void SaveStashData(AController* Controller);

	UFUNCTION(BlueprintCallable)
	void LoadInvenData(APlayerController* Controller);

	UFUNCTION(BlueprintCallable)
	void LoadStashData(APlayerController* Controller);

protected:
	UFUNCTION()
	void OnBackendLoadInventoryComplete(const FInventorySaveData& InventoryLoadedData, int32 Code, const APlayerController* InventoryOwner);

	UFUNCTION()
	void OnBackendLoadStashComplete(const FInventorySaveData& StashLoadedData, int32 Code, const APlayerController* InventoryOwner);

	UFUNCTION()
	void OnSaveInvenResult(const FBackendSaveInvenResult& InvenSaveResult, int32 Code);

	UFUNCTION()
	void OnSaveStashResult(const FBackendSaveStashResult& StashSaveResult, int32 Code);

};
