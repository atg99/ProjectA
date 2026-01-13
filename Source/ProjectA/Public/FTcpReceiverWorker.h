// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Async/Async.h"
/**
 * 
 */

class PROJECTA_API FTcpReceiverWorker : public FRunnable
{
public:
    using FOnBytesReceived = TFunction<void(const TArray<uint8>&)>;


    FTcpReceiverWorker(FSocket* InSocket, const FOnBytesReceived& InCallback);


    virtual ~FTcpReceiverWorker() override;

    /*
        Init()
        Game Thread 에서 실행된다. - Blocking
        Thread 초기화 및 생성자이다.
        false 를 반환하면 Thread 가 실행되지 않는다.

        Run()
        새로운 Thread 에서 실행된다. - Nonblocking
        Multi Thread 작업을 수행하는 실제 함수이다.

        Exit()
        새로운 Thread 에서 실행된다. - Nonblocking
        Thread 실행이 완료되고 실행된다.
        즉, Run() 함수가 종료된 후 실행된다.

        Stop()
        Game Thread 에서 실행된다. - Blocking
        FRunnableThread:: Kill() 이 호출되고 실행된다.
    */

    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Exit() override;
    virtual void Stop() override;

private:
    
    FSocket* Socket;
    FOnBytesReceived Callback;
    FRunnableThread* Thread;
    bool bStopping;
};
