// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ATGRangeWeapon.h"
#include "Data/ATGRangeWeaponData.h"
#include "Utils/NetworkUtil.h"
#include "GameFramework/Character.h"
#include "ATGPlayerCharacter.h"
#include "Utils/NetworkUtil.h"
#include "Weapon/ProjectileBase.h"
#include "Weapon/BulletManager.h"
#include "Net/UnrealNetwork.h"
#include "Weapon/BulletManagerWorldSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Data/CustomDamageEvents.h"
#include "Data/ATGWeaponData.h"

void AATGRangeWeapon::OnRep_WeaponData()
{
	Super::OnRep_WeaponData();
	if (const UATGRangeWeaponData* RangeWeaponData = Cast<UATGRangeWeaponData>(WeaponData))
	{
		WeaponBulletData = RangeWeaponData->WeaponBulletData;
	}
	else
	{
		//NET_LOG(TEXT("Failed to Cast UATGRangeWeaponData"));
	}
}

void AATGRangeWeapon::Fire()
{
	//클라에서 요청
	//NET_LOG(TEXT("fire"));
	if (bFullAuto)
	{
		//GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AATGWeaponBase::Fire, WeaponData->RefireRate, false);
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());

	if (!Character)
	{
		return;
	}
	FVector OutSpawnLoc;
	FRotator OutAimRot;
	CalculateShootData(OutSpawnLoc, OutAimRot);

	FireBullet(OutSpawnLoc, OutAimRot);
}

void AATGRangeWeapon::StopFire()
{
}

void AATGRangeWeapon::Reload()
{
}

void AATGRangeWeapon::FireBullet(FVector FireLoc, FRotator FireRot)
{
	//NET_LOG(FString::Printf(TEXT("speed : %f"), WeaponBulletData.Speed));
	FBullet NewBullet;
	NewBullet.StartLocation = FireLoc;
	NewBullet.Location = FireLoc;
	//속도 = 방향 * 속력
	NewBullet.Velocity = FireRot.Vector() * WeaponBulletData.Speed;
	NewBullet.GravityScale = WeaponBulletData.GravityScale;
	NewBullet.DragCoefficient = WeaponBulletData.DragCoefficient;

	NewBullet.BulletOwner = this;

	NewBullet.IgnoreActors.Add(GetInstigator());
	NewBullet.IgnoreActors.Add(this);

	if (UBulletManagerWorldSubsystem* BulletSys = GetWorld() ? GetWorld()->GetSubsystem<UBulletManagerWorldSubsystem>() : nullptr)
	{
		BulletSys->ActiveBullets.Add(NewBullet);
	}

	/*if (ABulletManager::GetBulletManager())
	{
		ABulletManager::GetBulletManager()->ActiveBullets.Add(NewBullet);
	}*/
}

bool AATGRangeWeapon::CalculateShootData(FVector& OutSpawnLocation, FRotator& OutAimRotation)
{
	AATGPlayerCharacter* Character = Cast<AATGPlayerCharacter>(GetOwner());

	if (!Character)
	{
		return false;
	}

	APlayerController* PC = Character->GetController<APlayerController>();
	if (!PC)
	{
		return false;
	}

	if (Mesh)
	{
		OutSpawnLocation = Mesh->GetSocketLocation(MuzzleSocketName);
	}
	else
	{
		return false;
	}
	FVector TempVec;
	PC->GetPlayerViewPoint(TempVec, OutAimRotation);

	return true;
}

void AATGRangeWeapon::ServerStartFire()
{
	//총소리 이펙트 같은거 멀티케스트
	// 대략적인 궤적을 시뮬레이션 옆으로 총알 제압 효과 라인트레이스 한번

	//NET_LOG(TEXT(""));
}

void AATGRangeWeapon::TryHitFire(const FBulletHitResult& BulletHitResult)
{
	//NET_LOG(TEXT(""));

	DrawDebugLine(GetWorld(), BulletHitResult.Bullet.StartLocation, BulletHitResult.HitLocation, FColor::Blue, false, 3.f);
	ServerHitFire(BulletHitResult);
}

bool AATGRangeWeapon::ServerHitFire_Validate(const FBulletHitResult& BulletHitResult)
{
	//NET_LOG(TEXT(""));

	// 데이터가 깨져서 오지 않았는지
	if (BulletHitResult.Bullet.StartLocation.ContainsNaN() || BulletHitResult.HitLocation.ContainsNaN())
	{
		return false;
	}

	return true;
}

