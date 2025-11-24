// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATGInventoryGirdWidget.h"
#include "ATGEquipmentGirdWidget.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EEquipmentSlotType : uint8
{
	None			UMETA(DisplayName = "None"),
	MainWeapon1		UMETA(DisplayName = "MainWeapon1"),
	MainWeapon2		UMETA(DisplayName = "MainWeapon2"),
};

class UATGEquipmentComponent;

UCLASS()
class PROJECTA_API UATGEquipmentGirdWidget : public UATGInventoryGirdWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EEquipmentSlotType EquipmentSlot = EEquipmentSlotType::None;

public:
	virtual void NativeConstruct() override;
	virtual void BindInventoryComp() override;

	virtual void RebuildAll() override;

	virtual void BuildCellBackground() override;
};
