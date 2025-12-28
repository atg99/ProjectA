// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/ATGRangeWeaponData.h"
#include "Weapon/ATGRangeWeapon.h"

UATGRangeWeaponData::UATGRangeWeaponData()
{
}

TSubclassOf<class AATGWeaponBase> UATGRangeWeaponData::GetWeaponClass() const
{
    return WeaponClass;
}
