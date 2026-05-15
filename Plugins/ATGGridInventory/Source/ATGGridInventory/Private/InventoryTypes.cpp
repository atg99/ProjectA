// Fill out your copyright notice in the Description page of Project Settings.

#include "InventoryTypes.h"
#include "ATGInventoryOwnerInterface.h"
#include "Data/ATGItemData.h"
#include "Algo/Sort.h"

int32 FInventoryGrid::GlobalEntryIdCounter = 0;

void FInventoryEntry::PreReplicatedRemove(const FInventoryGrid& InArraySerializer)
{
    if (InArraySerializer.Owner)
        InArraySerializer.Owner->ItemRemoved(Id);
}

void FInventoryEntry::PostReplicatedAdd(const FInventoryGrid& InArraySerializer)
{
    if (InArraySerializer.Owner)
    {
        InArraySerializer.Owner->ItemAdded(Id);
    }
}

void FInventoryEntry::PostReplicatedChange(const FInventoryGrid& InArraySerializer)
{
    if (InArraySerializer.Owner)
        InArraySerializer.Owner->ItemChanged(Id);
}

bool FInventoryGrid::CanPlaceRect(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId) const
{
    if (StartX < 0 || StartY < 0 || StartX + W > GridWidth || StartY + H > GridHeight)
        return false;

    for (const auto& E : Entries)
    {
        if (E.Id == IgnoreId) continue;

        const int32 EX2 = E.X + E.Width - 1;
        const int32 EY2 = E.Y + E.Height - 1;
        const int32 NX2 = StartX + W - 1;
        const int32 NY2 = StartY + H - 1;

        const bool bOverlap = !(NX2 < E.X || EX2 < StartX || NY2 < E.Y || EY2 < StartY);
        if (bOverlap) return false;
    }
    return true;
}

bool FInventoryGrid::SortCanPlaceRect(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId) const
{
    if (StartX < 0 || StartY < 0 || StartX + W > GridWidth || StartY + H > GridHeight)
        return false;

    for (const auto& E : TempEntries)
    {
        if (E->Id == IgnoreId) continue;

        const int32 EX2 = E->X + E->Width - 1;
        const int32 EY2 = E->Y + E->Height - 1;
        const int32 NX2 = StartX + W - 1;
        const int32 NY2 = StartY + H - 1;

        const bool bOverlap = !(NX2 < E->X || EX2 < StartX || NY2 < E->Y || EY2 < StartY);
        if (bOverlap) return false;
    }
    return true;
}

bool FInventoryGrid::FindFirstFit(int32 W, int32 H, int32& OutX, int32& OutY, int32 IgnoreId)
{
    for (int32 y = 0; y <= GridHeight - H; ++y)
        for (int32 x = 0; x <= GridWidth - W; ++x)
            if (CanPlaceRect(x, y, W, H, IgnoreId)) { OutX = x; OutY = y; return true; }
    return false;
}

bool FInventoryGrid::FindFirstFit(TSoftObjectPtr<UATGItemData> ItemDef, int32 W, int32 H, int32& OutX, int32& OutY, int32& Qty, int32 IgnoreId)
{
    Qty = FindAddFitStack(ItemDef, Qty, IgnoreId);
    for (int32 y = 0; y <= GridHeight - H; ++y)
        for (int32 x = 0; x <= GridWidth - W; ++x)
            if (CanPlaceRect(x, y, W, H, IgnoreId)) { OutX = x; OutY = y; return true; }
    return false;
}

int32 FInventoryGrid::FindAddFitStack(TSoftObjectPtr<UATGItemData> ItemDef, int32 Qty, int32 IgnoreId)
{
    if (!ItemDef.Get() && !ItemDef.LoadSynchronous())
        return Qty;

    int32 RemainQty = Qty;
    for (auto& E : Entries)
    {
        if (E.Id == IgnoreId || RemainQty == 0) continue;
        if (E.Item.IsNull()) { UE_LOG(LogTemp, Error, TEXT("%s Item.IsNull()"), ANSI_TO_TCHAR(__FUNCTION__)); continue; }
        if (!E.Item.IsValid() && !E.Item.LoadSynchronous()) continue;

        int32 RemainCapacity = E.Item->MaxStack - E.Quantity;
        if (E.Item->ItemId == ItemDef->ItemId && RemainCapacity >= 1)
        {
            int32 AddedQty = FMath::Min(RemainCapacity, RemainQty);
            RemainQty -= AddedQty;
            E.Quantity += AddedQty;
            if (Owner)
            {
                Owner->ItemChanged(E.Id);
                if (OwnerHasAuthority()) MarkItemDirty(E);
            }
        }
    }
    return RemainQty;
}

