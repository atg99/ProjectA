// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGGameModeBase.h"
#include "ATGPlayerState.h"
#include "Utils/ATGSerializationLibrary.h"
#include "ATGInventoryComponent.h"
#include <Kismet/GameplayStatics.h>
#include "Utils/NetworkUtil.h"
#include "Title/NetworkGameInstanceSubsystem.h"

// listen에서 host는 PreLogin Event발생안함 InitGame에서 토큰저장 PostLogin에서 인증요청
void AATGGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    NET_LOG(TEXT(""));

    // 리슨 서버 호스트가 OpenLevel에 넣은 Token
    HostSessionToken = UGameplayStatics::ParseOption(Options, TEXT("Token"));
}

// 접속 종료시 PS에 저장된 토큰으로 Inventory 저장요청
void AATGGameModeBase::Logout(AController* Exiting)
{
    NET_LOG(TEXT(""));
    UNetworkGameInstanceSubsystem* NetworkGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!NetworkGameInstanceSubsystem)
    {
        NET_LOG(FString::Printf(TEXT("Can't Find NetworkGameInstanceSubsystem")));
        return;
    }

	AATGPlayerState* ATGPlayerState = Exiting->GetPlayerState<AATGPlayerState>();
	if (ATGPlayerState)
	{
		UActorComponent* AC = ATGPlayerState->GetComponentByClass(UATGInventoryComponent::StaticClass());
		if (AC)
		{
			UATGInventoryComponent* InvenComp = Cast<UATGInventoryComponent>(AC);
			if (InvenComp)
			{
				FString GridJson = UATGSerializationLibrary::ConvertGridToJson(InvenComp->GetInventory());
                FString AuthToken = ATGPlayerState->BackendToken;

                NET_LOG(FString::Printf(TEXT("GridJson : %s"), *GridJson));
                NET_LOG(FString::Printf(TEXT("Token : %s"), *AuthToken));

                NetworkGameInstanceSubsystem->SaveInventoryData(AuthToken, GridJson);
			}
		}
	}

    if (Exiting && Exiting->PlayerState)
    {
        FUniqueNetIdRepl NetID = Exiting->PlayerState->GetUniqueId();
        EarlyValidationResults.Remove(NetID);
    }

    Super::Logout(Exiting);
}

void AATGGameModeBase::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    NET_LOG(TEXT(""));
	FString Token = UGameplayStatics::ParseOption(Options, TEXT("Token"));

	if (Token.IsEmpty())
	{
		ErrorMessage = TEXT("No Token Provided");
		//return;
	}
	
	UNetworkGameInstanceSubsystem* NetworkGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
	if (NetworkGameInstanceSubsystem)
	{
		NetworkGameInstanceSubsystem->OnValidateRequstResult.RemoveDynamic(this, &AATGGameModeBase::OnBackendValidateComplete);
		NetworkGameInstanceSubsystem->OnValidateRequstResult.AddDynamic(this, &AATGGameModeBase::OnBackendValidateComplete);

        FBackendValidateResult BackendValidateResult;
        BackendValidateResult.token = Token;
        EarlyValidationResults.Add(UniqueId, BackendValidateResult);

		NetworkGameInstanceSubsystem->BackendValidateToken(Token, UniqueId);
	}

	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

// 호스트는 InitGame에서 저장된 토큰으로 인증요청 클라는 버퍼에 결과가있다면 결과처리
void AATGGameModeBase::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    NET_LOG(TEXT(""));
    if (!NewPlayer || !NewPlayer->PlayerState) return;

    FUniqueNetIdRepl MyNetID = NewPlayer->PlayerState->GetUniqueId();

    UNetworkGameInstanceSubsystem* NetworkGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
    if (!NetworkGameInstanceSubsystem)
    {
        NET_LOG(TEXT("Can't Find NetworkGameInstanceSubsystem"));
    }

    if (NewPlayer->IsLocalController())
    {
        // 호스트는 PreLogin을 안 거쳤으므로 여기서 검증 요청
        if (!HostSessionToken.IsEmpty())
        {
            NetworkGameInstanceSubsystem->OnValidateRequstResult.RemoveDynamic(this, &AATGGameModeBase::OnBackendValidateComplete);
            NetworkGameInstanceSubsystem->OnValidateRequstResult.AddDynamic(this, &AATGGameModeBase::OnBackendValidateComplete);

            FBackendValidateResult BackendValidateResult;
            BackendValidateResult.token = HostSessionToken;
            EarlyValidationResults.Add(MyNetID, BackendValidateResult);

            NetworkGameInstanceSubsystem->BackendValidateToken(HostSessionToken, MyNetID);

            UE_LOG(LogTemp, Log, TEXT("Host validation started in PostLogin."));

        }
        else
        {
            ProcessValidationResult(NewPlayer, FBackendValidateResult());
            UE_LOG(LogTemp, Error, TEXT("Host joined but No Token found in InitGame Options!"));
        }
    }
    else
    {
        // 로그인전에 도착한 결과 확인 (클라)
        if (FBackendValidateResult* BackendValidateResult = EarlyValidationResults.Find(MyNetID))
        {
            //int32 Code = EarlyValidationResults[MyNetID].code;
            //FBackendValidateData Data = EarlyValidationResults.FindRef(MyNetID);

            UE_LOG(LogTemp, Log, TEXT("Found buffered validation result for %s."), *MyNetID.GetUniqueNetId()->ToString());

            ProcessValidationResult(NewPlayer, *BackendValidateResult);
            EarlyValidationResults.Remove(MyNetID);
        }
    }

}

