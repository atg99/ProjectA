// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryTypes.h"
#include "ATGInventoryComponent.h"
#include "ATGItemData.h"

void FInventoryEntry::PreReplicatedRemove(const FInventoryGrid& InArraySerializer)
{
    if (InArraySerializer.OwnerComp)
        InArraySerializer.OwnerComp->OnItemRemoved.Broadcast(Id);
}

void FInventoryEntry::PostReplicatedAdd(const FInventoryGrid& InArraySerializer)
{
    if (InArraySerializer.OwnerComp)
        InArraySerializer.OwnerComp->OnItemAdded.Broadcast(Id);
}

void FInventoryEntry::PostReplicatedChange(const FInventoryGrid& InArraySerializer)
{
    if (InArraySerializer.OwnerComp)
        InArraySerializer.OwnerComp->OnItemChanged.Broadcast(Id);
}

//void FInventoryGrid::AddOrStack(UATGItemData* Def, int32 Qty)
//{
//    AddOrStack(TSoftObjectPtr<UATGItemData>(Def), Qty);
//}
//
//bool FInventoryGrid::RemoveItem(UATGItemData* Def, int32 Qty)
//{
//    return RemoveItem(TSoftObjectPtr<UATGItemData>(Def), Qty);
//}
//
//void FInventoryGrid::AddOrStack(const TSoftObjectPtr<UATGItemData>& ItemDef, int32 Quantity)
//{
//    if (ItemDef.IsNull() || Quantity <= 0) return;
//
//    int32 Index = Entries.IndexOfByPredicate([&](const FInventoryEntry& E) { return E.Item == ItemDef; });
//    if (Index != INDEX_NONE)
//    {
//        Entries[Index].Quantity += Quantity;
//        MarkItemDirty(Entries[Index]);
//        return;
//    }
//
//    FInventoryEntry NewEntry;
//    NewEntry.Item = ItemDef;
//    NewEntry.Quantity = Quantity;
//    Entries.Add(NewEntry);
//    MarkItemDirty(Entries.Last());
//}
//
//bool FInventoryGrid::RemoveItem(const TSoftObjectPtr<UATGItemData>& ItemDef, int32 Quantity)
//{
//    int32 Index = Entries.IndexOfByPredicate([&](const FInventoryEntry& E) { return E.Item == ItemDef; });
//    if (Index == INDEX_NONE) return false;
//
//    FInventoryEntry& Entry = Entries[Index];
//    Entry.Quantity -= Quantity;
//    if (Entry.Quantity <= 0)
//    {
//        //�迭 ��ü ���� ����˸�
//        Entries.RemoveAt(Index);
//        MarkArrayDirty();
//    }
//    else
//    {
//        //�迭 ��� ���� �˸�
//        MarkItemDirty(Entry);
//    }
//    return true;
//}

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

        //������ �ϳ��� �����ϸ� ��ġ�� ����
        const bool bOverlap = !(NX2 < E.X || EX2 < StartX || NY2 < E.Y || EY2 < StartY);

        if (bOverlap) return false;
    }
    return true;
}

bool FInventoryGrid::FindFirstFit(int32 W, int32 H, int32& OutX, int32& OutY, int32 IgnoreId) const
{
    for (int32 y = 0; y <= GridHeight - H; ++y)
    {
        for (int32 x = 0; x <= GridWidth - W; ++x)
        {
            if (CanPlaceRect(x, y, W, H, IgnoreId))
            {
                OutX = x; OutY = y;
                return true;
            }
        }
    }
    return false;
}

const FInventoryEntry* FInventoryGrid::GetById(int32 EntryId) const
{
    return Entries.FindByPredicate([&](const FInventoryEntry& E) { return E.Id == EntryId; });
}
FInventoryEntry* FInventoryGrid::GetById(int32 EntryId)
{
    return Entries.FindByPredicate([&](FInventoryEntry& E) { return E.Id == EntryId; });
}

int32 FInventoryGrid::AddItemAt(UATGItemData* Def, int32 Qty, int32 X, int32 Y, int32 W, int32 H, bool bRotated)
{
    if (!Def || Qty <= 0) return -1;
    if (!CanPlaceRect(X, Y, W, H)) return -1;

    FInventoryEntry NewE;
    NewE.Item = TSoftObjectPtr<UATGItemData>(Def);
    NewE.Quantity = Qty;
    NewE.X = X; NewE.Y = Y;
    NewE.Width = W; NewE.Height = H;
    NewE.bRotated = bRotated;
    NewE.Id = Entries.Num() ? (Entries.Last().Id + 1) : 1;

    Entries.Add(NewE);
    MarkItemDirty(Entries.Last());
    if (OwnerComp) OwnerComp->OnItemAdded.Broadcast(NewE.Id);
    return NewE.Id;
}

