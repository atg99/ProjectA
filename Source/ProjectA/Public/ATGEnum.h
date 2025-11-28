// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATGEnum.generated.h"
/**
 * 
 */

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
enum class EEquipmentSlotType : uint8
{
	None		=0		UMETA(DisplayName = "None"),
	MainWeapon1	=10		UMETA(DisplayName = "MainWeapon1"),
	MainWeapon2	=20		UMETA(DisplayName = "MainWeapon2"),
};

UENUM(BlueprintType)
enum class EInteractionType : uint8
{
	None			UMETA(DisplayName = "None"),
	PickUpItem		UMETA(DisplayName = "PickUpItem"),
	ItemGridBox		UMETA(DisplayName = "OpenItemGrid"),
	Equipment		UMETA(DisplayName = "Equipment"),
};


UENUM(BlueprintType)
enum class EMonsterState : uint8
{
	None = 0	UMETA(DisplayName = "None"),
	Normal = 10	UMETA(DisplayName = "Normal"),
	Chase = 20	UMETA(DisplayName = "Chase"),
	Battle = 30	UMETA(DisplayName = "Battle"),
	Death = 40	UMETA(DisplayName = "Death"),
};

class UATGItemData;

USTRUCT(BlueprintType)
struct FInteractionData
{
	GENERATED_BODY()

	FInteractionData()
		: InteractedActor(nullptr)
		, InteractedComponent(nullptr)
		, InteractionType(EInteractionType::None)
		, ItemQty(0)
	{
	}

	UPROPERTY(BlueprintReadWrite)
	AActor* InteractingActor;

	UPROPERTY(BlueprintReadWrite)
	AActor* InteractedActor;

	UPROPERTY(BlueprintReadWrite)
	UActorComponent* InteractedComponent;

	UPROPERTY(BlueprintReadWrite)
	EInteractionType InteractionType;

	UPROPERTY(BlueprintReadWrite)
	TSoftObjectPtr<UATGItemData> ItemDef;

	UPROPERTY(BlueprintReadWrite)
	int32 ItemQty;
};

USTRUCT(BlueprintType)
struct FEquipmentSlot
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EEquipmentSlotType SlotType = EEquipmentSlotType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> EquippedActor = nullptr;
};
