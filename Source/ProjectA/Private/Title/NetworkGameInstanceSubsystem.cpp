// Fill out your copyright notice in the Description page of Project Settings.


#include "Title/NetworkGameInstanceSubsystem.h"
#include "JsonUtilities.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/NetworkUtil.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Common/TcpSocketBuilder.h"
#include "Async/Async.h"
#include "InventoryTypes.h"
//namespace GamePacket;

FTcpSocketWorker::FTcpSocketWorker(FSocket* InSocket, FOnBytesReceived InCallback)
	: Socket(InSocket), Callback(InCallback), Thread(nullptr), bStopping(false)
{
	Thread = FRunnableThread::Create(this, TEXT("TcpSocketWorker"), 0, TPri_BelowNormal);
}

FTcpSocketWorker::~FTcpSocketWorker()
{
	if (Thread)
	{
		Stop();
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}
}

bool FTcpSocketWorker::Init() { return true; }

uint32 FTcpSocketWorker::Run()
{
	while (!bStopping && Socket)
	{
		if (Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(0.1)))
		{
			//os에 대기중인 버퍼가 있는지
			uint32 PendingDataSize = 0;
			if (Socket->HasPendingData(PendingDataSize) && PendingDataSize > 0)
			{
				TArray<uint8> Buffer;
				Buffer.SetNumUninitialized(PendingDataSize);

				int32 BytesRead = 0;
				if (Socket->Recv(Buffer.GetData(), PendingDataSize, BytesRead))
				{
					AsyncTask(ENamedThreads::GameThread, [this, Buffer]()
						{
							if (Callback) Callback(Buffer);
						});
				}
			}
		}
	}
	return 0;
}

void FTcpSocketWorker::Stop() { bStopping = true; }
void FTcpSocketWorker::Exit() {}

void UNetworkGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	HTTPModule = &FHttpModule::Get();
	Socket = nullptr;
	bIsConnected = false;
	UE_LOG(LogTemp, Log, TEXT("NetworkSubsystem Initialized"));
}

void UNetworkGameInstanceSubsystem::Deinitialize()
{
	TCPDisconnect();
	UE_LOG(LogTemp, Log, TEXT("NetworkSubsystem Deinitialized"));
	Super::Deinitialize();
}

void UNetworkGameInstanceSubsystem::BackendLogin()
{
	auto Request = HTTPModule->CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(
		this,
		&UNetworkGameInstanceSubsystem::OnBackendLoginProcessRequestComplete
	);

	FString URL = FString::Printf(TEXT("http://%s:3000/api/v1/auth/login"), *BackendIP);

	Request->SetURL(URL);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("username"), UserID);
	JsonObject->SetStringField(TEXT("password"), Password);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);

	Request->ProcessRequest();
}

void UNetworkGameInstanceSubsystem::BackendRegister(FString NewUserID, FString NewPassword)
{
	auto Request = HTTPModule->CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &UNetworkGameInstanceSubsystem::OnBackendRegisterProcessRequestComplete);

	FString URL = FString::Printf(TEXT("http://%s:3000/api/v1/auth/register"), *BackendIP);

	Request->SetURL(URL);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("username"), NewUserID);
	JsonObject->SetStringField(TEXT("password"), NewPassword);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);

	Request->ProcessRequest();
}

void UNetworkGameInstanceSubsystem::OnBackendLoginProcessRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bProcessedSuccessfully)
{
	if (!bProcessedSuccessfully || !Response.IsValid())
	{
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	UE_LOG(LogTemp, Warning, TEXT("LoginResponseCode %d"), StatusCode);

	FString ResponseContent = Response->GetContentAsString();

	if (FJsonObjectConverter::JsonObjectStringToUStruct<FBackendLoginData>(ResponseContent, &LoginData, 0, 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("Token : %s, Login message : %s"), *LoginData.token ,*LoginData.message);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("JsonObjectStringToUStruct Fail"));
	}

	if (StatusCode == 200)
	{
		UE_LOG(LogTemp, Warning, TEXT("Login Success OpenLobbyLevel"));
		
		CachedToken = LoginData.token;

		FBackendRequstResult RES;
		RES.ResultType = EBackendResultType::Login_RES;
		RES.bIsSuccessful = true;
		RES.Message = LoginData.message;
		OnRequstResult.Broadcast(RES);

		ConnectToTCPServer(BackendIP, TCPPort);

		UGameplayStatics::OpenLevel(GetWorld(), TEXT("LobbyMap"), true);
	}
	else
	{
		FBackendRequstResult RES;
		RES.ResultType = EBackendResultType::Login_RES;
		RES.bIsSuccessful = false;
		RES.Message = LoginData.message;
		OnRequstResult.Broadcast(RES);
		UE_LOG(LogTemp, Warning, TEXT("Login Failed : code: %d, message : %s "), StatusCode, *LoginData.message);
	}

}