bool FInventoryGrid::SortFindFirstFit(TSoftObjectPtr<UATGItemData> ItemDef, int32 W, int32 H, int32& OutX, int32& OutY, int32& Qty, int32 IgnoreId)
{
    Qty = SortFindAddFitStack(ItemDef, Qty, IgnoreId);
    for (int32 y = 0; y <= GridHeight - H; ++y)
        for (int32 x = 0; x <= GridWidth - W; ++x)
            if (SortCanPlaceRect(x, y, W, H)) { OutX = x; OutY = y; return true; }
    return false;
}

int32 FInventoryGrid::SortFindAddFitStack(TSoftObjectPtr<UATGItemData> ItemDef, int32 Qty, int32 IgnoreId)
{
    if (!ItemDef.Get() && !ItemDef.LoadSynchronous())
        return Qty;

    int32 RemainQty = Qty;
    for (auto E : TempEntries)
    {
        if (!E || E->Id == IgnoreId || RemainQty == 0) continue;
        if (E->Item.IsNull()) { UE_LOG(LogTemp, Error, TEXT("%s E->Item.IsNull()"), ANSI_TO_TCHAR(__FUNCTION__)); continue; }
        if (!E->Item.IsValid() && !E->Item.LoadSynchronous()) continue;

        int32 RemainCapacity = E->Item->MaxStack - E->Quantity;
        if (E->Item->ItemId == ItemDef->ItemId && RemainCapacity >= 1)
        {
            int32 AddedQty = FMath::Min(RemainCapacity, RemainQty);
            RemainQty -= AddedQty;
            E->Quantity += AddedQty;
            MarkItemDirty(*E);
            if (Owner) Owner->ItemChanged(E->Id);
        }
    }
    return RemainQty;
}

const FInventoryEntry* FInventoryGrid::GetById(int32 EntryId) const
{
    return Entries.FindByPredicate([&](const FInventoryEntry& E) { return E.Id == EntryId; });
}

FInventoryEntry* FInventoryGrid::GetById(int32 EntryId)
{
    return Entries.FindByPredicate([&](FInventoryEntry& E) { return E.Id == EntryId; });
}

int32 FInventoryGrid::AddItemAt(TSoftObjectPtr<UATGItemData> ItemDef, int32& Qty, int32 X, int32 Y, int32 W, int32 H, bool bRotated, int32 PreKey, int32 DBId)
{
    UE_LOG(LogTemp, Warning, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));
    if (!ItemDef.Get() && !ItemDef.LoadSynchronous()) return 0;
    if (!ItemDef || Qty <= 0) return 0;
    if (!CanPlaceRect(X, Y, bRotated ? H : W, bRotated ? W : H)) return 0;

    FInventoryEntry NewE;
    NewE.Item = ItemDef;
    int32 QtyStack = FMath::Min(ItemDef->MaxStack, Qty);
    Qty = Qty - QtyStack;
    NewE.Quantity = QtyStack;
    NewE.X = X; NewE.Y = Y;
    NewE.Width = bRotated ? H : W;
    NewE.Height = bRotated ? W : H;
    NewE.bRotated = bRotated;
    NewE.DBId = DBId;

    if (Owner && OwnerHasAuthority())
    {
        NewE.Id = ++GlobalEntryIdCounter;
        NewE.PredictionKey = PreKey;
        Entries.Add(NewE);
        MarkItemDirty(Entries.Last());
        MarkArrayDirty();
        Owner->InventoryForceNetUpdate();
        Owner->ItemAdded(NewE.Id);
        return NewE.Id;
    }
    else if (Owner && !OwnerHasAuthority())
    {
        NewE.Id = PreKey;
        return NewE.Id;
    }
    return 0;
}

