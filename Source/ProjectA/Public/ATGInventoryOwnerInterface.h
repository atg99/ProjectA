// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ATGInventoryOwnerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UATGInventoryOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PROJECTA_API IATGInventoryOwnerInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

    virtual void ItemRemoved(int32 EntryId) {}

    virtual void ItemAdded(int32 EntryId) {}

	virtual void ItemChanged(int32 EntryId) {}

	virtual void InventoryForceNetUpdate() {}

	virtual bool IsLocallyOwned() { return false; }

	virtual const TArray<struct FInventoryEntry>& GetEntries() = 0;

	virtual int32 GetGridWidth() const = 0;

	virtual int32 GetGridHeight() const = 0;

	virtual void TryDropItem(int32 EntryId, int32 SplitNum = -1) {}

	virtual void TryMoveOrSwapClient(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate) {}

	virtual void TrySplitStack(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate, int32 SplitNum) {}

	virtual const struct FInventoryGrid& GetInventory() = 0;

	virtual bool CheckCanMove(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId = -1) = 0;

	virtual void TrySortByItemId() {}

	virtual void TryAddItemAt(TSoftObjectPtr<class UATGItemData> ItemDef, int32 Qty, int32 X, int32 Y, bool bRotate = false, TScriptInterface<IATGInventoryOwnerInterface> Inven = nullptr) {}
};
