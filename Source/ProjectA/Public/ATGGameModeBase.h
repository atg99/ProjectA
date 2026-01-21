// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Title/NetworkGameInstanceSubsystem.h"
#include "ATGGameModeBase.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FBackendValidateResult
{
	GENERATED_BODY()
public:
	UPROPERTY()
	int uid;

	UPROPERTY()
	FString username;

	UPROPERTY()
	FString message;

	UPROPERTY()
	FString token;

	UPROPERTY()
	int32 code;
};

UCLASS()
class PROJECTA_API AATGGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	virtual void Logout(AController* Exiting) override;

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

    virtual void PostLogin(APlayerController* NewPlayer) override;

protected:

	UFUNCTION()
	void OnBackEndValidateComplete(const FBackendValidateData& BackendValidateData, int32 Code, const FUniqueNetIdRepl& RequestUserID);

    void ProcessValidationResult(APlayerController* PC, const FBackendValidateResult& Data);

    TMap<FUniqueNetIdRepl, FBackendValidateResult> EarlyValidationResults;

    FString HostSessionToken;
};