void AATGRangeWeapon::ServerHitFire_Implementation(const FBulletHitResult& BulletHitResult)
{
	//NET_LOG(TEXT(""));
	//UPrimitiveComponent* PrimitiveComp = BulletHitResult.HitResult.GetComponent();
	AActor* HitActor = BulletHitResult.Result.GetActor();
	if (!HitActor)
	{
		//NET_LOG(TEXT("Hit Rejected: HitActor Null"));
		return;
	}
	// 시작점 검증 (유저가 너무 멀리 이동했나?)
	// 총알이 날아가는 시간 동안 플레이어는 이동할 수 있음
	//현재 위치와 발사 위치의 차이를 검사
	float DistFromStart = FVector::Dist(GetActorLocation(), BulletHitResult.Bullet.StartLocation);

	// 허용 오차: (최대 이동 속도 * 총알 수명) + 여유분
	float Speed = GetOwner() ? GetOwner()->GetVelocity().Size() : 600.f;

	float MaxAllowedDist = Speed * BulletHitResult.Bullet.LifeTime + 100.f;
	//NET_LOG(FString::Printf(TEXT("LifeTime : %f, Dist : %f , AllowDist : %f"), BulletHitResult.Bullet.LifeTime, DistFromStart, MaxAllowedDist));
	if (DistFromStart > MaxAllowedDist)
	{
		// 로그만 찍고 데미지 처리는 안 함
		//NET_LOG(TEXT("Hit Rejected: Shooter moved too far from StartLocation"));
		return;
	}


	//  타겟 위치 검증 (맞았다는 위치에 적이 실제로 있는가?)
	// 움직이는 적을 맞춘 경우, 핑 차이 때문에 서버 위치와 클라 히트 위치가 다름
	// 적의 중심과 히트 위치 사이의 거리
	float HitDistToTarget = FVector::Dist(HitActor->GetActorLocation(), BulletHitResult.HitLocation);
	float TargetCapsuleRadius = 100.0f;
	float PingTolerance = 150.0f;

	// 적의 중심에서 (반지름 + 오차) 범위를 벗어난 곳을 맞췄다고 주장하면 기각
	if (HitDistToTarget > (TargetCapsuleRadius + PingTolerance))
	{
		//NET_LOG(TEXT("Hit Rejected: Target is not at HitLocation"));
		return;
	}


	// 경로 검증 (월핵 방지)
	FHitResult ServerWallHit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());
	Params.AddIgnoredActor(HitActor);

	bool bHitWall = GetWorld()->LineTraceSingleByChannel(ServerWallHit, BulletHitResult.Bullet.StartLocation, BulletHitResult.HitLocation, ECC_Visibility, Params);

	if (bHitWall)
	{
		// HitLocation보다 더 가까운 곳에 벽이 있는지
		if (ServerWallHit.Distance < FVector::Dist(BulletHitResult.Bullet.StartLocation, BulletHitResult.HitLocation) - 10.0f)
		{
			//NET_LOG(TEXT("Hit Rejected: Wall detected between shooter and target"));
			return;
		}
	}


	//데미지 적용
	//NET_LOG(TEXT("Hit Verified Dealing Damage"));

	ApplayGunDamage(
		HitActor,
		WeaponBulletData.AttackPower,
		WeaponBulletData.ImpulseScale,
		(BulletHitResult.HitLocation - BulletHitResult.Bullet.StartLocation).GetSafeNormal(),
		BulletHitResult.Result,
		GetOwner()->GetInstigatorController(),
		this, WeaponData->DamageTypeClass);

}


float AATGRangeWeapon::ApplayGunDamage(AActor* DamagedActor, float BaseDamage, float ImpulseScale, FVector const& HitFromDirection, FHitResult const& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass)
{
	if (DamagedActor && BaseDamage != 0.f)
	{
		// make sure we have a good damage type
		TSubclassOf<UDamageType> const ValidDamageTypeClass = DamageTypeClass ? DamageTypeClass : TSubclassOf<UDamageType>(UDamageType::StaticClass());
		FGunPointDamageEvent GunDamageEvent(WeaponData->OwnedTags, BaseDamage, ImpulseScale, HitInfo, HitFromDirection, ValidDamageTypeClass);

		return DamagedActor->TakeDamage(BaseDamage, GunDamageEvent, EventInstigator, DamageCauser);
	}

	return 0.f;
}
