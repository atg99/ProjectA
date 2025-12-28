// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/ATGWeaponData.h"
#include "ATGMeleeWeaponData.generated.h"

/**
 * 
 */
class AATGMeleeWeapon;
UCLASS()
class PROJECTA_API UATGMeleeWeaponData : public UATGWeaponData
{
	GENERATED_BODY()
	
public:
    UATGMeleeWeaponData();

    //weaponclass to equip
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    TSubclassOf<AATGMeleeWeapon> WeaponClass = nullptr;

    virtual TSubclassOf<class AATGWeaponBase> GetWeaponClass() const override;

    // GA
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
    TArray<TSubclassOf<class UGameplayAbility>> WeaponAbilitys;
};
