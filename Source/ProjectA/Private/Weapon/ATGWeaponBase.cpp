// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ATGWeaponBase.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "ATGPlayerCharacter.h"
#include "NetworkUtil.h"
#include "Weapon/ProjectileBase.h"
#include "Weapon/BulletManager.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AATGWeaponBase::AATGWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	

}

// Called when the game starts or when spawned
void AATGWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void AATGWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	//DOREPLIFETIME(AATGWeaponBase, WeaponBulletData);
	// 만약 데이터가 변하지 않고 처음에만 전송
	DOREPLIFETIME_CONDITION(AATGWeaponBase, WeaponBulletData, COND_InitialOnly);
}

// Called every frame
void AATGWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AATGWeaponBase::Fire()
{
	if (bFullAuto)
	{
		GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AATGWeaponBase::Fire, RefireRate, false);
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

void AATGWeaponBase::StopFire()
{
}

void AATGWeaponBase::Reload()
{
}

void AATGWeaponBase::FireBullet(FVector FireLoc, FRotator FireRot)
{
	NET_LOG(FString::Printf(TEXT("speed : %f"), WeaponBulletData.Speed));
	FBullet NewBullet;
	NewBullet.Location = FireLoc;
	//속도 = 방향 * 속력
	NewBullet.Velocity = FireRot.Vector() * WeaponBulletData.Speed;
	NewBullet.GravityScale = WeaponBulletData.GravityScale;
	NewBullet.DragCoefficient = WeaponBulletData.DragCoefficient;

	NewBullet.IgnoreActors.Add(GetInstigator());
	NewBullet.IgnoreActors.Add(this);

	if (ABulletManager::GetBulletManager())
	{
		ABulletManager::GetBulletManager()->ActiveBullets.Add(NewBullet);
	}
}

bool AATGWeaponBase::CalculateShootData(FVector& OutSpawnLocation, FRotator& OutAimRotation)
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


