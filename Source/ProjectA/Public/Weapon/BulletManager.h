// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletManager.generated.h"

struct FBullet
{
public:
	//위치
	FVector Location;
	//속도 방향 + 속력
	FVector Velocity;
	// 중력 영향도 1.0 ~ 0.0
	float GravityScale;
	// 공기 저항 계수 0.0이면 저항 없음, 값이 클수록 빨리 느려짐
	float DragCoefficient;
};

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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
