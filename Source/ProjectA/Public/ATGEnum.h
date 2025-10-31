// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATGEnum.generated.h"
/**
 * 
 */

UENUM(BlueprintType)
enum class EInteractionType : uint8
{
	Inventory	UMETA(DisplayName = "Inventory"),
	Equipment	UMETA(DisplayName = "Equipment"),
};

class UATGItemData;

USTRUCT(BlueprintType)
struct FInteractionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* InteractedActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EInteractionType InteractionType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UATGItemData> ItemDef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ItemQty;
};

