// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/ATGBTInterface.h"
#include "ATGEnum.h"
#include "Engine/HitResult.h"
#include "Interface/DamageableInterface.h"
#include "AbilitySystemInterface.h" 
#include "Interface/MeleeWeaponInterface.h"
#include "ZombieEnemy.generated.h"

struct FLastDamageCapture
{
	FName BoneName;
	FVector HitLocation;
	FVector HitNormal;
	float ImpulseScale;

};

//DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonsterStateChanged, EMonsterState, InState);

class UNiagaraSystem;
class USliceSystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIHealthChangedSignature, float, OldValue, float, NewValue);

UCLASS()
class PROJECTA_API AZombieEnemy : public ACharacter, public IATGBTInterface, public IDamageableInterface, public IAbilitySystemInterface, public IMeleeWeaponInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AZombieEnemy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PossessedBy(AController* NewController) override;
public:

	//-----------IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

	//-----------IDamageableInterface
	UFUNCTION(BlueprintCallable, Category = "Damageable")
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) override;

	/** Handles death events */
	UFUNCTION(BlueprintCallable, Category = "Damageable")
	virtual void HandleDeath(const FHitResult& InHitResult) override;

	/** Handles healing events */
	UFUNCTION(BlueprintCallable, Category = "Damageable")
	virtual void ApplyHealing(float Healing, AActor* Healer) override;

	UPROPERTY(BlueprintAssignable, Category = "GAS")
	FOnAIHealthChangedSignature OnHealthChanged;

	//------------------------------------

	//-----------IMeleeWeaponInterface
	virtual void StartHitCheck() override;
	virtual void TickHitCheck() override;
	virtual void EndHitCheck() override;
	//------------------------------------
protected:

	UFUNCTION(NetMulticast, Reliable) //����� ����
	void Multi_HandleDeath(const FHitResult& InHitResult);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	class UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY()
	class UCharacterAttributeSet* AttributeSet;

	// 스텟 초기화용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DefaultAttributesEffectClass;

	//   GE (Ʈ	// 맞았을 때 적용할 GE (인스턴트, 데미지 등 기본 타격 처리)
	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<class UGameplayEffect> DefaultDamageEffectClass;

	// 근접 공격 성공 시 적중한 대상에게 적용할 데미지 GE
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Melee")
	TSubclassOf<class UGameplayEffect> DefaultMeleeDamageEffectClass;

	UPROPERTY(EditAnywhere, Category = "Combat|Melee")
	FName MeleeSocketName = TEXT("hand_r");

	UPROPERTY(EditAnywhere, Category = "Combat|Melee")
	float MeleeAttackRadius = 30.f;

	TArray<AActor*> HitActors;
	bool bIsMeleeAttacking = false;

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

	FLastDamageCapture LastDamageCapture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USliceSystemComponent> SliceSystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentHP = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHP = 100;

	void GunPointApplyDamageMomentum(float InImpulseScale, const FVector& ShotDir, FDamageEvent const& DamageEvent, APawn* PawnInstigator, AActor* DamageCauser, bool bScaleMomentumByMass);

	UFUNCTION(NetMulticast, Unreliable)
	void MultiPlayEffectHitReact(const class UATGDamageType* DamageType, const FVector& HitLocation, const FVector& HitNormal, const FName& BoneName, float ImpulseScale);

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

	UFUNCTION(BlueprintCallable)
	void StartSlice(const FHitResult& InHitResult);

protected:
	UPROPERTY(EditAnywhere)
	float DefaultImpulsePower = 10000;

	float DeathBlendWeight = 0.0f;

	float DeathBlendDecreaseSpeed = 0.2f;

	//void SetCurrentHP(float InCurrentHP) { CurrentHP = InCurrentHP; }
};