bool FInventoryGrid::MoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
    FInventoryEntry* Me = GetById(EntryId);
    if (!Me) return false;

    const int32 NewW = bIsRotate ? Me->Height : Me->Width;
    const int32 NewH = bIsRotate ? Me->Width : Me->Height;

    if (CanPlaceRect(NewX, NewY, NewW, NewH, Me->Id))
    {
        Me->X = NewX; Me->Y = NewY;
        Me->Width = NewW; Me->Height = NewH;
        if (bIsRotate) Me->bRotated = !Me->bRotated;
        if (Owner && OwnerHasAuthority())
        {
            MarkItemDirty(*Me);
            Owner->ItemChanged(EntryId);
            Owner->InventoryForceNetUpdate();
        }
        return true;
    }

    FInventoryEntry* Other = nullptr;
    for (auto& E : Entries)
    {
        if (E.Id == Me->Id) continue;
        const bool bHit = !(NewX + NewW - 1 < E.X || E.X + E.Width - 1 < NewX || NewY + NewH - 1 < E.Y || E.Y + E.Height - 1 < NewY);
        if (bHit) { Other = &E; break; }
    }
    if (!Other) return false;

    if (!Me->Item.IsValid()) Me->Item.LoadSynchronous();
    if (!Other->Item.IsValid()) Other->Item.LoadSynchronous();
    if (!Me->Item.IsValid() || !Other->Item.IsValid()) return false;

    if (Other->Item->ItemId == Me->Item->ItemId)
    {
        const int32 Remaining = Other->Item->MaxStack - Other->Quantity;
        if (Remaining > 0)
        {
            const int32 StackNum = FMath::Min(Remaining, Me->Quantity);
            IncreaseQtyByRef(*Other, StackNum);
            DecreaseQtyByRef(*Me, StackNum);
            return true;
        }
    }

    const bool bOtherFitInMy = CanPlaceRect(Me->X, Me->Y, Other->Width, Other->Height, Me->Id);
    const bool bMeFitInNew = CanPlaceRect(NewX, NewY, NewW, NewH, Other->Id);

    if (bMeFitInNew && bOtherFitInMy)
    {
        Other->X = Me->X; Other->Y = Me->Y;
        Me->X = NewX; Me->Y = NewY;
        Me->Width = NewW; Me->Height = NewH;
        if (bIsRotate) Me->bRotated = !Me->bRotated;
        if (Owner && OwnerHasAuthority())
        {
            MarkItemDirty(*Other); MarkItemDirty(*Me);
            Owner->ItemChanged(EntryId); Owner->ItemChanged(Other->Id);
            Owner->InventoryForceNetUpdate();
        }
        return true;
    }
    return false;
}

bool FInventoryGrid::SortMove(int32 EntryId, int32 NewX, int32 NewY)
{
    FInventoryEntry* Me = GetById(EntryId);
    if (!Me) return false;
    if (SortCanPlaceRect(NewX, NewY, Me->Width, Me->Height, Me->Id))
    {
        Me->X = NewX; Me->Y = NewY;
        MarkItemDirty(*Me);
        if (Owner) Owner->ItemChanged(EntryId);
        return true;
    }
    return false;
}

bool FInventoryGrid::CheckMoveOrSwap(int32 StartX, int32 StartY, int32 W, int32 H, int32 Id)
{
    if (CanPlaceRect(StartX, StartY, W, H, Id)) return true;

    FInventoryEntry* Me = GetById(Id);
    if (!Me) return false;

    FInventoryEntry* Other = nullptr;
    for (auto& E : Entries)
    {
        if (E.Id == Id) continue;
        const bool bHit = !(StartX + W - 1 < E.X || E.X + E.Width - 1 < StartX || StartY + H - 1 < E.Y || E.Y + E.Height - 1 < StartY);
        if (bHit) { Other = &E; break; }
    }
    if (!Other) return false;

    return CanPlaceRect(Me->X, Me->Y, Other->Width, Other->Height, Id) && CanPlaceRect(StartX, StartY, W, H, Other->Id);
}

