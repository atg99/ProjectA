// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATGEnum.h"
#include "BulletManager.generated.h"


/*
* Parallel Simulation
* 이 엑터는 Replicate 안하고 다른 엑터에서 RPC
* 발사한 클라 : 서버에 RPC 전송총알 탄도학 계산 총알시각표현 맞으면 이펙트 처리
* 서버 : MultiCast RPC 총알 탄도학 계산 맞으면 데미지 처리 맞으면 Multi RPC 이펙트
* Other 클라 : 총알시각표현
*/
UCLASS()
class PROJECTA_API ABulletManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABulletManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	TArray<FBullet> ActiveBullets;

	//SingleTon
public:
	// static 함수로 접근
	static ABulletManager* GetBulletManager();

private:
	// Weak 포인터 사용 대상이 파괴되면 알아서 무효화됨.
	static TWeakObjectPtr<ABulletManager> GlobalBulletManagerInstance;

};