void UNetworkGameInstanceSubsystem::OnBackendRegisterProcessRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bProcessedSuccessfully)
{
	if (!bProcessedSuccessfully || !Response.IsValid())
	{
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	UE_LOG(LogTemp, Warning, TEXT("RegisterResponseCode %d"), StatusCode);

	FString ResponseContent = Response->GetContentAsString();
	FBackendLoginData RegisterData;

	if (FJsonObjectConverter::JsonObjectStringToUStruct<FBackendLoginData>(ResponseContent, &RegisterData, 0, 0))
	{
		UE_LOG(LogTemp, Warning, TEXT("Register message : %s"), *RegisterData.message);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Register JsonObjectStringToUStruct Fail"));
	}

	if (StatusCode == 201)
	{
		UE_LOG(LogTemp, Warning, TEXT("Register Success"));

		FBackendRequstResult RES;
		RES.ResultType = EBackendResultType::Register_RES;
		RES.bIsSuccessful = true;
		RES.Message = RegisterData.message;
		OnRequstResult.Broadcast(RES);
	}
	else
	{
		FBackendRequstResult RES;
		RES.ResultType = EBackendResultType::Register_RES;
		RES.bIsSuccessful = false;
		RES.Message = RegisterData.message;
		OnRequstResult.Broadcast(RES);
		UE_LOG(LogTemp, Warning, TEXT("Register Failed : code: %d, message : %s "), StatusCode, *RegisterData.message);
	}
}

void UNetworkGameInstanceSubsystem::ConnectToTCPServer(FString IpAddress, int32 Port)
{
	TCPDisconnect(); // 기존 연결 정리

	//IP 파싱 union{uint32 A.B.C.D, uint32}
	FIPv4Address IP;
	if (!FIPv4Address::Parse(IpAddress, IP))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid IP Address: %s"), *IpAddress);
		return;
	}

	//FInternetAddr 에 IP, Port Setting
	TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	Addr->SetIp(IP.Value);
	Addr->SetPort(Port);

	Socket = FTcpSocketBuilder(TEXT("GameClientSocket")).AsReusable().BoundToPort(0).Build();

	if (Socket && Socket->Connect(*Addr))
	{
		bIsConnected = true;
		UE_LOG(LogTemp, Log, TEXT("Connected to Server: %s:%d"), *IpAddress, Port);

		TWeakObjectPtr<UNetworkGameInstanceSubsystem> WeakThis(this);

		// 스레드 시작 (람다로 콜백 연결)
		SocketWorker = MakeShareable(new FTcpSocketWorker(Socket,
			[WeakThis](const TArray<uint8>& Data)
			{
				if (UNetworkGameInstanceSubsystem* EnsureThis = WeakThis.Get())
				{
					EnsureThis->OnDataReceived(Data);
				}
			}));

		// 연결 즉시 로그인 패킷 전송
		SendLoginPacket(CachedToken);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to connect to server."));
	}
}

void UNetworkGameInstanceSubsystem::TCPDisconnect()
{
	// 1. 스레드 정지
	if (SocketWorker.IsValid())
	{
		SocketWorker->Stop();
		SocketWorker.Reset(); // 소멸자에서 스레드 Join 대기
	}

	// 2. 소켓 닫기
	if (Socket)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}

	bIsConnected = false;
	ReceiveBuffer.Empty();
}

// [Send 로직]

