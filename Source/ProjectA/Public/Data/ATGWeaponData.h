// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATGEquipmentData.h"
#include "ATGWeaponData.generated.h"

/**
 * 
 */

// ¾îºô¸®Æ¼¿Í ÀÎÇ²
USTRUCT(BlueprintType)
struct FWeaponAbilityBind
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    EPlayerAbilityInputID InputID;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TSubclassOf<class UGameplayAbility> AbilityClass;
};


class AATGWeaponBase;

UCLASS(Abstract)
class PROJECTA_API UATGWeaponData : public UATGEquipmentData
{
	GENERATED_BODY()
	
public:
    UATGWeaponData();

    //Weapon type
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    EWeaponType WeaponType = EWeaponType::None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    EWeaponCategory WeaponCategory = EWeaponCategory::None;

    //Player equip skeletalmesh
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    USkeletalMesh* WeaponSkeletalMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Socket");
    FName HandSocketName = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Socket");
    FName Main1BackSocketName = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Socket");
    FName Main2BackSocketName = TEXT("");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Socket");
    FName SubBackSocketName = TEXT("");

    virtual FName GetSocketName(EEquipmentSlotType SlotType, bool bIsEquipped) const;
    //weaponclass to equip
    //UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    //TSubclassOf<AATGWeaponBase> WeaponClass = nullptr;

    virtual TSubclassOf<class AATGWeaponBase> GetWeaponClass() const PURE_VIRTUAL(UATGWeaponData::GetWeaponClass, return nullptr;);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    TSubclassOf<UDamageType> DamageTypeClass = nullptr;
public:

    // GA
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
    TArray<FWeaponAbilityBind> WeaponAbilitys;

};
