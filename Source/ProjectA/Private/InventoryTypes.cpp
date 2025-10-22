// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryTypes.h"
#include "ATGInventoryComponent.h"
#include "ATGItemData.h"

void FInventoryEntry::PreReplicatedRemove(const FInventoryList& InArraySerializer)
{
    if (InArraySerializer.OwnerComp)
        InArraySerializer.OwnerComp->OnItemRemoved.Broadcast(*this);
}

void FInventoryEntry::PostReplicatedAdd(const FInventoryList& InArraySerializer)
{
    if (InArraySerializer.OwnerComp)
        InArraySerializer.OwnerComp->OnItemAdded.Broadcast(*this);
}

void FInventoryEntry::PostReplicatedChange(const FInventoryList& InArraySerializer)
{
    if (InArraySerializer.OwnerComp)
        InArraySerializer.OwnerComp->OnItemChanged.Broadcast(*this);
}

void FInventoryList::AddOrStack(UATGItemData* Def, int32 Qty)
{
    AddOrStack(TSoftObjectPtr<UATGItemData>(Def), Qty);
}

bool FInventoryList::RemoveItem(UATGItemData* Def, int32 Qty)
{
    return RemoveItem(TSoftObjectPtr<UATGItemData>(Def), Qty);
}

void FInventoryList::AddOrStack(const TSoftObjectPtr<UATGItemData>& ItemDef, int32 Quantity)
{
    if (ItemDef.IsNull() || Quantity <= 0) return;

    int32 Index = Entries.IndexOfByPredicate([&](const FInventoryEntry& E) { return E.Item == ItemDef; });
    if (Index != INDEX_NONE)
    {
        Entries[Index].Quantity += Quantity;
        MarkItemDirty(Entries[Index]);
        return;
    }

    FInventoryEntry NewEntry;
    NewEntry.Item = ItemDef;
    NewEntry.Quantity = Quantity;
    Entries.Add(NewEntry);
    MarkItemDirty(Entries.Last());
}

bool FInventoryList::RemoveItem(const TSoftObjectPtr<UATGItemData>& ItemDef, int32 Quantity)
{
    int32 Index = Entries.IndexOfByPredicate([&](const FInventoryEntry& E) { return E.Item == ItemDef; });
    if (Index == INDEX_NONE) return false;

    FInventoryEntry& Entry = Entries[Index];
    Entry.Quantity -= Quantity;
    if (Entry.Quantity <= 0)
    {
        Entries.RemoveAt(Index);
        MarkArrayDirty();
    }
    else
    {
        MarkItemDirty(Entry);
    }
    return true;
}