void UNetworkGameInstanceSubsystem::SendLoginPacket(FString Token)
{
	if (!bIsConnected) return;

	flatbuffers::FlatBufferBuilder builder(1024);

	// 1. 내부 데이터(LoginReqPacket) 생성
	auto tokenOffset = builder.CreateString(TCHAR_TO_UTF8(*Token));

	// Builder 객체를 써도 되지만, Create 함수가 더 간결합니다.
	auto loginReqOffset = GamePacket::CreateLoginReqPacket(builder, tokenOffset);

	// 2. 최상위 루트(GameMessage) 생성
	// Union 타입과 오프셋을 전달해야 합니다.
	auto gameMessage = GamePacket::CreateGameMessage(builder,
		GamePacket::PacketData_LoginReqPacket,
		loginReqOffset.Union());

	// 3. 빌더 종료 (최상위 루트 지정)
	builder.Finish(gameMessage);

	// 4. 전송 (Type 인자 제거)
	SendPacket(builder.GetBufferPointer(), builder.GetSize());
}

void UNetworkGameInstanceSubsystem::SendChatMessage(FString Message)
{
	if (!bIsConnected) return;

	flatbuffers::FlatBufferBuilder builder(1024);

	// 1. 내부 데이터(ChatPacket) 생성
	auto msgOffset = builder.CreateString(TCHAR_TO_UTF8(*Message));
	auto senderOffset = builder.CreateString("me");
	int64 UnixTime = FDateTime::UtcNow().ToUnixTimestamp() * 1000;

	auto chatOffset = GamePacket::CreateChatPacket(builder, senderOffset, msgOffset, UnixTime);

	// 2. 최상위 루트(GameMessage) 생성
	auto gameMessage = GamePacket::CreateGameMessage(builder,
		GamePacket::PacketData_ChatPacket,
		chatOffset.Union());

	builder.Finish(gameMessage);

	// 4. 전송
	SendPacket(builder.GetBufferPointer(), builder.GetSize());
}

void UNetworkGameInstanceSubsystem::SendPacket(const uint8* Data, int32 Size)
{
	if (!Socket) return;

	// 패킷 구조: [Length(4)] + [FlatBuffer(N)]
	// FlatBuffer(GameMessage) 안에 이미 어떤 패킷인지 정보(Union Type)가 들어있습니다.

	TArray<uint8> Buffer;
	uint32 PacketSize = Size; // Type 바이트(1) 추가 없음

	Buffer.AddUninitialized(4);
	FMemory::Memcpy(Buffer.GetData(), &PacketSize, 4); // Little Endian (Node.js 서버 호환)

	if (Size > 0)
	{
		Buffer.Append(Data, Size);
	}

	int32 Sent = 0;
	Socket->Send(Buffer.GetData(), Buffer.Num(), Sent);
}

// [Receive 로직] - Framing 및 Deserialization

void UNetworkGameInstanceSubsystem::OnDataReceived(const TArray<uint8>& Data)
{
	ReceiveBuffer.Append(Data);
	ProcessPacketBuffer();
}

void UNetworkGameInstanceSubsystem::ProcessPacketBuffer()
{
	// Node.js 서버: writeUInt32LE (4바이트 길이 정보)
	while (ReceiveBuffer.Num() >= 4)
	{
		uint32 PacketSize = 0;
		FMemory::Memcpy(&PacketSize, ReceiveBuffer.GetData(), 4);

		// 데이터가 아직 덜 도착했으면 대기
		if ((uint32)ReceiveBuffer.Num() < 4 + PacketSize)
		{
			break;
		}

		// 패킷 추출 (헤더 4바이트 제거)
		TArray<uint8> Payload;
		Payload.Append(ReceiveBuffer.GetData() + 4, PacketSize);

		// 처리된 데이터 버퍼에서 제거
		ReceiveBuffer.RemoveAt(0, 4 + PacketSize);

		// 처리
		if (Payload.Num() > 0)
		{
			// [변경] 기존에는 Payload[0]이 Type이었으나, 이제 전체가 FlatBuffer입니다.
			// 별도의 Type 파싱 없이 전체 데이터를 넘깁니다.
			HandlePacket(Payload.GetData(), Payload.Num());
		}
	}
}

