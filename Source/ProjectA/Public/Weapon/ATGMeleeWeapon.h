// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/ATGWeaponBase.h"
#include "Interface/MeleeWeaponinterface.h"
#include "GameplayTagContainer.h"
#include "ATGMeleeWeapon.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTA_API AATGMeleeWeapon : public AATGWeaponBase, public IMeleeWeaponInterface
{
	GENERATED_BODY()
	
public:

	AATGMeleeWeapon();

	// IMeleeWeaponInterface
	// 공격 시작 
	virtual void StartHitCheck() override;

	// 매 프레임 판정 수행
	virtual void TickHitCheck() override;

	// 공격 종료
	virtual void EndHitCheck() override;

protected:
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float TraceRadius = 20.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    FName StartSocketName = TEXT("TraceStart");

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    FName EndSocketName = TEXT("TraceEnd");

    UPROPERTY(EditDefaultsOnly, Category = "GAS")
    FGameplayTag HitEventTag;

private:
    // 중복 피격 방지용
    UPROPERTY()
    TSet<AActor*> IgnoreActors;

    // 터널링 방지(끊김 없는 판정)를 위한 이전 프레임 소켓 위치 저장
    FVector PreviousStartLocation;
    FVector PreviousEndLocation;

    void ProcessHit(const FHitResult& HitResult);

};
