// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityLibrary.h"
#include "Http.h"
#include "MCPAIChat.generated.h"

/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOrchestratorResponseDelegate, const FString&, ResponseContent, bool, bSuccess);

UCLASS()
class PROJECTA_API UMCPAIChat : public UEditorUtilityBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
    // 1. BP에서 호출할 노드 생성 함수 (static)
    // BlueprintInternalUseOnly = true로 설정해야 이 함수 자체가 노드로 변환됩니다.
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "AI|Orchestrator")
    static UMCPAIChat* SendChatToOrchestrator(const FString& UserMessage, const FString& ModelName = TEXT("qwen3:4b"));

    // 2. 실행 핀 (성공/실패)
    UPROPERTY(BlueprintAssignable)
    FOrchestratorResponseDelegate OnSuccess;

    UPROPERTY(BlueprintAssignable)
    FOrchestratorResponseDelegate OnFail;

    // 3. 실제 비동기 작업 시작 함수
    virtual void Activate() override;

private:
    void HandleRequestCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    FString TargetUrl;
    FString UserMessagePayload;
    FString TargetModel;
	
};