void UNetworkGameInstanceSubsystem::HandlePacket(const uint8* Data, int32 DataSize)
{
	// 안전장치: FlatBuffer 검증기 (선택 사항이지만 권장)
	flatbuffers::Verifier verifier(Data, DataSize);
	if (!GamePacket::VerifyGameMessageBuffer(verifier))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid FlatBuffer Data Received!"));
		return;
	}

	// 1. 최상위 루트 패킷 열기
	auto Msg = GamePacket::GetGameMessage(Data);

	// 2. Union Type 확인 및 분기
	switch (Msg->data_type())
	{
	case GamePacket::PacketData_LoginResPacket:
	{
		auto pkt = Msg->data_as_LoginResPacket();

		bool bSuccess = pkt->success();
		FString Message = pkt->message() ? UTF8_TO_TCHAR(pkt->message()->c_str()) : TEXT("");

		UE_LOG(LogTemp, Log, TEXT("[Login] Success: %d, Msg: %s"), bSuccess, *Message);
		FBackendRequstResult Result;
		Result.ResultType = EBackendResultType::TCPLogin_RES;
		Result.bIsSuccessful = bSuccess;
		Result.Message = Message;

		OnRequstResult.Broadcast(Result);
		break;
	}

	case GamePacket::PacketData_ChatPacket:
	{
		auto pkt = Msg->data_as_ChatPacket();

		FString Sender = pkt->sender_id() ? UTF8_TO_TCHAR(pkt->sender_id()->c_str()) : TEXT("Unknown");
		FString Message = pkt->message() ? UTF8_TO_TCHAR(pkt->message()->c_str()) : TEXT("");
		int64 Timestamp = pkt->timestamp();
		
		UE_LOG(LogTemp, Log, TEXT("[Chat] %s: %s"), *Sender, *Message);
		OnChatReceived.Broadcast(Sender, Message, Timestamp);
		break;
	}

	case GamePacket::PacketData_NONE:
	default:
		UE_LOG(LogTemp, Warning, TEXT("Empty or Unknown Packet Type"));
		break;
	}
}

void UNetworkGameInstanceSubsystem::BackendValidateToken(const FString& Token, const FUniqueNetIdRepl& RequestUserID)
{
	NET_LOG("");
	auto Request = HTTPModule->CreateRequest();
	
	TWeakObjectPtr<UNetworkGameInstanceSubsystem> WeakThis(this);

	// RequestUserID: 응답이 올 때까지 이 ID를 람다 안에 저장해둠 (복사됨)
	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis, RequestUserID](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			if (!bConnected || !Response.IsValid())
			{
				WeakThis->OnValidateRequstResult.Broadcast(FBackendValidateData(), 0, RequestUserID);
				return;
			}

			int32 StatusCode = Response->GetResponseCode();

			UE_LOG(LogTemp, Warning, TEXT("ValidateResponseCode %d"), StatusCode);

			FString ResponseContent = Response->GetContentAsString();
			FBackendValidateData ValidatData;

			if (FJsonObjectConverter::JsonObjectStringToUStruct<FBackendValidateData>(ResponseContent, &ValidatData, 0, 0))
			{
				// 파싱 성공
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Json Parse Fail"));
			}

			if (StatusCode == 200)
			{
				
				UE_LOG(LogTemp, Log, TEXT("Success for NetID: %s , username : %s"), *RequestUserID.GetUniqueNetId()->ToString(), *ValidatData.username);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Fail for NetID: %s, username : %s"), *RequestUserID.GetUniqueNetId()->ToString(), *ValidatData.username);
			}

			//캡처해뒀던 RequestUserID
			WeakThis->OnValidateRequstResult.Broadcast(ValidatData, StatusCode, RequestUserID);
		});

	FString URL = FString::Printf(TEXT("http://%s:3000/api/v1/auth/verify"), *BackendIP);

	Request->SetURL(URL);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("token"), Token);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);

	Request->ProcessRequest();
}

void UNetworkGameInstanceSubsystem::SaveInventoryData(FString AuthToken, FString InvenJson)
{
	NET_LOG("");
	auto Request = HTTPModule->CreateRequest();

	TWeakObjectPtr<UNetworkGameInstanceSubsystem> WeakThis(this);

	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}
			
			if (!bConnected || !Response.IsValid())
			{
				WeakThis->OnSaveInvenRequstResult.Broadcast(FBackendSaveInvenResult(), -1);
				return;
			}

			int32 StatusCode = Response->GetResponseCode();

			FString ResponseContent = Response->GetContentAsString();
			FBackendSaveInvenResult BackendSaveInvenResult;
			if (FJsonObjectConverter::JsonObjectStringToUStruct<FBackendSaveInvenResult>(ResponseContent, &BackendSaveInvenResult, 0, 0))
			{

			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Save InventoryJson Parse Fail"));
				WeakThis->OnSaveInvenRequstResult.Broadcast(FBackendSaveInvenResult(), StatusCode);
				return;
			}

			if (StatusCode == 200)
			{

				UE_LOG(LogTemp, Log, TEXT("Success for Save Inventory code: %d , message : %s"), StatusCode, *BackendSaveInvenResult.message);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Fail for Save Inventory code: %d , message : %s"), StatusCode, *BackendSaveInvenResult.message);
			}

			WeakThis->OnSaveInvenRequstResult.Broadcast(BackendSaveInvenResult, StatusCode);
		});

	FString URL = FString::Printf(TEXT("http://%s:3000/api/v1/inventory/save"), *BackendIP);

	Request->SetURL(URL);
	Request->SetVerb(TEXT("POST"));

	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (!AuthToken.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));
	}

	Request->SetContentAsString(InvenJson);

	Request->ProcessRequest();
}

