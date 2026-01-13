// Fill out your copyright notice in the Description page of Project Settings.


#include "FTcpReceiverWorker.h"

FTcpReceiverWorker::FTcpReceiverWorker(FSocket* InSocket, const FOnBytesReceived& InCallback)
{
	Socket = InSocket;
	Callback = InCallback;
	bStopping = false;

	Thread = FRunnableThread::Create(this, TEXT("TcpReceiverThread"), 0, TPri_BelowNormal);
}

FTcpReceiverWorker::~FTcpReceiverWorker()
{
	if (Thread)
	{
		Stop();
		Thread->WaitForCompletion();
		delete Thread;
        Thread = nullptr;
	}
}

bool FTcpReceiverWorker::Init()
{
	return true;
}

uint32 FTcpReceiverWorker::Run()
{
    while (!bStopping && Socket)
    {
        // 0.1초 동안 데이터가 오길 기다림 (Sleep)
        if (Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(0.1)))
        {
            uint32 PendingDataSize = 0;
            if (Socket->HasPendingData(PendingDataSize) && PendingDataSize > 0)
            {
                TArray<uint8> Buffer;
                Buffer.SetNumUninitialized(PendingDataSize);

                int32 BytesRead = 0;
                if (Socket->Recv(Buffer.GetData(), PendingDataSize, BytesRead))
                {
                    // 수신 성공 -> 메인 스레드(GameThread)로 데이터 전달
                    // TFunction 복사를 위해 Buffer를 캡처
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

void FTcpReceiverWorker::Exit()
{
}

void FTcpReceiverWorker::Stop()
{
    bStopping = true;
}
