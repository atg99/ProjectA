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
	None			UMETA(DisplayName = "None"),
	Equipment		UMETA(DisplayName = "Equipment"),
	Consumables		UMETA(DisplayName = "Consumables"),
	Materials 		UMETA(DisplayName = "Materials"),
};

UENUM(BlueprintType)
enum class EEquipmentType : uint8
{
	None		UMETA(DisplayName = "None"),
	Weapon		UMETA(DisplayName = "Weapon"),
	Armor		UMETA(DisplayName = "Armor"),
};

UENUM(BlueprintType)
enum class EInteractionType : uint8
{
	None			UMETA(DisplayName = "None"),
	PickUpItem		UMETA(DisplayName = "PickUpItem"),
	ItemGridBox		UMETA(DisplayName = "OpenItemGrid"),
	Equipment		UMETA(DisplayName = "Equipment"),
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