void UNetworkGameInstanceSubsystem::LoadInventoryData(FString AuthToken, APlayerController* InventoryOwner)
{
	NET_LOG("");
	auto Request = HTTPModule->CreateRequest();

	TWeakObjectPtr<UNetworkGameInstanceSubsystem> WeakThis(this);

	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis, InventoryOwner](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			if (!bConnected || !Response.IsValid())
			{
				WeakThis->OnLoadInvenRequstResult.Broadcast(FInventorySaveData(), -1, InventoryOwner);
				return;
			}

			int32 StatusCode = Response->GetResponseCode();

			FString ResponseContent = Response->GetContentAsString();
			FInventorySaveData InventorySaveData;
			if (FJsonObjectConverter::JsonObjectStringToUStruct<FInventorySaveData>(ResponseContent, &InventorySaveData, 0, 0))
			{

			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Load Inventory Json Parse Fail"));
				WeakThis->OnLoadInvenRequstResult.Broadcast(FInventorySaveData(), StatusCode, InventoryOwner);
				return;
			}

			if (StatusCode == 200)
			{
				UE_LOG(LogTemp, Log, TEXT("Success for Load Inventory code: %d"), StatusCode);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Fail for Load Inventory code: %d"), StatusCode);
			}

			WeakThis->OnLoadInvenRequstResult.Broadcast(InventorySaveData, StatusCode, InventoryOwner);
		});

	FString URL = FString::Printf(TEXT("http://%s:3000/api/v1/inventory/load"), *BackendIP);

	Request->SetURL(URL);
	Request->SetVerb(TEXT("POST"));

	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (!AuthToken.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));
	}

	Request->ProcessRequest();
}

void UNetworkGameInstanceSubsystem::SaveStashData(FString AuthToken, FString InvenJson)
{
	NET_LOG("");
	auto Request = HTTPModule->CreateRequest();

	TWeakObjectPtr<UNetworkGameInstanceSubsystem> WeakThis(this);

	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			if (!bConnected || !Response.IsValid())
			{
				WeakThis->OnSaveStashRequstResult.Broadcast(FBackendSaveStashResult(), -1);
				return;
			}

			int32 StatusCode = Response->GetResponseCode();

			FString ResponseContent = Response->GetContentAsString();
			FBackendSaveStashResult BackendSaveStashResult;
			if (FJsonObjectConverter::JsonObjectStringToUStruct<FBackendSaveStashResult>(ResponseContent, &BackendSaveStashResult, 0, 0))
			{

			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Save InventoryJson Parse Fail"));
				WeakThis->OnSaveStashRequstResult.Broadcast(FBackendSaveStashResult(), StatusCode);
				return;
			}

			if (StatusCode == 200)
			{

				UE_LOG(LogTemp, Log, TEXT("Success for Save Inventory code: %d , message : %s"), StatusCode, *BackendSaveStashResult.message);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Fail for Save Inventory code: %d , message : %s"), StatusCode, *BackendSaveStashResult.message);
			}

			WeakThis->OnSaveStashRequstResult.Broadcast(BackendSaveStashResult, StatusCode);
		});

	FString URL = FString::Printf(TEXT("http://%s:3000/api/v1/stash/save"), *BackendIP);

	Request->SetURL(URL);
	Request->SetVerb(TEXT("POST"));

	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (!AuthToken.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));
	}

	Request->SetContentAsString(InvenJson);

	Request->ProcessRequest();
}

