// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATGEquipmentData.h"
#include "ATGWeaponData.generated.h"

/**
 * 
 */
class AATGWeaponBase;

UCLASS()
class PROJECTA_API UATGWeaponData : public UATGEquipmentData
{
	GENERATED_BODY()
	
public:
    UATGWeaponData();

    //Weapon type
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    EWeaponType WeaponType = EWeaponType::None;

    //Player equip skeletalmesh
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    USkeletalMesh* WeaponSkeletalMesh = nullptr;

    //weaponclass to equip
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    TSubclassOf<AATGWeaponBase> WeaponClass = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    TSubclassOf<UDamageType> DamageTypeClass = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
    FWeaponBulletData WeaponBulletData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item | delay")
    float ADSTime;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item | delay")
    float SprinttoFireTime;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Data, meta = (ClampMin = 0.01f, ClampMax = 2.0f, Unit = "s"))
    float RefireRate = 0.5f;


};
