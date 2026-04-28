// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "InventoryTypes.generated.h"


class UATGItemData;
class UATGInventoryComponent;

struct FGridActionInfo
{
    int32 EntryId; int32 NewX; int32 NewY; bool bIsRotate; int32 SplitNum = -1;
};

USTRUCT(BlueprintType)
struct FClientAddRequest
{
    GENERATED_BODY()

    UPROPERTY()
    TSoftObjectPtr<UATGItemData> ItemDef;

    UPROPERTY()
    int32 PredictionKey = 0;

    UPROPERTY()
    int32 Quantity = 1;

    UPROPERTY()
    int32 X = -1;
    UPROPERTY()
    int32 Y = -1;
    UPROPERTY()
    bool  bRotated = false;
};

UENUM(BlueprintType)
enum class EInventoryChangeStatus : uint8
{
    Success         UMETA(DisplayName = "Success"),
    Rejected        UMETA(DisplayName = "Rejected"),
    PartialSuccess  UMETA(DisplayName = "Partial Success"),
    Error           UMETA(DisplayName = "Error"),
};

UENUM(BlueprintType)
enum class EInventoryRejectReason : uint8
{
    None,
    AlreadyTaken,
    NoSpace,
    StackFull,
    OutOfRange,
    InvalidItem,
    NotOwner,
    RateLimited,
    Unknown,
};

USTRUCT(BlueprintType)
struct FInventoryChangeResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EInventoryChangeStatus Status = EInventoryChangeStatus::Error;

    UPROPERTY(BlueprintReadOnly)
    EInventoryRejectReason Reason = EInventoryRejectReason::None;

    UPROPERTY(BlueprintReadOnly)
    int32 PredictionKey = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 InventoryRevision = 0;

    UPROPERTY(BlueprintReadOnly)
    FPrimaryAssetId ItemDefId;

    UPROPERTY(BlueprintReadOnly)
    TArray<int32> NewEntryIds;

    UPROPERTY(BlueprintReadOnly)
    int32 MergedIntoEntryId = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 X = -1;
    UPROPERTY(BlueprintReadOnly)
    int32 Y = -1;
    UPROPERTY(BlueprintReadOnly)
    int32 W = 1;
    UPROPERTY(BlueprintReadOnly)
    int32 H = 1;
    UPROPERTY(BlueprintReadOnly)
    bool bRotated = false;

    UPROPERTY(BlueprintReadOnly)
    int32 RequestedQuantity = 0;
    UPROPERTY(BlueprintReadOnly)
    int32 GrantedQuantity = 0;
    UPROPERTY(BlueprintReadOnly)
    int32 PreviousQuantity = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bServerAutoPlaced = false;

    UPROPERTY(BlueprintReadOnly)
    int32 SuggestedX = -1;
    UPROPERTY(BlueprintReadOnly)
    int32 SuggestedY = -1;
};

USTRUCT(BlueprintType)
struct FInventoryEntrySaveData
{
    GENERATED_BODY()

    UPROPERTY()
    int32 item_entry_id = -1;

    UPROPERTY()
    FString primary_asset_id;

    UPROPERTY()
    int32 qty = 0;

    UPROPERTY()
    int32 x = 0;

    UPROPERTY()
    int32 y = 0;

    UPROPERTY()
    bool b_rotated = false;
};

USTRUCT(BlueprintType)
struct FInventorySaveData
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FInventoryEntrySaveData> saved_entries;

    UPROPERTY()
    int32 grid_width = 0;

    UPROPERTY()
    int32 grid_height = 0;
};

class IATGInventoryOwnerInterface;

USTRUCT(BlueprintType)
struct FInventoryEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TSoftObjectPtr<UATGItemData> Item = nullptr;

    UPROPERTY(BlueprintReadOnly)
    int32 Quantity = -1;

    UPROPERTY(BlueprintReadWrite)
    int32 X = -1;

    UPROPERTY(BlueprintReadWrite)
    int32 Y = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 Width = -1;

    UPROPERTY(BlueprintReadOnly)
    int32 Height = -1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRotated = true;

    UPROPERTY()
    int32 Id = -1;

    UPROPERTY()
    int32 PredictionKey = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 DBId = -1;

    bool operator==(const FInventoryEntry& Other) const
    {
        return Item == Other.Item &&
            Quantity == Other.Quantity &&
            X == Other.X &&
            Y == Other.Y &&
            Width == Other.Width &&
            Height == Other.Height &&
            bRotated == Other.bRotated &&
            Id == Other.Id &&
            PredictionKey == Other.PredictionKey &&
            DBId == Other.DBId;
    }

    ATGGRIDINVENTORY_API void PreReplicatedRemove(const struct FInventoryGrid& InArraySerializer);
    ATGGRIDINVENTORY_API void PostReplicatedAdd(const struct FInventoryGrid& InArraySerializer);
    ATGGRIDINVENTORY_API void PostReplicatedChange(const struct FInventoryGrid& InArraySerializer);
};

