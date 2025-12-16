// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BulletManagerWorldSubsystem.h"
#include "Utils/NetworkUtil.h"
#include "Weapon/ATGWeaponBase.h"

UBulletManagerWorldSubsystem::UBulletManagerWorldSubsystem()
{
	
}

void UBulletManagerWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	GetWorld()->GetTimerManager().SetTimer(TimerHandle_Loop, this, &UBulletManagerWorldSubsystem::SimulateBullets, SimulateInterval, true);
}

void UBulletManagerWorldSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UBulletManagerWorldSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


}

TStatId UBulletManagerWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBulletManagerWorldSubsystem, STATGROUP_Tickables);
}

void UBulletManagerWorldSubsystem::SimulateBullets()
{
	//중력
	const float WorldGravityZ = GetWorld()->GetGravityZ();

	//역for문
	for (int32 i = ActiveBullets.Num() - 1; i >= 0; i--)
	{
		
		FBullet& Bullet = ActiveBullets[i];

		FVector StartLocation = Bullet.Location;

		//중력 적용
		Bullet.Velocity.Z += (WorldGravityZ * Bullet.GravityScale * SimulateInterval);

		//항력 계수 적용
		if (Bullet.DragCoefficient > 0.0f)
		{
			Bullet.Velocity *= (1.0f - (Bullet.DragCoefficient * SimulateInterval));
		}

		if (Bullet.Velocity.SizeSquared() < 10.0f || Bullet.Location.Z < -100.f) // 속도 느려지면 삭제
		{
			ActiveBullets.RemoveAtSwap(i);
			continue;
		}

		//이동 거리 
		FVector EndLocation = StartLocation + (Bullet.Velocity * SimulateInterval);

		TArray<FHitResult> HitResults;

		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActors(Bullet.IgnoreActors);
		//정밀 추적
		CollisionQueryParams.bTraceComplex = true;

		FColor DrawColor = FColor::Green; // 기본: 클라이언트 (초록색)
		//if (GetWorld()->GetNetMode() != NM_Client)
		//{
		//	DrawColor = FColor::Red; // 서버: 빨간색
		//}

		DrawDebugLine(GetWorld(), StartLocation, EndLocation, DrawColor, false, 3.f);

		bool bHit = GetWorld()->LineTraceMultiByChannel(HitResults, StartLocation, EndLocation, ECC_Visibility, CollisionQueryParams);
		for (const auto& HitResult : HitResults)
		{
			NET_LOG(FString::Printf(TEXT("%s"), *HitResult.GetActor()->GetName()));
			if (!HitResult.bBlockingHit)
			{
				Bullet.PierceActors.Add(HitResult.GetActor());
				DrawDebugSphere(GetWorld(), HitResult.Location, 5.f, 5, FColor::Cyan, false, 3.f);
			}
			else
			{
				DrawDebugSphere(GetWorld(), HitResult.Location, 5.f, 5, FColor::Red, false, 3.f);

				// 충돌 처리 
				AATGWeaponBase* WeaponBase = Bullet.BulletOwner ? Cast<AATGWeaponBase>(Bullet.BulletOwner) : nullptr;
				if (WeaponBase)
				{
					FBulletHitResult BulletHitResult;
					BulletHitResult.Bullet = Bullet;
					BulletHitResult.HitLocation = HitResult.Location;
					BulletHitResult.Result = HitResult;
					WeaponBase->TryHitFire(BulletHitResult);
				}
				ActiveBullets.RemoveAtSwap(i);
				continue;
			}
		}

		Bullet.Location = EndLocation;
		Bullet.LifeTime += SimulateInterval;
		if (Bullet.LifeTime > 5.f)
		{
			ActiveBullets.RemoveAtSwap(i);
		}
	}
}
