// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATGInventoryEnums.generated.h"

// -------------------------------------------------------
//  Item / Equipment category enums
// -------------------------------------------------------

UENUM(BlueprintType)
enum class EItemType : uint8
{
	None		=0		UMETA(DisplayName = "None"),
	Equipment	=10		UMETA(DisplayName = "Equipment"),
	Consumables	=20		UMETA(DisplayName = "Consumables"),
	Materials 	=30		UMETA(DisplayName = "Materials"),
};

UENUM(BlueprintType)
enum class EEquipmentType : uint8
{
	None	=0		UMETA(DisplayName = "None"),
	Weapon	=10		UMETA(DisplayName = "Weapon"),
	Armor	=20		UMETA(DisplayName = "Armor"),
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	None		= 0		UMETA(DisplayName = "None"),
	MainWeapon  = 10	UMETA(DisplayName = "MainWeapon"),
	SubWeapon   = 20	UMETA(DisplayName = "SubWeapon"),
};

UENUM(BlueprintType)
enum class EWeaponCategory : uint8
{
	None = 0			UMETA(DisplayName = "None"),
	GunWeapon = 1		UMETA(DisplayName = "GunWeapon"),
	MeleeWeapon = 2		UMETA(DisplayName = "MeleeWeapon"),
};

UENUM(BlueprintType)
enum class EPlayerAbilityInputID : uint8
{
	None,
	Confirm,
	Cancel,
	MeleeAttack,
};

UENUM(BlueprintType)
enum class EEquipmentSlotType : uint8
{
	None			= 0		UMETA(DisplayName = "None"),
	MainWeapon1Slot	= 1		UMETA(DisplayName = "MainWeapon1Slot"),
	MainWeapon2Slot	= 2		UMETA(DisplayName = "MainWeapon2Slot"),
};
ENUM_RANGE_BY_FIRST_AND_LAST(EEquipmentSlotType, EEquipmentSlotType::None, EEquipmentSlotType::MainWeapon2Slot);

// -------------------------------------------------------
//  Interaction (used by PickupComponent)
// -------------------------------------------------------

UENUM(BlueprintType)
enum class EInteractionType : uint8
{
	None		UMETA(DisplayName = "None"),
	PickUpItem	UMETA(DisplayName = "PickUpItem"),
	ItemGridBox	UMETA(DisplayName = "OpenItemGrid"),
	Equipment	UMETA(DisplayName = "Equipment"),
};

// FInteractionData, FATGItemInfo 는 UATGItemData/UATGInventoryItemWidget 이동 후 함께 이동