void UNetworkGameInstanceSubsystem::LoadStashData(FString AuthToken, APlayerController* InventoryOwner)
{
	NET_LOG("");
	auto Request = HTTPModule->CreateRequest();

	TWeakObjectPtr<UNetworkGameInstanceSubsystem> WeakThis(this);

	Request->OnProcessRequestComplete().BindLambda(
		[WeakThis, InventoryOwner](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnected)
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			if (!bConnected || !Response.IsValid())
			{
				WeakThis->OnLoadStashRequstResult.Broadcast(FInventorySaveData(), -1, InventoryOwner);
				return;
			}

			int32 StatusCode = Response->GetResponseCode();

			FString ResponseContent = Response->GetContentAsString();
			FInventorySaveData InventorySaveData;
			if (FJsonObjectConverter::JsonObjectStringToUStruct<FInventorySaveData>(ResponseContent, &InventorySaveData, 0, 0))
			{

			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Load Inventory Json Parse Fail"));
				WeakThis->OnLoadStashRequstResult.Broadcast(FInventorySaveData(), StatusCode, InventoryOwner);
				return;
			}

			if (StatusCode == 200)
			{
				UE_LOG(LogTemp, Log, TEXT("Success for Load Inventory code: %d"), StatusCode);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Fail for Load Inventory code: %d"), StatusCode);
			}

			WeakThis->OnLoadStashRequstResult.Broadcast(InventorySaveData, StatusCode, InventoryOwner);
		});

	FString URL = FString::Printf(TEXT("http://%s:3000/api/v1/stash/load"), *BackendIP);

	Request->SetURL(URL);
	Request->SetVerb(TEXT("POST"));

	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (!AuthToken.IsEmpty())
	{
		Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AuthToken));
	}

	Request->ProcessRequest();
}

void UNetworkGameInstanceSubsystem::SendRequest(FString Verb, FString Endpoint, FString JsonBody, FOnNetworkResponse Callback)
{
	auto Request = FHttpModule::Get().CreateRequest();
	// Base URL + Endpoint
	FString URL = FString::Printf(TEXT("http://%s:3000%s"), *BackendIP, *Endpoint);
	NET_LOG(FString::Printf(TEXT("%s"), *URL));
	Request->SetURL(URL);
	Request->SetVerb(Verb);
	Request->SetHeader("Content-Type", "application/json");

	if (!LoginData.token.IsEmpty())
	{
		Request->SetHeader("Authorization", "Bearer " + LoginData.token);
	}

	if (!JsonBody.IsEmpty())
	{
		Request->SetContentAsString(JsonBody);
	}

	Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bConnected)
		{
			// 200
			if (bConnected && Res.IsValid() && EHttpResponseCodes::IsOk(Res->GetResponseCode()))
			{
				NET_LOG2(FString::Printf(TEXT("Success : \n %s"), *Res->GetContentAsString()));
				Callback.ExecuteIfBound(true, Res->GetContentAsString(), Res->GetResponseCode());
			}
			else if (bConnected && Res.IsValid())
			{
				NET_LOG2(TEXT("Connect Success Respon Fail"));
				Callback.ExecuteIfBound(false, Res->GetContentAsString(), Res->GetResponseCode());
			}
			else
			{
				NET_LOG2(TEXT("Connect Fail"));
				Callback.ExecuteIfBound(false, TEXT("Error"), -1);
			}
		});

	Request->ProcessRequest();
}

//deprecated
void UNetworkGameInstanceSubsystem::OnBackendValidateTokenRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bProcessedSuccessfully)
{
	if (!bProcessedSuccessfully || !Response.IsValid())
	{
		return;
	}

	int32 StatusCode = Response->GetResponseCode();
	UE_LOG(LogTemp, Warning, TEXT("ValidateResponseCode %d"), StatusCode);

	FString ResponseContent = Response->GetContentAsString();

	FBackendValidateData ValidatData;

	if (FJsonObjectConverter::JsonObjectStringToUStruct<FBackendValidateData>(ResponseContent, &ValidatData, 0, 0))
	{
		//UE_LOG(LogTemp, Warning, TEXT("Token : %s, Login message : %s"), *ValidatData.uid, *ValidatData.message);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("OnBackendValidateTokenRequestComplete : JsonObjectStringToUStruct Fail"));
	}

	if (StatusCode == 200)
	{
		UE_LOG(LogTemp, Warning, TEXT("Validate Success uid : %d, username : %s"), ValidatData.uid, *ValidatData.username);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Validate Failed : code: %d, message : %s "), StatusCode, *ValidatData.message);
	}
}
