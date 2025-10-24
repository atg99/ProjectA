// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryTypes.h"
#include "ATGInventoryItemWidget.generated.h"

class UImage;
class UTextBlock;
class UATGInventoryComponent;
class USizeBox;

/**
 * item widget
 */
UCLASS()
class PROJECTA_API UATGInventoryItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 EntryId = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	UATGInventoryComponent* InventoryComp = nullptr;

	UPROPERTY(meta = (BindWidgetOptional)) 
	UImage* ItemIcon = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) 
	UTextBlock* QuantityText = nullptr;

	UPROPERTY(meta = (BindWidget)) 
	class USizeBox* RootSizeBox;

protected:
	FInventoryEntry CachedEntry;

	int32 CachedCellSize;

	int32 CachedCellPadding;

public:
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetupFromEntry(const FInventoryEntry& InEntry, UATGInventoryComponent* InComp, int32 InCellSize, int32 InCellPadding);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshFromEntry(const FInventoryEntry& InEntry, int32 InCellSize, int32 InCellPadding);

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeo, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeo, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	
};
