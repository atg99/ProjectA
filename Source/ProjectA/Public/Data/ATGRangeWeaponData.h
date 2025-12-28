// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/ATGWeaponData.h"
#include "ATGRangeWeaponData.generated.h"

/**
 * 
 */
class AATGRangeWeapon;
UCLASS()
class PROJECTA_API UATGRangeWeaponData : public UATGWeaponData
{
	GENERATED_BODY()

public:

    UATGRangeWeaponData();

    //weaponclass to equip
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    TSubclassOf<AATGRangeWeapon> WeaponClass = nullptr;

    virtual TSubclassOf<class AATGWeaponBase> GetWeaponClass() const override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
    FWeaponBulletData WeaponBulletData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item | delay")
    float ADSTime;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item | delay")
    float SprinttoFireTime;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Data, meta = (ClampMin = 0.01f, ClampMax = 2.0f, Unit = "s"))
    float RefireRate = 0.5f;
	
};
