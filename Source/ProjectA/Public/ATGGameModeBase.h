// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Title/NetworkGameInstanceSubsystem.h"
#include "ATGGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTA_API AATGGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:

	virtual void Logout(AController* Exiting) override;

	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;

    virtual void PostLogin(APlayerController* NewPlayer) override;

protected:

	UFUNCTION()
	void OnBackEndValidateComplete(const FBackendValidateData& BackendValidateData, int32 Code, const FUniqueNetIdRepl& RequestUserID);

    void ProcessValidationResult(APlayerController* PC, const FBackendValidateData& Data, int32 Code);

    // [버퍼] 아직 접속 처리가 안 끝난 유저의 검증 결과 임시 저장소
    // Key: NetID, Value: 검증 데이터
    TMap<FUniqueNetIdRepl, FBackendValidateData> EarlyValidationResults;

    // 검증 실패 시 킥을 해야 하므로 코드도 저장 필요하면 구조체를 따로 만들어도 됨
    // 여기서는 편의상 성공한 경우의 데이터만 저장한다고 가정하거나, 
    // 실패(Code != 200)인 경우도 별도 Set으로 관리 가능.
    TMap<FUniqueNetIdRepl, int32> EarlyValidationCodes; // 상태 코드 저장용
};
