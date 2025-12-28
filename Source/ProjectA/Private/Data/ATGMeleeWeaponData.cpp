// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/ATGMeleeWeaponData.h"
#include "Weapon/ATGMeleeWeapon.h"

UATGMeleeWeaponData::UATGMeleeWeaponData()
{

	EquipmentType = EEquipmentType::Weapon;

}

TSubclassOf<class AATGWeaponBase> UATGMeleeWeaponData::GetWeaponClass() const
{
	return WeaponClass;
}
