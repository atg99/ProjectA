// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Public/ATGEnum.h"
#include "ATGWeaponBase.generated.h"

class UStaticMeshComponent;
class AProjectileBase;

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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USkeletalMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	TSubclassOf<AProjectileBase> ProjectileTemplate;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TSubclassOf<ABulletBase> BulletTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SocketName = TEXT("HandGrip_R");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	int32 MaxBulletCount = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	int32 CurrentBulletCount = 100;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Data)
	TObjectPtr<UAnimMontage> ReloadMontage;

public:

	UFUNCTION(BlueprintCallable)
	void Fire();

	UFUNCTION(BlueprintCallable)
	void StopFire();

	UFUNCTION(BlueprintCallable)
	void Reload();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	uint8 bFullAuto : 1 = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data, meta = (ClampMin = 0.1f, ClampMax = 2.0f, Unit = "s"))
	float RefireRate = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Data)
	float TimeofLastShoot = 0.0f;

	UFUNCTION(BlueprintCallable)
	void SpawnFireProjectile(FTransform SpawnTransform);

	FTimerHandle RefireTimer;

	bool CalculateShootData(FVector& OutSpawnLocation, FRotator& OutAimRotation);
};

