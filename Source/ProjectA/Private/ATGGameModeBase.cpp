// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGGameModeBase.h"
#include "ATGPlayerState.h"
#include "Utils/ATGSerializationLibrary.h"
#include "ATGInventoryComponent.h"
#include <Kismet/GameplayStatics.h>
#include "Title/NetworkGameInstanceSubsystem.h"

void AATGGameModeBase::Logout(AController* Exiting)
{
	AATGPlayerState* ATGPlayerState = Exiting->GetPlayerState<AATGPlayerState>();
	if (ATGPlayerState)
	{
		UActorComponent* AC = ATGPlayerState->GetComponentByClass(UATGInventoryComponent::StaticClass());
		if (AC)
		{
			UATGInventoryComponent* InvenComp = Cast<UATGInventoryComponent>(AC);
			if (InvenComp)
			{
				UATGSerializationLibrary::ConvertGridToJson(InvenComp->GetInventory());
			}
		}
	}


    if (Exiting && Exiting->PlayerState)
    {
        FUniqueNetIdRepl NetID = Exiting->PlayerState->GetUniqueId();
        EarlyValidationResults.Remove(NetID);
        EarlyValidationCodes.Remove(NetID);
    }

    Super::Logout(Exiting);
}

void AATGGameModeBase::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	FString Token = UGameplayStatics::ParseOption(Options, TEXT("Token"));

	if (Token.IsEmpty())
	{
		ErrorMessage = TEXT("No Token Provided");
		return;
	}
	
	UNetworkGameInstanceSubsystem* NetworkGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
	if (NetworkGameInstanceSubsystem)
	{
		NetworkGameInstanceSubsystem->OnValidateRequstResult.RemoveDynamic(this, &AATGGameModeBase::OnBackEndValidateComplete);
		NetworkGameInstanceSubsystem->OnValidateRequstResult.AddDynamic(this, &AATGGameModeBase::OnBackEndValidateComplete);
		NetworkGameInstanceSubsystem->BackendValidateToken(Token, UniqueId);
	}

	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

void AATGGameModeBase::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!NewPlayer || !NewPlayer->PlayerState) return;

    FUniqueNetIdRepl MyNetID = NewPlayer->PlayerState->GetUniqueId();

    // 로그인전에 도착한 결과 확인
    if (EarlyValidationCodes.Contains(MyNetID))
    {
        int32 Code = EarlyValidationCodes[MyNetID];
        FBackendValidateData Data = EarlyValidationResults.FindRef(MyNetID);

        UE_LOG(LogTemp, Log, TEXT("Found buffered validation result for %s."), *MyNetID.GetUniqueNetId()->ToString());

        ProcessValidationResult(NewPlayer, Data, Code);

        // 버퍼에서 삭제
        EarlyValidationResults.Remove(MyNetID);
        EarlyValidationCodes.Remove(MyNetID);
    }
}

void AATGGameModeBase::OnBackEndValidateComplete(const FBackendValidateData& BackendValidateData, int32 Code, const FUniqueNetIdRepl& RequestUserID)
{
    bool bIsSuccess = (Code == 200);

    // RequestUserID를 가진 플레이어 찾기
    APlayerController* TargetPC = nullptr;
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        APlayerController* PC = Iterator->Get();

        if (PC && PC->PlayerState && PC->PlayerState->GetUniqueId() == RequestUserID)
        {
            TargetPC = PC;
            break;
        }
    }

    if (TargetPC)
    {
        ProcessValidationResult(TargetPC, BackendValidateData, Code);
    }
    else
    {
        // PC가 없음 (생성 전) -> 버퍼에 저장 나중에 PostLogin에서 처리
        UE_LOG(LogTemp, Log, TEXT("Validation arrived too early for %s. Buffering result."), *RequestUserID.GetUniqueNetId()->ToString());

        EarlyValidationResults.Add(RequestUserID, BackendValidateData);
        EarlyValidationCodes.Add(RequestUserID, Code);
    }
}

void AATGGameModeBase::ProcessValidationResult(APlayerController* PC, const FBackendValidateData& Data, int32 Code)
{
    if (Code == 200)
    {
        // 성공: PlayerState에 ID 
        if (AATGPlayerState* PS = PC->GetPlayerState<AATGPlayerState>())
        {
            PS->BackendUserID = Data.uid;
            PS->BackendUserName = Data.username;
            UE_LOG(LogTemp, Log, TEXT("Auth Success for: %d, %s"), PS->BackendUserID, *PS->BackendUserName);
        }
    }
    else
    {
        // 실패: 킥
        UE_LOG(LogTemp, Warning, TEXT("Auth Failed! Kicking Player"));
        PC->ClientReturnToMainMenuWithTextReason(FText::FromString("Authentication Failed"));
    }
}
