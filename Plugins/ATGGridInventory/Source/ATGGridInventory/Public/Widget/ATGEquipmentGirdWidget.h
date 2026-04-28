// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ATGInventoryGirdWidget.h"
#include "ATGEquipmentGirdWidget.generated.h"

class UATGEquipmentComponent;

UCLASS()
class ATGGRIDINVENTORY_API UATGEquipmentGirdWidget : public UATGInventoryGirdWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EEquipmentSlotType EquipmentSlot = EEquipmentSlotType::None;

	virtual void NativeConstruct() override;
	virtual void BindInventoryComp() override;
	virtual void RebuildAll() override;
	virtual void BuildCellBackground() override;
	virtual void HandleIncomingItem(UDragDropOperation* InOperation, UATGInventoryItemWidget* InDragged, FVector2D Screen) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool CheckIsFromOther(UATGInventoryItemWidget* Dragged) override;

	UFUNCTION()
	void HandleEquipmentChanged(FInventoryEntry EquipmentEntry);

protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UATGInventoryItemWidget> EquipmentWidget = nullptr;

	EEquipmentType FitEquipmentType = EEquipmentType::None;

	bool CheckFitEquip(UATGItemData* ItemData);
};