bool FInventoryGrid::MergeStackAtAndDecrease(FInventoryEntry& Entry, int32 Qty, int32 NewX, int32 NewY, bool bIsRotate)
{
    const int32 NewW = bIsRotate ? Entry.Height : Entry.Width;
    const int32 NewH = bIsRotate ? Entry.Width : Entry.Height;

    FInventoryEntry* Other = nullptr;
    for (auto& E : Entries)
    {
        if (E.Id == Entry.Id) continue;
        const bool bHit = !(NewX + NewW - 1 < E.X || E.X + E.Width - 1 < NewX || NewY + NewH - 1 < E.Y || E.Y + E.Height - 1 < NewY);
        if (bHit) { Other = &E; break; }
    }
    if (!Other) return false;

    if (Entry.Item.IsNull() || Other->Item.IsNull()) return false;
    if (!Entry.Item.IsValid() && !Entry.Item.LoadSynchronous()) return false;
    if (!Other->Item.IsValid() && !Other->Item.LoadSynchronous()) return false;

    if (Other->Item->ItemId == Entry.Item->ItemId)
    {
        const int32 Remain = Other->Item->MaxStack - Other->Quantity;
        if (Remain <= 0) return false;
        const int32 StackNum = FMath::Min(Remain, Qty);
        IncreaseQtyByRef(*Other, StackNum);
        DecreaseQtyByRef(Entry, StackNum);
        return true;
    }
    return false;
}

bool FInventoryGrid::Rotate(int32 EntryId)
{
    FInventoryEntry* Me = GetById(EntryId);
    if (!Me) return false;

    const int32 NewW = Me->Height, NewH = Me->Width;
    if (CanPlaceRect(Me->X, Me->Y, NewW, NewH, Me->Id))
    {
        Me->Width = NewW; Me->Height = NewH; Me->bRotated = !Me->bRotated;
        MarkItemDirty(*Me);
        if (Owner) Owner->ItemChanged(EntryId);
        return true;
    }
    int32 Fx, Fy;
    if (FindFirstFit(NewW, NewH, Fx, Fy, Me->Id))
    {
        Me->X = Fx; Me->Y = Fy; Me->Width = NewW; Me->Height = NewH; Me->bRotated = !Me->bRotated;
        MarkItemDirty(*Me);
        if (Owner) Owner->ItemChanged(EntryId);
        return true;
    }
    return false;
}

bool FInventoryGrid::RemoveById(int32 EntryId)
{
    const int32 Idx = Entries.IndexOfByPredicate([&](const FInventoryEntry& E) { return E.Id == EntryId; });
    if (Idx == INDEX_NONE) return false;
    Entries.RemoveAt(Idx);
    MarkArrayDirty();
    if (Owner) { Owner->ItemRemoved(EntryId); Owner->InventoryForceNetUpdate(); }
    return true;
}

bool FInventoryGrid::DecreaseQtyAndRemoveById(int32 EntryId, int32 Num)
{
    FInventoryEntry* E = GetById(EntryId);
    if (!E) return false;
    if (E->Quantity - Num > 0)
    {
        E->Quantity -= Num;
        if (Owner) { Owner->ItemChanged(E->Id); if (OwnerHasAuthority()) { MarkItemDirty(*E); Owner->InventoryForceNetUpdate(); } }
        return true;
    }
    RemoveById(EntryId);
    return false;
}

bool FInventoryGrid::DecreaseQtyByRef(FInventoryEntry& E, int32 Num)
{
    if (E.Quantity - Num > 0)
    {
        E.Quantity -= Num;
        if (Owner) { Owner->ItemChanged(E.Id); if (OwnerHasAuthority()) { MarkItemDirty(E); Owner->InventoryForceNetUpdate(); } }
        return true;
    }
    RemoveById(E.Id);
    return false;
}

bool FInventoryGrid::IncreaseQtyById(int32 EntryId, int32 Num)
{
    if (Num <= 0) return false;
    FInventoryEntry* E = GetById(EntryId);
    if (!E) return false;
    if (E->Quantity + Num <= E->Item->MaxStack)
    {
        E->Quantity += Num;
        if (Owner) { Owner->ItemChanged(E->Id); if (OwnerHasAuthority()) { MarkItemDirty(*E); Owner->InventoryForceNetUpdate(); } }
        return true;
    }
    return false;
}

bool FInventoryGrid::IncreaseQtyByRef(FInventoryEntry& E, int32 Num)
{
    if (Num <= 0) return false;
    if (!E.Item.IsValid() && !E.Item.LoadSynchronous()) return false;
    if (E.Quantity + Num <= E.Item->MaxStack)
    {
        E.Quantity += Num;
        if (Owner) { Owner->ItemChanged(E.Id); if (OwnerHasAuthority()) { MarkItemDirty(E); Owner->InventoryForceNetUpdate(); } }
        return true;
    }
    return false;
}