USTRUCT(BlueprintType)
struct FInventoryGrid : public FFastArraySerializer
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FInventoryEntry> Entries;

    TArray<FInventoryEntry*> TempSortEntries;
    TArray<FInventoryEntry*> TempEntries;
    TArray<FInventoryEntry> PreviewEntries;

    TScriptInterface<IATGInventoryOwnerInterface> Owner = nullptr;

    int32 GridWidth = 10;
    int32 GridHeight = 10;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryEntry, FInventoryGrid>(Entries, DeltaParms, *this);
    }

    ATGGRIDINVENTORY_API bool CanPlaceRect(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId = -1) const;
    ATGGRIDINVENTORY_API bool SortCanPlaceRect(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId = -1) const;
    ATGGRIDINVENTORY_API bool FindFirstFit(int32 W, int32 H, int32& OutX, int32& OutY, int32 IgnoreId = -1);
    ATGGRIDINVENTORY_API bool FindFirstFit(TSoftObjectPtr<UATGItemData> ItemDef, int32 W, int32 H, int32& OutX, int32& OutY, int32& Qty, int32 IgnoreId = -1);
    ATGGRIDINVENTORY_API bool SortFindFirstFit(TSoftObjectPtr<UATGItemData> ItemDef, int32 W, int32 H, int32& OutX, int32& OutY, int32& Qty, int32 IgnoreId = -1);
    ATGGRIDINVENTORY_API int32 FindAddFitStack(TSoftObjectPtr<UATGItemData> ItemDef, int32 Qty, int32 IgnoreId = -1);
    ATGGRIDINVENTORY_API int32 SortFindAddFitStack(TSoftObjectPtr<UATGItemData> ItemDef, int32 Qty, int32 IgnoreId = -1);
    ATGGRIDINVENTORY_API int32 AddItemAt(TSoftObjectPtr<UATGItemData> ItemDef, int32& Qty, int32 X, int32 Y, int32 W, int32 H, bool bRotated, int32 PreKey = -1, int32 DBId = -1);
    ATGGRIDINVENTORY_API bool MoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate);
    ATGGRIDINVENTORY_API bool SortMove(int32 EntryId, int32 NewX, int32 NewY);
    ATGGRIDINVENTORY_API bool CheckMoveOrSwap(int32 StartX, int32 StartY, int32 W, int32 H, int32 Id);
    ATGGRIDINVENTORY_API bool MergeStackAtAndDecrease(FInventoryEntry& Entry, int32 Qty, int32 NewX, int32 NewY, bool bIsRotate);
    ATGGRIDINVENTORY_API bool Rotate(int32 EntryId);
    ATGGRIDINVENTORY_API bool RemoveById(int32 EntryId);
    ATGGRIDINVENTORY_API bool DecreaseQtyAndRemoveById(int32 EntryId, int32 Num);
    ATGGRIDINVENTORY_API bool DecreaseQtyByRef(FInventoryEntry& E, int32 Num);
    ATGGRIDINVENTORY_API bool IncreaseQtyById(int32 EntryId, int32 Num);
    ATGGRIDINVENTORY_API bool IncreaseQtyByRef(FInventoryEntry& E, int32 Num);
    ATGGRIDINVENTORY_API bool CheckFitStack(int32 EntryId);
    ATGGRIDINVENTORY_API const FInventoryEntry* GetById(int32 EntryId) const;
    ATGGRIDINVENTORY_API FInventoryEntry* GetById(int32 EntryId);
    ATGGRIDINVENTORY_API bool PreviewRemoveById(int32 EntryId);
    ATGGRIDINVENTORY_API bool PreviewMoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate);
    ATGGRIDINVENTORY_API bool OwnerHasAuthority();

public:
    ATGGRIDINVENTORY_API void SortEntryByItemId();

private:
    ATGGRIDINVENTORY_API static int32 GlobalEntryIdCounter;
};

template<>
struct TStructOpsTypeTraits<FInventoryGrid> : public TStructOpsTypeTraitsBase2<FInventoryGrid>
{
    enum { WithNetDeltaSerializer = true };
};
