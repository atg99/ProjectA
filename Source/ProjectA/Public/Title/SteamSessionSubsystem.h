// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "SteamSessionSubsystem.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FSearchSessionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString ServerName;

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayers;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers;

	UPROPERTY(BlueprintReadOnly)
	int32 Ping;

	int32 SearchResultIndex;
};

// 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnATGCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnATGFindSessionsComplete, const TArray<FSearchSessionResult>&, Results);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnATGJoinSessionComplete, bool, bWasSuccessful);
UCLASS()
class PROJECTA_API USteamSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	USteamSessionSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// --- Blueprint Callable Functions ---

	UFUNCTION(BlueprintCallable, Category = "Steam Session")
	void CreateGameSession(FString SessionName, int32 MaxPlayers, bool bIsLAN);

	UFUNCTION(BlueprintCallable, Category = "Steam Session")
	void FindGameSessions(bool bIsLAN);

	UFUNCTION(BlueprintCallable, Category = "Steam Session")
	void JoinGameSession(const FSearchSessionResult& SessionResult);

	UPROPERTY(BlueprintAssignable)
	FOnATGCreateSessionComplete OnCreateSessionComplete;

	UPROPERTY(BlueprintAssignable)
	FOnATGFindSessionsComplete OnFindSessionsComplete;

	UPROPERTY(BlueprintAssignable)
	FOnATGJoinSessionComplete OnJoinSessionComplete;

protected:
	void OnCreateSessionCompleteCallback(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsCompleteCallback(bool bWasSuccessful);
	void OnJoinSessionCompleteCallback(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

private:
	IOnlineSessionPtr GetSessionInterface();

	// 검색 설정을 저장해둘 포인터
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	// 델리게이트 핸들
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
};
