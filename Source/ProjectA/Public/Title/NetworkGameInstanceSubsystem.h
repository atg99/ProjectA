// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpModule.h"
#include "HAL/Runnable.h"
#include "Sockets.h"
#include "GameFramework/OnlineReplStructs.h"
#include "flatbuffers/flatbuffers.h"
#include "cpp_gen/game_generated.h"

#include "NetworkGameInstanceSubsystem.generated.h"

struct FInventorySaveData;

UENUM(BlueprintType)
enum class EBackendResultType : uint8
{
	None			= 0		UMETA(DisplayName = "None"),
	Login_RES		= 1		UMETA(DisplayName = "Login_RES"),
	Register_RES	= 2		UMETA(DisplayName = "Register_RES"),
	TCPLogin_RES	= 2		UMETA(DisplayName = "TCPLogin_RES"),
};

USTRUCT(BlueprintType)
struct FBackendRequstResult
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EBackendResultType ResultType = EBackendResultType::None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsSuccessful = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Message;
};

USTRUCT(BlueprintType)
struct FBackendLoginData
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FString token;
	UPROPERTY()
	FString message;
};

USTRUCT(BlueprintType)
struct FBackendValidateData
{
	GENERATED_BODY()
public:
	UPROPERTY()
	int uid = -1;

	UPROPERTY()
	FString username;

	UPROPERTY()
	FString message;
};

USTRUCT(BlueprintType)
struct FBackendSaveInvenResult
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FString message;

	UPROPERTY()
	int32 inventory_id = -1;
};

USTRUCT(BlueprintType)
struct FBackendSaveStashResult
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FString message;

	UPROPERTY()
	int32 stash_id = -1;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnChatReceived, FString, Sender, FString, Message, int64, Timestamp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBackendRequstResult, FBackendRequstResult, RequstResult);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnValidateRequstResult, const FBackendValidateData&, ValidateData, int32, Code, const FUniqueNetIdRepl&, RequestUserID);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSaveInvenRequstResult, const FBackendSaveInvenResult&, SaveResult, int32, Code);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLoadInvenRequstResult, const FInventorySaveData&, InventorySaveData, int32, Code, const APlayerController*, InventoryOwner);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSaveStashRequstResult, const FBackendSaveStashResult&, StashSaveResult, int32, Code);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLoadStashRequstResult, const FInventorySaveData&, InventorySaveData, int32, Code, const APlayerController*, StashOwner);

DECLARE_DELEGATE_ThreeParams(FOnNetworkResponse, bool /*bSuccess*/, FString /*Content*/, int32 /*Code*/);

class FTcpSocketWorker : public FRunnable
{
public:
	// 데이터 수신 시 메인 스레드로 넘겨줄 람다 함수 타입
	using FOnBytesReceived = TFunction<void(const TArray<uint8>&)>;

	FTcpSocketWorker(FSocket* InSocket, FOnBytesReceived InCallback);
	virtual ~FTcpSocketWorker();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	void Exit() override;

private:
	FSocket* Socket;
	FOnBytesReceived Callback;
	FRunnableThread* Thread;
	FThreadSafeBool bStopping;
};



/**
 * 
 */
UCLASS()
class PROJECTA_API UNetworkGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	FString UserID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	FString Password;
	
	UFUNCTION(BlueprintCallable)
	void BackendLogin();

	UFUNCTION(BlueprintCallable)
	void BackendRegister(FString NewUserID, FString NewPassword);

	UFUNCTION(BlueprintCallable)
	void ConnectToTCPServer(FString IpAddress, int32 Port);

	UFUNCTION(BlueprintCallable)
	void SendChatMessage(FString Message);

	UFUNCTION(BlueprintCallable)
	void TCPDisconnect();

	UFUNCTION(BlueprintCallable)
	void BackendValidateToken(const FString& Token, const FUniqueNetIdRepl& RequestUserID);

	// DB에 Inventory 저장
	UFUNCTION(BlueprintCallable)
	void SaveInventoryData(FString AuthToken, FString InvenJson);

	UFUNCTION(BlueprintCallable)
	void LoadInventoryData(FString AuthToken, APlayerController* InventoryOwner);

	// Stash
	UFUNCTION(BlueprintCallable)
	void SaveStashData(FString AuthToken, FString InvenJson);

	UFUNCTION(BlueprintCallable)
	void LoadStashData(FString AuthToken, APlayerController* Exiting);

protected:
	void SendRequest_Internal(FString Verb, FString Endpoint, FString JsonBody, FString AuthToken, FOnNetworkResponse Callback);
public:
	//범용
	void SendRequest(FString Verb, FString Endpoint, FString JsonBody, FOnNetworkResponse Callback);
	void SendRequest(FString Verb, FString Endpoint, FString JsonBody, FString AuthToken, FOnNetworkResponse Callback);

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnChatReceived OnChatReceived;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnBackendRequstResult OnRequstResult;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnValidateRequstResult OnValidateRequstResult;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnSaveInvenRequstResult OnSaveInvenRequstResult;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnLoadInvenRequstResult OnLoadInvenRequstResult;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnSaveStashRequstResult OnSaveStashRequstResult;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnLoadStashRequstResult OnLoadStashRequstResult;

	FString BackendIP = TEXT("127.0.0.1");

	FBackendLoginData LoginData;

	void TrySendTCPLoginPacket();

protected:

	FHttpModule* HTTPModule;

	void OnBackendLoginProcessRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bProcessedSuccessfully);

	void OnBackendRegisterProcessRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bProcessedSuccessfully);

	void OnBackendValidateTokenRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bProcessedSuccessfully);

	void SendLoginPacket(FString Token);

	void SendPacket(const uint8* Data, int32 Size);

	void OnDataReceived(const TArray<uint8>& Data);

	void ProcessPacketBuffer();

	void HandlePacket(const uint8* Data, int32 Size);

	FSocket* Socket;

	TSharedPtr<FTcpSocketWorker> SocketWorker;

	TArray<uint8> ReceiveBuffer; 

	bool bIsConnected;

	FString CachedToken;

	const int32 TCPPort = 57776;
};