bool FInventoryGrid::MoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
    FInventoryEntry* Me = GetById(EntryId);
    if (!Me) return false;

    // 1) �̹� �̵�/���ҿ��� ����� "�˻��" ġ��
    const int32 NewW = bIsRotate ? Me->Height : Me->Width;
    const int32 NewH = bIsRotate ? Me->Width : Me->Height;

    // 2) �� �ڸ��� �̵� (�˻�� NewW/NewH��)
    if (CanPlaceRect(NewX, NewY, NewW, NewH, Me->Id))
    {
        Me->X = NewX;
        Me->Y = NewY;

        // ���� Ȯ���̹Ƿ� ���� ȸ�� �ݿ�
        if (bIsRotate)
        {
            Swap(Me->Width, Me->Height); // �Ǵ� Orientation ���
        }

        MarkItemDirty(*Me);
        if (OwnerComp) OwnerComp->OnItemChanged.Broadcast(EntryId);
        return true;
    }

    // 3) ���� �ĺ� ã�� (��ħ ������ NewW/NewH��)
    FInventoryEntry* Other = nullptr;
    for (auto& E : Entries)
    {
        if (E.Id == Me->Id) continue;

        const int32 NX2 = NewX + NewW - 1;
        const int32 NY2 = NewY + NewH - 1;
        const int32 EX2 = E.X + E.Width - 1;
        const int32 EY2 = E.Y + E.Height - 1;

        const bool bHit = !(NX2 < E.X || EX2 < NewX || NY2 < E.Y || EY2 < NewY);
        if (bHit) { Other = &E; break; }
    }
    if (!Other) return false;

    // 4) ���� ���ɼ� �˻�
    const int32 MeOldX = Me->X;
    const int32 MeOldY = Me->Y;
    const int32 OtOldX = Other->X;
    const int32 OtOldY = Other->Y;

    // Other�� �� ���� �ڸ��� �� �� �ִ°� (Other�� ȸ�� ����)
    const bool bOtherFitInMy = CanPlaceRect(MeOldX, MeOldY, Other->Width, Other->Height, Me->Id);

    // ���� �� �ڸ�(NewX,NewY)�� �� �� �ִ°� (���� NewW/NewH�� �˻�)
    const bool bMeFitInNew = CanPlaceRect(NewX, NewY, NewW, NewH, Other->Id);

    if (bMeFitInNew && bOtherFitInMy)
    {
        // ���� Ȯ��
        Other->X = MeOldX;
        Other->Y = MeOldY;

        Me->X = NewX;
        Me->Y = NewY;

        // ���� Ȯ���̹Ƿ� ���� ȸ�� �ݿ�
        if (bIsRotate)
        {
            Swap(Me->Width, Me->Height); // �Ǵ� Orientation ���
        }

        MarkItemDirty(*Other);
        MarkItemDirty(*Me);
        if (OwnerComp) OwnerComp->OnItemChanged.Broadcast(EntryId);
        return true;
    }

    return false;
}

bool FInventoryGrid::Rotate(int32 EntryId)
{
    FInventoryEntry* Me = GetById(EntryId);
    if (!Me) return false;

    const int32 NewW = Me->Height;
    const int32 NewH = Me->Width;

    // ���� ��ġ���� ȸ���� ��������
    if (CanPlaceRect(Me->X, Me->Y, NewW, NewH, Me->Id))
    {
        Me->Width = NewW;
        Me->Height = NewH;
        Me->bRotated = !Me->bRotated;
        MarkItemDirty(*Me);
        if (OwnerComp) OwnerComp->OnItemChanged.Broadcast(EntryId);
        return true;
    }

    // �ȵǸ� ù ���� ��ġ Ž��
    int32 Fx, Fy;
    if (FindFirstFit(NewW, NewH, Fx, Fy, Me->Id))
    {
        Me->X = Fx; Me->Y = Fy;
        Me->Width = NewW; Me->Height = NewH;
        Me->bRotated = !Me->bRotated;
        MarkItemDirty(*Me);
        if (OwnerComp) OwnerComp->OnItemChanged.Broadcast(EntryId);
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
    if (OwnerComp) OwnerComp->OnItemRemoved.Broadcast(EntryId);
    return true;
}
