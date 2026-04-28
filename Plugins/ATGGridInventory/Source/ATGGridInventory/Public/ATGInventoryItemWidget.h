// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ATGInventoryInterface.h"
#include "ATGInventoryOwnerInterface.h"
#include "InventoryTypes.h"
#include "ATGInventoryItemWidget.generated.h"

class UImage;
class UTextBlock;
class USizeBox;
class UATGItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemPressed, const FATGItemInfo&, ItemInfo);

UCLASS()
class ATGGRIDINVENTORY_API UATGInventoryItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TScriptInterface<IATGInventoryOwnerInterface> Inven = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 EntryId = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 DBId = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 Quantity = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bIsRotated = false;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UATGItemData> ItemDef;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TObjectPtr<UATGItemData> ItemData = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuantityText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> RootSizeBox = nullptr;

	UPROPERTY(BlueprintAssignable)
	FOnItemPressed OnItemPressed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanDrag = true;

	UPROPERTY(BlueprintReadOnly)
	bool bIsLock = false;

protected:
	FInventoryEntry CachedEntry;
	int32 CachedCellSize = 0;
	int32 CachedCellPadding = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor LockColor;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetupFromEntry(const TScriptInterface<IATGInventoryOwnerInterface> InInven, const FInventoryEntry& InEntry, int32 InCellSize, int32 InCellPadding);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshFromEntry(const FInventoryEntry& InEntry, int32 InCellSize, int32 InCellPadding);

	UFUNCTION(BlueprintCallable)
	void SetLockItem(bool bInIsLock);

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeo, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeo, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
};
