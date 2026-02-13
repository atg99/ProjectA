// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/MCPAIChat.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"

UMCPAIChat* UMCPAIChat::SendChatToOrchestrator(const FString& UserMessage, const FString& ModelName)
{
    UMCPAIChat* Node = NewObject<UMCPAIChat>();
    Node->TargetUrl = TEXT("http://localhost:8000/chat"); // 명세서의 URL
    Node->UserMessagePayload = UserMessage;
    Node->TargetModel = ModelName;
    Node->AddToRoot();
    return Node;
}

void UMCPAIChat::Activate()
{
    // 1. JSON 객체 생성 (명세서 구조)
    // {
    //    "messages": [ { "role": "user", "content": "..." } ],
    //    "model": "..."
    // }

    // 1-1. 메시지 객체 생성
    TSharedPtr<FJsonObject> MessageObj = MakeShareable(new FJsonObject);
    MessageObj->SetStringField("role", "user");
    MessageObj->SetStringField("content", UserMessagePayload);

    // 1-2. 메시지 배열 생성
    TArray<TSharedPtr<FJsonValue>> MessagesArray;
    MessagesArray.Add(MakeShareable(new FJsonValueObject(MessageObj)));

    // 1-3. 루트 JSON 객체 생성
    TSharedPtr<FJsonObject> RootObj = MakeShareable(new FJsonObject);
    RootObj->SetArrayField("messages", MessagesArray);
    if (!TargetModel.IsEmpty())
    {
        RootObj->SetStringField("model", TargetModel);
    }

    // 2. JSON을 문자열로 변환 (Serialize)
    FString RequestBody;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
    FJsonSerializer::Serialize(RootObj.ToSharedRef(), Writer);

    // 3. HTTP 요청 생성
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TargetUrl);
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(RequestBody);
    Request->SetTimeout(300.f);
    // 4. 콜백 바인딩 및 요청 전송
    Request->OnProcessRequestComplete().BindUObject(this, &UMCPAIChat::HandleRequestCompleted);
    Request->ProcessRequest();
}

void UMCPAIChat::HandleRequestCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    // 1. 네트워크 연결 및 HTTP 응답 객체 유효성 검사
    if (!bWasSuccessful || !Response.IsValid())
    {
        FString ErrorMsg = TEXT("Network Connection Failed.");
        if (Request.IsValid())
        {
            if (Response.IsValid())
            {
                EHttpFailureReason Reason = Response->GetFailureReason();
                ErrorMsg += FString::Printf(TEXT("\nReason : %d"), (uint8)Reason);
            }
            ErrorMsg += FString::Printf(TEXT("\nTarget URL: %s"), *Request->GetURL());
        }

        UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMsg);
        OnFail.Broadcast(ErrorMsg, false);
        SetReadyToDestroy();
        return;
    }

    // 2. HTTP 상태 코드 확인 (200 OK가 아닌 경우 처리)
    int32 ResponseCode = Response->GetResponseCode();
    if (ResponseCode != 200)
    {
        FString ErrorMsg = FString::Printf(TEXT("HTTP Error: %d"), ResponseCode);
        ErrorMsg += FString::Printf(TEXT("\nURL: %s"), *Request->GetURL());
        ErrorMsg += FString::Printf(TEXT("\nResponse Body: %s"), *Response->GetContentAsString());

        // 헤더 정보도 로그에 추가 (인증 오류 등 확인용)
        TArray<FString> Headers = Response->GetAllHeaders();
        ErrorMsg += TEXT("\n--- Headers ---");
        for (const FString& Header : Headers)
        {
            ErrorMsg += FString::Printf(TEXT("\n%s"), *Header);
        }

        UE_LOG(LogTemp, Error, TEXT("Request Failed: %s"), *ErrorMsg);
        OnFail.Broadcast(ErrorMsg, false);
        SetReadyToDestroy();
        return;
    }

    // 3. 응답 JSON 파싱 시도
    TSharedPtr<FJsonObject> JsonResponse;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

    if (FJsonSerializer::Deserialize(Reader, JsonResponse) && JsonResponse.IsValid())
    {
        FString ReplyContent;
        // 4. 'content' 필드 확인
        if (JsonResponse->TryGetStringField(TEXT("content"), ReplyContent))
        {
            // === 성공 ===
            UE_LOG(LogTemp, Log, TEXT("Request Success: Content received."));
            OnSuccess.Broadcast(ReplyContent, true);
            SetReadyToDestroy();
            return;
        }
        else
        {
            // 5. JSON은 유효하나 구조가 예상과 다른 경우 (논리적 오류)
            FString ErrorMsg = TEXT("Invalid JSON Structure: Missing 'content' field.");
            ErrorMsg += FString::Printf(TEXT("\nReceived JSON: %s"), *Response->GetContentAsString());

            UE_LOG(LogTemp, Warning, TEXT("%s"), *ErrorMsg);
            OnFail.Broadcast(ErrorMsg, false);
            SetReadyToDestroy();
            return;
        }
    }
    else
    {
        // 6. JSON 파싱 실패 (형식이 JSON이 아님)
        FString ErrorMsg = TEXT("JSON Deserialization Failed.");
        ErrorMsg += FString::Printf(TEXT("\nError: Line %s"), *Reader->GetErrorMessage());
        ErrorMsg += FString::Printf(TEXT("\nRaw Content: %s"), *Response->GetContentAsString());

        UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMsg);
        OnFail.Broadcast(ErrorMsg, false);
        SetReadyToDestroy();
        return;
    }
}