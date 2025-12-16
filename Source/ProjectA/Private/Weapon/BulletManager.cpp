// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BulletManager.h"
#include "Utils/NetworkUtil.h"

TWeakObjectPtr<ABulletManager> ABulletManager::GlobalBulletManagerInstance = nullptr;

// Sets default values
ABulletManager::ABulletManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABulletManager::BeginPlay()
{
	Super::BeginPlay();

	if (GlobalBulletManagerInstance.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("BulletManager duplication"));

		Destroy();
		return;
	}

	GlobalBulletManagerInstance = this;

}

void ABulletManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (GlobalBulletManagerInstance == this)
	{
		GlobalBulletManagerInstance = nullptr;
	}
}

// Called every frame
void ABulletManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//중력
	const float WorldGravityZ = GetWorld()->GetGravityZ();

	//역for문
	for (int32 i = ActiveBullets.Num() - 1; i >= 0; i--)
	{
		NET_LOG(FString::Printf(TEXT("%d"), i));
		FBullet& Bullet = ActiveBullets[i];

		FVector StartLocation = Bullet.Location;

		//중력 적용
		Bullet.Velocity.Z += (WorldGravityZ * Bullet.GravityScale * DeltaTime);

		//항력 계수 적용
		if (Bullet.DragCoefficient > 0.0f)
		{
			Bullet.Velocity *= (1.0f - (Bullet.DragCoefficient * DeltaTime));
		}

		if (Bullet.Velocity.SizeSquared() < 10.0f) // 속도 느려지면 삭제
		{
			ActiveBullets.RemoveAtSwap(i); 
			continue;
		}

		//이동 거리 
		FVector EndLocation = StartLocation + (Bullet.Velocity * DeltaTime);

		FHitResult HitResult;
		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActors(Bullet.IgnoreActors);
		FColor DrawColor = FColor::Green; // 기본: 클라이언트 (초록색)
		if (GetWorld()->GetNetMode() != NM_Client)
		{
			DrawColor = FColor::Red; // 서버: 빨간색
		}
		DrawDebugLine(GetWorld(), StartLocation, EndLocation, DrawColor, false, 0.5f);
		bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, CollisionQueryParams);
		if (bHit)
		{
			// 충돌 처리 
			ActiveBullets.RemoveAtSwap(i);
			continue;
		}
		
		Bullet.Location = EndLocation;
	
	}
}

ABulletManager* ABulletManager::GetBulletManager()
{
	if (GlobalBulletManagerInstance.IsValid())
	{
		return GlobalBulletManagerInstance.Get();
	}
	return nullptr;
}

