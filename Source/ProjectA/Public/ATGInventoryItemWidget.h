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

USTRUCT(BlueprintType)
struct FATGItemInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UATGItemData> ItemDef;

	UPROPERTY(BlueprintReadOnly)
	int32 EntryId = -1;

	UPROPERTY(BlueprintReadOnly)
	int32 Quantity = -1;

	UPROPERTY(BlueprintReadOnly)
	bool bIsRotated = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemPressed, const FATGItemInfo&, ItemInfo);

/**
 * item widget
 */
UCLASS()
class PROJECTA_API UATGInventoryItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// 인벤토리 소스
	UPROPERTY()
	TScriptInterface<IATGInventoryOwnerInterface> Inven = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 EntryId = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Quantity = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bIsRotated = false;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UATGItemData> ItemDef;

	//UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	//UATGInventoryComponent* InventoryComp = nullptr;

	UPROPERTY(meta = (BindWidgetOptional)) 
	UImage* ItemIcon = nullptr;
	UPROPERTY(meta = (BindWidgetOptional)) 
	UTextBlock* QuantityText = nullptr;

	UPROPERTY(meta = (BindWidget)) 
	class USizeBox* RootSizeBox;

	UPROPERTY(BlueprintAssignable)
	FOnItemPressed OnItemPressed;

protected:
	FInventoryEntry CachedEntry;

	int32 CachedCellSize;

	int32 CachedCellPadding;

public:
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetupFromEntry(const TScriptInterface<IATGInventoryOwnerInterface> InInven, const FInventoryEntry& InEntry, int32 InCellSize, int32 InCellPadding);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshFromEntry(const FInventoryEntry& InEntry, int32 InCellSize, int32 InCellPadding);

	//UFUNCTION(BlueprintCallable, Category = "Inventory")
	//void SetQuantityText(int32 Qty);

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeo, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeo, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	
};
