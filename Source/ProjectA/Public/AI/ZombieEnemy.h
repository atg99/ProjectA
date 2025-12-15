// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/ATGBTInterface.h"
#include "ATGEnum.h"
#include "ZombieEnemy.generated.h"

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonsterStateChanged, EMonsterState, InState);

class UNiagaraSystem;

UCLASS()
class PROJECTA_API AZombieEnemy : public ACharacter, public IATGBTInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AZombieEnemy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "State")
	EMonsterState MonsterState = EMonsterState::Normal;

	virtual float TryPlayMontage(UAnimMontage* Montage, float PlayRate = 1.f, FName StartSessionName = NAME_None) override;

	UFUNCTION(NetMulticast, Reliable)
	void MultiPlayMontage(UAnimMontage* Montage, float PlayRate = 1.f, FName StartSessionName = NAME_None);

	virtual void TryStopMontage(UAnimMontage* Montage) override;

	UFUNCTION(NetMulticast, Reliable)
	void MultiStopMontage(UAnimMontage* Montage);

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	//FOnMonsterStateChanged OnMonsterStateChanged;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentHP = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHP = 100;

	void GunPointApplyDamageMomentum(float InImpulseScale, const FVector& ShotDir, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser, bool bScaleMomentumByMass);

	UFUNCTION(NetMulticast, Unreliable)
	void MultiPlayEffectHitReact(const class UATGDamageType* DamageType, const FVector& HitLocation, const FVector& HitNormal);

	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* NormalDamageImpactVFX;

	UPROPERTY(EditAnywhere, Category = "VFX")
	UNiagaraSystem* FireDamageImpactVFX;

	void ReceiveGunPointDamage(const struct FGunPointDamageEvent* Event, float Damage, const class UATGDamageType* DamageType, FVector HitLocation, FVector HitNormal, class UPrimitiveComponent* HitComponent, FName BoneName, FVector ShotFromDirection, class AController* InstigatedBy, AActor* DamageCauser, const FHitResult& HitInfo);

public:

	//void CheckMonsterState();

	float GetCurrentHP() const { return CurrentHP;}

	UFUNCTION(BlueprintImplementableEvent)
	void StartDeath();

	UFUNCTION(NetMulticast, Reliable)
	void MultiStartDeath();
	bool bIsDying = false;

protected:
	float DeathBlendWeight = 0.0f;

	float DeathBlendDecreaseSpeed = 0.2f;

	//void SetCurrentHP(float InCurrentHP) { CurrentHP = InCurrentHP; }
};
