// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/ATGWeaponData.h"

UATGWeaponData::UATGWeaponData() : Super()
{
	EquipmentType = EEquipmentType::Weapon;
}

FName UATGWeaponData::GetSocketName(EEquipmentSlotType SlotType, bool bIsEquipped) const
{
    if (bIsEquipped)
    {
        return HandSocketName;
    }

    switch (SlotType)
    {
    case EEquipmentSlotType::MainWeapon1Slot:
        return Main1BackSocketName;
    case EEquipmentSlotType::MainWeapon2Slot:
        return Main2BackSocketName;
    default:
        return SubBackSocketName;
    }
}
