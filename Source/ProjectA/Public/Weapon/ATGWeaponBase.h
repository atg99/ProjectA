// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Public/ATGEnum.h"
#include "ATGWeaponBase.generated.h"

class UStaticMeshComponent;
class AProjectileBase;
class UATGWeaponData;

UCLASS()
class PROJECTA_API AATGWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AATGWeaponBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USkeletalMeshComponent> Mesh;
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MuzzleSocketName = TEXT("Muzzle");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	int32 MaxBulletCount = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	int32 CurrentBulletCount = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	TObjectPtr<UAnimMontage> ReloadMontage;

public:

	UPROPERTY(ReplicatedUsing = OnRep_WeaponData, BlueprintReadOnly)
	UATGWeaponData* WeaponData;

	UFUNCTION()
	void OnRep_WeaponData();

	UFUNCTION(BlueprintCallable)
	void Fire();

	UFUNCTION(BlueprintCallable)
	void StopFire();

	UFUNCTION(BlueprintCallable)
	void Reload();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	uint8 bFullAuto : 1 = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	float TimeofLastShoot = 0.0f;

	//처음에만 복제
	UPROPERTY(BlueprintReadWrite, Category = Data)
	FWeaponBulletData WeaponBulletData;

	UFUNCTION(BlueprintCallable)
	void FireBullet(FVector FireLoc, FRotator FireRot);

	FTimerHandle RefireTimer;

	bool CalculateShootData(FVector& OutSpawnLocation, FRotator& OutAimRotation);

	//서버에서 호출됨
	void ServerStartFire();

	void TryHitFire(const FBulletHitResult& BulletHitResult);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerHitFire(const FBulletHitResult& BulletHitResult);

protected:

	float ApplayGunDamage(AActor* DamagedActor, float BaseDamage, float ImpulseScale, FVector const& HitFromDirection, FHitResult const& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass);
};

