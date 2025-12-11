// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/BulletManagerWorldSubsystem.h"
#include "NetworkUtil.h"
#include "Weapon/ATGWeaponBase.h"

UBulletManagerWorldSubsystem::UBulletManagerWorldSubsystem()
{

}

void UBulletManagerWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UBulletManagerWorldSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UBulletManagerWorldSubsystem::Tick(float DeltaTime)
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

		if (Bullet.Velocity.SizeSquared() < 10.0f || Bullet.Location.Z < -100.f) // 속도 느려지면 삭제
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

		DrawDebugLine(GetWorld(), StartLocation, EndLocation, DrawColor, false, 3.f);
		bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, CollisionQueryParams);
		if (bHit)
		{
			// 충돌 처리 
			AATGWeaponBase* WeaponBase = Bullet.BulletOwner ? Cast<AATGWeaponBase>(Bullet.BulletOwner) : nullptr;
			if (WeaponBase)
			{
				FBulletHitResult BulletHitResult;
				BulletHitResult.StartLocation = Bullet.StartLocation;
				BulletHitResult.HitLocation = HitResult.Location;
				WeaponBase->TryHitFire(BulletHitResult);
			}
			ActiveBullets.RemoveAtSwap(i);
			continue;
		}

		Bullet.Location = EndLocation;

	}
}

TStatId UBulletManagerWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBulletManagerWorldSubsystem, STATGROUP_Tickables);
}
