// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ATGInventoryEnums.h"
#include "ATGInventoryInterface.h"
#include "ATGInventoryOwnerInterface.generated.h"

class UATGItemData;
struct FInventoryEntry;
struct FInventoryGrid;
// FATGItemInfo is fully defined in ATGInventoryInterface.h (included above)

UINTERFACE(MinimalAPI, BlueprintType, NotBlueprintable)
class UATGInventoryOwnerInterface : public UInterface
{
    GENERATED_BODY()
};

class ATGGRIDINVENTORY_API IATGInventoryOwnerInterface
{
    GENERATED_BODY()

public:

    virtual void ItemRemoved(int32 EntryId) {}
    virtual void ItemAdded(int32 EntryId) {}
    virtual void ItemChanged(int32 EntryId) {}
    virtual void InventoryForceNetUpdate() {}
    virtual bool IsLocallyOwned() { return false; }
    virtual const TArray<struct FInventoryEntry>& GetEntries();
    virtual void GetEquipmentEntry(EEquipmentSlotType EquipSlotType, FInventoryEntry& OutEntry) {}
    virtual int32 GetGridWidth() const { return 0; }
    virtual int32 GetGridHeight() const { return 0; }

    UFUNCTION(BlueprintCallable)
    virtual FIntPoint GetGridSize() { return FIntPoint(0, 0); }

    UFUNCTION(BlueprintCallable)
    virtual void IncreaseGridSize(int32 W, int32 H) {}

    UFUNCTION(BlueprintCallable)
    virtual void DecreaseGridSize(int32 W, int32 H) {}

    UFUNCTION(BlueprintCallable)
    virtual void SetGridSize(int32 W, int32 H) {}

    UFUNCTION(BlueprintCallable)
    virtual void OpenContextMenu(FATGItemInfo& ItemInfo, FVector2D ScreenPosition) {}

    virtual void TryDropItem(int32 EntryId, int32 SplitNum = -1) {}
    virtual void TryMoveOrSwapClient(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate) {}
    virtual void TrySplitStack(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate, int32 SplitNum) {}
    virtual const struct FInventoryGrid& GetInventory();
    virtual bool CheckCanMove(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId = -1) { return false; }

    UFUNCTION(BlueprintCallable)
    virtual void TrySortByItemId() {}

    virtual void TryAddItemAt(TScriptInterface<IATGInventoryOwnerInterface> Inven, int32 OtherGridId, TSoftObjectPtr<UATGItemData> ItemDef, int32 Qty, int32 X, int32 Y, bool bRotate = false) {}
    virtual void TryHandleTransItemResult(int32 EntryId, int32 RemoveQty = -1) {}
};
