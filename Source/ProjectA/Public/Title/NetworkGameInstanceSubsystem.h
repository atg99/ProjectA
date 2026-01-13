// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "HttpModule.h"
#include "HAL/Runnable.h"
#include "Sockets.h"

#include "flatbuffers/flatbuffers.h"
#include "cpp_gen/game_generated.h"

#include "NetworkGameInstanceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnChatReceived, FString, Sender, FString, Message, int64, Timestamp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoginResult, bool, bSuccess);

enum class EPacketType : uint8
{
	LOGIN_REQ = 1,
	LOGIN_RES = 2,
	CHAT_MSG = 3
};

USTRUCT(BlueprintType)
struct FLoginData
{
	GENERATED_BODY()
public:
	FString token;
	FString message;
};

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
	void Login();

	UFUNCTION(BlueprintCallable)
	void Register(FString NewUserID, FString NewPassword);

	UFUNCTION(BlueprintCallable)
	void ConnectToGameServer(FString IpAddress, int32 Port, FString Token);

	UFUNCTION(BlueprintCallable)
	void SendChatMessage(FString Message);

	UFUNCTION(BlueprintCallable)
	void TCPDisconnect();

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnChatReceived OnChatReceived;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnLoginResult OnLoginResult;

protected:

	FHttpModule* HTTPModule;

	void OnLoginProcessRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bProcessedSuccessfully);

	void OnRegisterProcessRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bProcessedSuccessfully);

	void SendLoginPacket(FString Token);

	void SendPacket(const uint8* Data, int32 Size);

	void OnDataReceived(const TArray<uint8>& Data);

	void ProcessPacketBuffer();

	void HandlePacket(const uint8* Data, int32 Size);

	FSocket* Socket;

	TSharedPtr<FTcpSocketWorker> SocketWorker;

	TArray<uint8> ReceiveBuffer; 

	bool bIsConnected;
};