bool FInventoryGrid::PreviewRemoveById(int32 PreviewId)
{
    return false;
}

bool FInventoryGrid::PreviewMoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
    const FInventoryEntry* Me = GetById(EntryId);
    if (!Me || !Owner) return false;

    const int32 NewW = bIsRotate ? Me->Height : Me->Width;
    const int32 NewH = bIsRotate ? Me->Width : Me->Height;

    if (CanPlaceRect(NewX, NewY, NewW, NewH, Me->Id))
    {
        PreviewRemoveById(Me->Id);
        FInventoryEntry Pre = *Me;
        Pre.X = NewX; Pre.Y = NewY; Pre.Width = NewW; Pre.Height = NewH; Pre.bRotated = bIsRotate;
        PreviewEntries.Add(Pre);
        return true;
    }

    const FInventoryEntry* Other = nullptr;
    const int32 NX2 = NewX + NewW - 1, NY2 = NewY + NewH - 1;
    for (const auto& E : Entries)
    {
        if (E.Id == Me->Id) continue;
        const bool bOverlap = !(NX2 < E.X || E.X + E.Width - 1 < NewX || NY2 < E.Y || E.Y + E.Height - 1 < NewY);
        if (bOverlap) { Other = &E; break; }
    }
    if (!Other) return false;

    const bool bMeFitInNew = CanPlaceRect(NewX, NewY, NewW, NewH, Other->Id);
    const bool bOtherFitInMy = CanPlaceRect(Me->X, Me->Y, Other->Width, Other->Height, Me->Id);
    if (!bMeFitInNew || !bOtherFitInMy) return false;

    PreviewRemoveById(Me->Id); PreviewRemoveById(Other->Id);
    FInventoryEntry PreA = *Me; PreA.X = NewX; PreA.Y = NewY; PreA.Width = NewW; PreA.Height = NewH; PreA.bRotated = bIsRotate;
    PreviewEntries.Add(PreA);
    FInventoryEntry PreB = *Other; PreB.X = Me->X; PreB.Y = Me->Y;
    PreviewEntries.Add(PreB);
    return true;
}

bool FInventoryGrid::OwnerHasAuthority()
{
    if (!Owner.GetObject()) return false;
    UActorComponent* OwnerComp = Cast<UActorComponent>(Owner.GetObject());
    if (!OwnerComp) return false;
    return OwnerComp->GetOwner()->HasAuthority();
}

void FInventoryGrid::SortEntryByItemId()
{
    if (!Owner || !OwnerHasAuthority()) return;

    TempSortEntries.Empty();
    TempEntries.Empty();

    for (auto& Entry : Entries)
    {
        if (Entry.Item.IsNull()) continue;
        if (!Entry.Item.IsValid() && !Entry.Item.LoadSynchronous()) continue;
        TempSortEntries.Add(&Entry);
    }

    Algo::StableSort(TempSortEntries, [](const FInventoryEntry* A, const FInventoryEntry* B) {
        if (!A || !B) return false;
        return A->Item->ItemId < B->Item->ItemId;
    });

    TArray<int32> DeleteIds;
    for (FInventoryEntry* Entry : TempSortEntries)
    {
        int32 OutX = -1, OutY = -1, RemainQty = Entry->Quantity;
        SortFindFirstFit(Entry->Item, Entry->Width, Entry->Height, OutX, OutY, RemainQty, Entry->Id);
        if (RemainQty <= 0) { DeleteIds.Add(Entry->Id); continue; }
        Entry->Quantity = RemainQty;
        if (SortMove(Entry->Id, OutX, OutY)) TempEntries.Add(Entry);
    }

    UObject* OwnerObj = Owner.GetObject();
    if (!OwnerObj) return;

    TWeakObjectPtr<UObject> WeakOwner(OwnerObj);
    OwnerObj->GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([this, DeleteIds, WeakOwner]() {
        if (!WeakOwner.IsValid()) return;
        if (DeleteIds.Num() > 0)
        {
            Entries.RemoveAll([&](const FInventoryEntry& Val) { return DeleteIds.Contains(Val.Id); });
            MarkArrayDirty();
            if (Owner) { for (const int32 DId : DeleteIds) Owner->ItemRemoved(DId); Owner->InventoryForceNetUpdate(); }
        }
        else { MarkArrayDirty(); }
    }));

    TempEntries.Empty();
}
