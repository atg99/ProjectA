// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby/LobbyGameMode.h"
#include "Lobby/LobbyGameState.h"
#include "Utils/NetworkUtil.h"
#include "Utils/ATGSerializationLibrary.h"
#include "GameFramework/PlayerState.h"
#include "ATGInventoryComponent.h"
#include "Title/NetworkGameInstanceSubsystem.h"
#include "ATGContainerComponent.h"

ALobbyGameMode::ALobbyGameMode()
{
}

void ALobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	//ErrorMessage 가 비어있지 않으면 접속 불가
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

APlayerController* ALobbyGameMode::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	APlayerController* PC = Super::Login(NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);

	return PC;
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	LoadInvenData(NewPlayer);
	LoadStashData(NewPlayer);
}

void ALobbyGameMode::Logout(AController* Exiting)
{
	SaveInventoyData(Exiting);
	SaveStashData(Exiting);

	Super::Logout(Exiting);
}



void ALobbyGameMode::OnBackendLoadInventoryComplete(const FInventorySaveData& InventoryLoadedData, int32 Code, const APlayerController* InventoryOwner)
{
	NET_LOG(FString::Printf(TEXT("Code : %d, entry num : %d"), Code, InventoryLoadedData.saved_entries.Num()));
	for (auto entry : InventoryLoadedData.saved_entries)
	{
		NET_LOG(FString::Printf(TEXT("asset id : %s"), *entry.primary_asset_id));
	}
	if (Code == 200 && InventoryOwner)
	{
		APlayerState* PS = InventoryOwner->GetPlayerState<APlayerState>();
		if (PS)
		{
			UActorComponent* AC = PS->GetComponentByClass(UATGInventoryComponent::StaticClass());
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

void ALobbyGameMode::OnBackendLoadStashComplete(const FInventorySaveData& StashLoadedData, int32 Code, const APlayerController* InventoryOwner)
{
	NET_LOG(FString::Printf(TEXT("Code : %d, entry num : %d"), Code, StashLoadedData.saved_entries.Num()));
	for (auto entry : StashLoadedData.saved_entries)
	{
		NET_LOG(FString::Printf(TEXT("asset id : %s"), *entry.primary_asset_id));
	}
	if (Code == 200 && InventoryOwner)
	{
		APlayerState* PS = InventoryOwner->GetPlayerState<APlayerState>();
		if (PS)
		{
			UActorComponent* AC = PS->GetComponentByClass(UATGContainerComponent::StaticClass());
			if (AC)
			{
				UATGContainerComponent* ContainerComp = Cast<UATGContainerComponent>(AC);
				if (ContainerComp)
				{
					UATGSerializationLibrary::ConvertDataToGrid(StashLoadedData, ContainerComp->GetContainerInventory());
				}
			}
		}
		else
		{
			NET_LOG(TEXT("ATGPlayerState Null"));
		}
	}
}

void ALobbyGameMode::SaveInventoyData(AController* Controller)
{
	NET_LOG(TEXT(""));
	UNetworkGameInstanceSubsystem* NetworkGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
	if (!NetworkGameInstanceSubsystem)
	{
		NET_LOG(FString::Printf(TEXT("Can't Find NetworkGameInstanceSubsystem")));
		return;
	}



	APlayerState* PlayerState = Controller->GetPlayerState<APlayerState>();
	if (PlayerState)
	{
		UActorComponent* AC = PlayerState->GetComponentByClass(UATGInventoryComponent::StaticClass());
		if (AC)
		{
			UATGInventoryComponent* InvenComp = Cast<UATGInventoryComponent>(AC);
			if (InvenComp)
			{
				FString GridJson = UATGSerializationLibrary::ConvertGridToJson(InvenComp->GetInventory());
				FString AuthToken = NetworkGameInstanceSubsystem->LoginData.token;

				NET_LOG(FString::Printf(TEXT("GridJson : %s"), *GridJson));
				NET_LOG(FString::Printf(TEXT("Token : %s"), *AuthToken));

				UWorld* World = GetWorld();
				// 월드가 파괴 중이거나 에디터 종료 중이면 저장하지 않고 리턴
				if (World && !World->bIsTearingDown && !GIsRequestingExit)
				{
					NetworkGameInstanceSubsystem->OnSaveInvenRequstResult.RemoveDynamic(this, &ALobbyGameMode::OnSaveInvenResult);
					NetworkGameInstanceSubsystem->OnSaveInvenRequstResult.AddDynamic(this, &ALobbyGameMode::OnSaveInvenResult);
				}

				NetworkGameInstanceSubsystem->SaveInventoryData(AuthToken, GridJson);
			}
		}
	}
}

void ALobbyGameMode::SaveStashData(AController* Controller)
{
	NET_LOG(TEXT(""));
	UNetworkGameInstanceSubsystem* NetworkGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
	if (!NetworkGameInstanceSubsystem)
	{
		NET_LOG(FString::Printf(TEXT("Can't Find NetworkGameInstanceSubsystem")));
		return;
	}

	APlayerState* PlayerState = Controller->GetPlayerState<APlayerState>();
	if (PlayerState)
	{
		UActorComponent* AC = PlayerState->GetComponentByClass(UATGContainerComponent::StaticClass());
		if (AC)
		{
			UATGContainerComponent* StashComp = Cast<UATGContainerComponent>(AC);
			if (StashComp)
			{
				FString GridJson = UATGSerializationLibrary::ConvertGridToJson(StashComp->GetContainerInventory());
				FString AuthToken = NetworkGameInstanceSubsystem->LoginData.token;

				NET_LOG(FString::Printf(TEXT("GridJson : %s"), *GridJson));
				NET_LOG(FString::Printf(TEXT("Token : %s"), *AuthToken));

				UWorld* World = GetWorld();
				// 월드가 파괴 중이거나 에디터 종료 중이면 저장하지 않고 리턴
				if (World && !World->bIsTearingDown && !GIsRequestingExit)
				{
					NetworkGameInstanceSubsystem->OnSaveStashRequstResult.RemoveDynamic(this, &ALobbyGameMode::OnSaveStashResult);
					NetworkGameInstanceSubsystem->OnSaveStashRequstResult.AddDynamic(this, &ALobbyGameMode::OnSaveStashResult);

				}
				NetworkGameInstanceSubsystem->SaveStashData(AuthToken, GridJson);
			}
		}
	}
}

void ALobbyGameMode::LoadInvenData(APlayerController* Controller)
{
	UNetworkGameInstanceSubsystem* NetworkGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
	if (NetworkGameInstanceSubsystem)
	{
		NetworkGameInstanceSubsystem->OnLoadInvenRequstResult.RemoveDynamic(this, &ALobbyGameMode::OnBackendLoadInventoryComplete);
		NetworkGameInstanceSubsystem->OnLoadInvenRequstResult.AddDynamic(this, &ALobbyGameMode::OnBackendLoadInventoryComplete);
		NetworkGameInstanceSubsystem->LoadInventoryData(NetworkGameInstanceSubsystem->LoginData.token, Controller);
	}
}

void ALobbyGameMode::LoadStashData(APlayerController* Controller)
{
	UNetworkGameInstanceSubsystem* NetworkGameInstanceSubsystem = GetGameInstance()->GetSubsystem<UNetworkGameInstanceSubsystem>();
	if (NetworkGameInstanceSubsystem)
	{
		NetworkGameInstanceSubsystem->OnLoadStashRequstResult.RemoveDynamic(this, &ALobbyGameMode::OnBackendLoadStashComplete);
		NetworkGameInstanceSubsystem->OnLoadStashRequstResult.AddDynamic(this, &ALobbyGameMode::OnBackendLoadStashComplete);
		NetworkGameInstanceSubsystem->LoadStashData(NetworkGameInstanceSubsystem->LoginData.token, Controller);
	}
}

void ALobbyGameMode::OnSaveInvenResult(const FBackendSaveInvenResult& InvenSaveResult, int32 Code)
{
	// 실패시 텍스트로 저장
}

void ALobbyGameMode::OnSaveStashResult(const FBackendSaveStashResult& StashSaveResult, int32 Code)
{
}

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->GetTimerManager().SetTimer(LeftTimeHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]() {
			ALobbyGameState* GS = GetGameState<ALobbyGameState>();
			if (GS)
			{
				//Server
				GS->LeftTime--;
				GS->OnRep_LeftTime();
			}
			}),
		1.0f,
		true,
		0.0f
	);
}

void ALobbyGameMode::StartPlay()
{
	Super::StartPlay();
}

void ALobbyGameMode::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}
