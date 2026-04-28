// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/ATGItemData.h"
#include "GameplayEffect.h"
#include "ATGEquipmentData.generated.h"

UCLASS()
class ATGGRIDINVENTORY_API UATGEquipmentData : public UATGItemData
{
	GENERATED_BODY()

public:

	UATGEquipmentData();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EEquipmentType EquipmentType = EEquipmentType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Durability = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> EffectClass;
};
