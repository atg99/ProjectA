// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/SteamSessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"
#include "Title/NetworkGameInstanceSubsystem.h"

USteamSessionSubsystem::USteamSessionSubsystem()
{
}

void USteamSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

IOnlineSessionPtr USteamSessionSubsystem::GetSessionInterface()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		return Subsystem->GetSessionInterface();
	}
	return nullptr;
}

void USteamSessionSubsystem::CreateGameSession(FString SessionName, int32 MaxPlayers, bool bIsLAN)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid()) return;

	// 이미 세션이 있다면 파괴 (안전장치)
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession)
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}

	// 델리게이트 바인딩
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::OnCreateSessionCompleteCallback));

	// 세션 설정
	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());

	SessionSettings->bIsLANMatch = false;
	SessionSettings->NumPublicConnections = MaxPlayers;
	SessionSettings->bAllowJoinInProgress = true;
	SessionSettings->bAllowJoinViaPresence = true; // 스팀 친구 초대/참가 기능 필수
	SessionSettings->bShouldAdvertise = true;      // 다른 사람이 검색 가능하도록
	SessionSettings->bUsesPresence = true;         // 스팀 로비 기능을 쓰려면 필수!
	SessionSettings->bUseLobbiesIfAvailable = true; // UE 5.x 스팀 로비 사용

	// 커스텀 데이터 설정 (예: 맵 이름)
	SessionSettings->Set(FName("SERVER_NAME"), SessionName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// 세션 생성 시도
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings))
	{
		// 실패 시 바로 델리게이트 해제 및 알림
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		OnCreateSessionComplete.Broadcast(false);
	}
}

void USteamSessionSubsystem::OnCreateSessionCompleteCallback(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}

	OnCreateSessionComplete.Broadcast(bWasSuccessful);

	if (bWasSuccessful)
	{
		UNetworkGameInstanceSubsystem* NetworkGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
		if (NetworkGameInstanceSubsystem)
		{
			const FString Token = NetworkGameInstanceSubsystem->LoginData.token;
			FString Options = FString::Printf(TEXT("?listen?Token=%s"), *Token);
			// 세션 생성 성공 시 로비 레벨(Listen Server)로 이동
			UGameplayStatics::OpenLevel(GetWorld(), "DevLevel", true, Options);
		}
	}
}

void USteamSessionSubsystem::FindGameSessions(bool bIsLAN)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid()) return;

	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::OnFindSessionsCompleteCallback));

	// 검색 설정
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = 10000;
	LastSessionSearch->bIsLanQuery = false;
	// 로비
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		UE_LOG(LogTemp, Error, TEXT("Error : %s Fail"), ANSI_TO_TCHAR(__FUNCTION__));
		OnFindSessionsComplete.Broadcast(TArray<FSearchSessionResult>());
	}
}

void USteamSessionSubsystem::OnFindSessionsCompleteCallback(bool bWasSuccessful)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Error : %s SessionInterface InValid"), ANSI_TO_TCHAR(__FUNCTION__));
	}

	TArray<FSearchSessionResult> BlueprintResults;

	if (LastSessionSearch.IsValid() && LastSessionSearch->SearchResults.Num() > 0)
	{
		for (int32 i = 0; i < LastSessionSearch->SearchResults.Num(); ++i)
		{
			const FOnlineSessionSearchResult& Result = LastSessionSearch->SearchResults[i];

			if (!Result.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("Error : %s Result InValid !!"), ANSI_TO_TCHAR(__FUNCTION__));
				continue;
			}
			FSearchSessionResult BpResult;

			// 세션 설정에서 넣어둔 SERVER_NAME 가져오기
			FString ServerName = "Unknown Server";
			Result.Session.SessionSettings.Get(FName("SERVER_NAME"), ServerName);

			BpResult.ServerName = ServerName;
			BpResult.CurrentPlayers = Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections;
			BpResult.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
			BpResult.Ping = Result.PingInMs;
			BpResult.SearchResultIndex = i; // 나중에 Join할 때 이 인덱스 사용

			BlueprintResults.Add(BpResult);
		}
	}
	else if(!LastSessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Error : %s LastSessionSearch InValid !!"), ANSI_TO_TCHAR(__FUNCTION__));
	}
	else if (LastSessionSearch->SearchResults.Num() < 1)
	{
		UE_LOG(LogTemp, Error, TEXT("Error : %s LastSessionSearch->SearchResults.Num() < 1 !!"), ANSI_TO_TCHAR(__FUNCTION__));
	}

	OnFindSessionsComplete.Broadcast(BlueprintResults);
}

void USteamSessionSubsystem::JoinGameSession(const FSearchSessionResult& SessionResult)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid()) return;

	// 검색 결과가 유효한지 체크
	if (!LastSessionSearch.IsValid() || !LastSessionSearch->SearchResults.IsValidIndex(SessionResult.SearchResultIndex))
	{
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::OnJoinSessionCompleteCallback));

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	// 찾은 결과(SearchResults) 중 해당 인덱스의 세션에 참가 요청
	const FOnlineSessionSearchResult& SearchResult = LastSessionSearch->SearchResults[SessionResult.SearchResultIndex];

	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SearchResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		OnJoinSessionComplete.Broadcast(false);
	}
}

void USteamSessionSubsystem::OnJoinSessionCompleteCallback(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}

	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		// 참가 성공 시 접속할 IP(또는 스팀 주소)를 얻어옴
		FString ConnectString;
		if (SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString))
		{
			// 클라이언트 트래블 (서버로 이동)
			APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
			if (PlayerController)
			{
				UNetworkGameInstanceSubsystem* NetworkGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
				if (NetworkGameInstanceSubsystem)
				{
					const FString Token = NetworkGameInstanceSubsystem->LoginData.token;
					FString FinalURL = FString::Printf(TEXT("%s?Token=%s"), *ConnectString, *Token);
					PlayerController->ClientTravel(FinalURL, ETravelType::TRAVEL_Absolute);
				}
			}
		}
	}
	else
	{
		OnJoinSessionComplete.Broadcast(false);
	}
}