// 백엔드 토큰 인증요청 성공시 RequestUserID로 PC가 생성됬다면 결과처리 생성 전이라면 버퍼에 저장하고 PostLogin에서 처리
void AATGGameModeBase::OnBackendValidateComplete(const FBackendValidateData& BackendValidateData, int32 Code, const FUniqueNetIdRepl& RequestUserID)
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

    if (FBackendValidateResult* BackendValidateResult = EarlyValidationResults.Find(RequestUserID))
    {
        BackendValidateResult->code = Code;
        BackendValidateResult->message = BackendValidateData.message;
        BackendValidateResult->uid = BackendValidateData.uid;
        BackendValidateResult->username = BackendValidateData.username;

        if (TargetPC)
        {
            // 값
            ProcessValidationResult(TargetPC, EarlyValidationResults.FindRef(RequestUserID));
            EarlyValidationResults.Remove(RequestUserID);
        }
        else
        {
            // PC가 없음 (생성 전) -> 버퍼에 저장 나중에 PostLogin에서 처리
            UE_LOG(LogTemp, Log, TEXT("Validation arrived too early for %s. Buffering result."), *RequestUserID.GetUniqueNetId()->ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Can't find FBackendValidateResult"));
    }
}

void AATGGameModeBase::OnBackendLoadInventoryComplete(const FInventorySaveData& InventoryLoadedData, int32 Code, const APlayerController* InventoryOwner)
{
    NET_LOG(FString::Printf(TEXT("Code : %d, entry num : %d"), Code, InventoryLoadedData.saved_entries.Num()));
    for (auto entry : InventoryLoadedData.saved_entries)
    {
        NET_LOG(FString::Printf(TEXT("asset id : %s"), *entry.primary_asset_id));
    }
    if (Code == 200 && InventoryOwner)
    {
        AATGPlayerState* ATGPlayerState = InventoryOwner->GetPlayerState<AATGPlayerState>();
        if (ATGPlayerState)
        {
            UActorComponent* AC = ATGPlayerState->GetComponentByClass(UATGInventoryComponent::StaticClass());
            if (AC)
            {
                UATGInventoryComponent* InvenComp = Cast<UATGInventoryComponent>(AC);
                if (InvenComp)
                {
                    UATGSerializationLibrary::ConvertDataToGrid(InventoryLoadedData, InvenComp->GetInventory());
                }
            }
        }
        else
        {
            NET_LOG(TEXT("ATGPlayerState Null"));
        }
    }
}

bool AATGGameModeBase::ProcessValidationResult(APlayerController* PC, const FBackendValidateResult& Data)
{
    if (Data.code == 200)
    {
        // 성공: PlayerState에 ID 
        if (AATGPlayerState* PS = PC->GetPlayerState<AATGPlayerState>())
        {
            PS->BackendUserID = Data.uid;
            PS->BackendUserName = Data.username;
            PS->BackendToken = Data.token;
            UE_LOG(LogTemp, Log, TEXT("Auth Success for: %d, %s %s Start LoadInventory"), PS->BackendUserID, *PS->BackendUserName, *Data.message);

            UNetworkGameInstanceSubsystem* NetworkGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
            if (NetworkGameInstanceSubsystem)
            {
                NetworkGameInstanceSubsystem->OnLoadInvenRequstResult.RemoveDynamic(this, &AATGGameModeBase::OnBackendLoadInventoryComplete);
                NetworkGameInstanceSubsystem->OnLoadInvenRequstResult.AddDynamic(this, &AATGGameModeBase::OnBackendLoadInventoryComplete);
                NetworkGameInstanceSubsystem->LoadInventoryData(PS->BackendToken, PC);
            }
            return true;
        }
    }
    else
    {
        // 실패: 킥
        UE_LOG(LogTemp, Error, TEXT("Auth Failed! Kicking Player"));
        return false;
        //PC->ClientReturnToMainMenuWithTextReason(FText::FromString("Authentication Failed"));
    }

    return false;
}
