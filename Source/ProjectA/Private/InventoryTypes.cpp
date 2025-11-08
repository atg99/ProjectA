// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryTypes.h"
#include "ATGInventoryComponent.h"
#include "ATGItemData.h"

int32 FInventoryGrid::GlobalEntryIdCounter = 0;

void FInventoryEntry::PreReplicatedRemove(const FInventoryGrid& InArraySerializer)
{
    if (InArraySerializer.OwnerComp)
        InArraySerializer.OwnerComp->OnItemRemoved.Broadcast(Id);
}

void FInventoryEntry::PostReplicatedAdd(const FInventoryGrid& InArraySerializer)
{
    if (InArraySerializer.OwnerComp)
    {
        InArraySerializer.OwnerComp->OnItemAdded.Broadcast(Id);
    }
    else
    {
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("!!! FInventoryEntry::PostReplicatedAdd"));
    }
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
//        //배열 전체 구조 변경알림
//        Entries.RemoveAt(Index);
//        MarkArrayDirty();
//    }
//    else
//    {
//        //배열 요소 변경 알림
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

        //조건중 하나라도 만족하면 겹치지 않음
        const bool bOverlap = !(NX2 < E.X || EX2 < StartX || NY2 < E.Y || EY2 < StartY);

        if (bOverlap) return false;
    }

    if (OwnerComp->IsLocallyOwned()) //Local 판정일 때 프리뷰아이템도 고려
    {
        for (const auto& E : PreviewEntries)
        {
            if (E.Id == IgnoreId) continue;

            const int32 EX2 = E.X + E.Width - 1;
            const int32 EY2 = E.Y + E.Height - 1;
            const int32 NX2 = StartX + W - 1;
            const int32 NY2 = StartY + H - 1;

            //조건중 하나라도 만족하면 겹치지 않음
            const bool bOverlap = !(NX2 < E.X || EX2 < StartX || NY2 < E.Y || EY2 < StartY);

            if (bOverlap) return false;
        }
    }

    return true;
}

bool FInventoryGrid::FindFirstFit(int32 W, int32 H, int32& OutX, int32& OutY, int32 IgnoreId)
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

bool FInventoryGrid::FindFirstFit(TSoftObjectPtr<UATGItemData> ItemDef, int32 W, int32 H, int32& OutX, int32& OutY, int32& Qty, int32 IgnoreId)
{
    //int32 TempQty = Qty;
    Qty = FindAddFitStack(ItemDef, Qty, IgnoreId); //채울수 있는 스택 검색 후 채움

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

int32 FInventoryGrid::FindAddFitStack(TSoftObjectPtr<UATGItemData> ItemDef, int32 Qty, int32 IgnoreId)
{
    if (!ItemDef.Get())
    {
        ItemDef.LoadSynchronous();
    }
    int32 RemainQty = Qty;
    for (auto& E : Entries)
    {
        if (E.Id == IgnoreId) continue; //자기 자신 무시

        if (RemainQty == 0) //스텍 찾으면서 순회하다 남은 수량 0되면 종료
        {
            return RemainQty;
        }

        int32 RemainCapacity = E.Item->MaxStack - E.Quantity;
        if (E.Item->ItemId == ItemDef->ItemId && RemainCapacity >= 1) // 아이템 아이디 같고 스텍 남은 자리가 1이상일때
        {
            int32 AddedQty = FMath::Min(RemainCapacity, RemainQty);
            RemainQty -= AddedQty; //스택에 넣고 남은 아이템 수량
            E.Quantity += AddedQty;
            if (OwnerComp)
            {   
                OwnerComp->OnItemChanged.Broadcast(E.Id);
                if (OwnerComp->IsHasAuthority())    //서버에만 배열 마크
                {
                    MarkItemDirty(E);
                    OwnerComp->GetOwner()->ForceNetUpdate();
                }
                else
                {
                    //프리뷰 위젯 변경 브로드케스트
                    OwnerComp->OnItemPreChanged.Broadcast(E);
                }
            }
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

int32 FInventoryGrid::AddItemAt(TSoftObjectPtr<UATGItemData> ItemDef, int32& Qty, int32 X, int32 Y, int32 W, int32 H, bool bRotated, int32 PreKey)
{
    
    if (!ItemDef || Qty <= 0) return 0;
    if (!CanPlaceRect(X, Y, bRotated ? H : W, bRotated ? W : H)) return 0;

    int32 RemainQty = Qty;
    
    FInventoryEntry NewE;
    NewE.Item = ItemDef;

    int32 QtyStack = FMath::Min(ItemDef->MaxStack, Qty); // 최대 스택, 남은 수량 중 낮은 값
    RemainQty = Qty - QtyStack; //남은 수량 다들어갔으면 남은 값은 0

    NewE.Quantity = QtyStack;
    NewE.X = X; NewE.Y = Y;

    NewE.Width = bRotated ? H : W; 
    NewE.Height = bRotated ? W : H;

    NewE.bRotated = bRotated;

    Qty = RemainQty;

    if (OwnerComp && OwnerComp->IsHasAuthority())
    {
        NewE.Id = ++GlobalEntryIdCounter;
        NewE.PredictionKey = PreKey;
        Entries.Add(NewE);
        MarkItemDirty(Entries.Last());

        OwnerComp->GetOwner()->ForceNetUpdate();

        OwnerComp->OnItemAdded.Broadcast(NewE.Id);
        return NewE.Id;
    }
    else if (OwnerComp && !OwnerComp->IsHasAuthority())
    {
        // 클라: 프리뷰는 Id 자체를 PredKey로 쓴다 (고유키)
        NewE.Id = PreKey;

        //PreviewEntries.Add(NewE); // 복제 안하는 로컬 배열에 추가

        OwnerComp->OnItemPreAdded.Broadcast(NewE);

        return NewE.Id;
    }

    return 0;
}

bool FInventoryGrid::MoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
    FInventoryEntry* Me = GetById(EntryId);
    if (!Me) return false;

    // 이번 이동/스왑에서 사용할 "검사용" 치수
    const int32 NewW = bIsRotate ? Me->Height : Me->Width;
    const int32 NewH = bIsRotate ? Me->Width : Me->Height;

    // 빈 자리면 이동
    if (CanPlaceRect(NewX, NewY, NewW, NewH, Me->Id))
    {
        Me->X = NewX;
        Me->Y = NewY;

        // 성공 확정이므로 실제 회전 반영
        if (bIsRotate)
        {
            Swap(Me->Width, Me->Height); // 또는 Orientation 토글
        }

        MarkItemDirty(*Me);
        if (OwnerComp)
        {
            OwnerComp->OnItemChanged.Broadcast(EntryId);
            if (OwnerComp->IsHasAuthority())
            {
                OwnerComp->GetOwner()->ForceNetUpdate();
            }
        }
        return true;
    }

    // 스왑 후보 찾기
    FInventoryEntry* Other = nullptr;
    for (auto& E : Entries)
    {
        if (E.Id == Me->Id) continue;

        const int32 NX2 = NewX + NewW - 1;
        const int32 NY2 = NewY + NewH - 1;
        const int32 EX2 = E.X + E.Width - 1;
        const int32 EY2 = E.Y + E.Height - 1;

        const bool bHit = !(NX2 < E.X || EX2 < NewX || NY2 < E.Y || EY2 < NewY);
        if (bHit) 
        { 
            Other = &E;
            break; 
        }
    }
    if (!Other) return false;
     
    // 아이템 종류가 같을 때
    if (Other->Item->ItemId == Me->Item->ItemId)
    {
        const int32 Remaining = Other->Item->MaxStack - Other->Quantity;
        if (Remaining <= 0)
        {
            return false;
        }
        const int32 StackNum = FMath::Min(Remaining, Me->Quantity);

        IncreaseQtyByRef(*Other, StackNum);
        DecreaseQtyByRef(*Me, StackNum);
        
        return true;
    }

    // 스왑 가능성 검사
    const int32 MeOldX = Me->X;
    const int32 MeOldY = Me->Y;
    const int32 OtOldX = Other->X;
    const int32 OtOldY = Other->Y;

    // Other가 내 기존 자리로 들어갈 수 있는가 (Other는 회전 없음)
    const bool bOtherFitInMy = CanPlaceRect(MeOldX, MeOldY, Other->Width, Other->Height, Me->Id);

    // 나는 새 자리(NewX,NewY)에 들어갈 수 있는가 (나는 NewW/NewH로 검사)
    const bool bMeFitInNew = CanPlaceRect(NewX, NewY, NewW, NewH, Other->Id);

    if (bMeFitInNew && bOtherFitInMy)
    {
        // 스왑 확정
        Other->X = MeOldX;
        Other->Y = MeOldY;

        Me->X = NewX;
        Me->Y = NewY;

        // 성공 확정이므로 실제 회전 반영
        if (bIsRotate)
        {
            Swap(Me->Width, Me->Height); // 또는 Orientation 토글
        }

        MarkItemDirty(*Other);
        MarkItemDirty(*Me);
        if (OwnerComp)
        {
            OwnerComp->OnItemChanged.Broadcast(EntryId);
            if (OwnerComp->IsHasAuthority())
            {
                OwnerComp->GetOwner()->ForceNetUpdate();
            }
        }
        return true;
    }

    return false;
}

bool FInventoryGrid::MergeStackAtAndDecrease(FInventoryEntry& Entry, int32 Qty, int32 NewX, int32 NewY, bool bIsRotate)
{

    const int32 NewW = bIsRotate ? Entry.Height : Entry.Width;
    const int32 NewH = bIsRotate ? Entry.Width : Entry.Height;

    FInventoryEntry* Other = nullptr;
    for (auto& E : Entries)
    {
        if (E.Id == Entry.Id) continue;

        const int32 NX2 = NewX + NewW - 1;
        const int32 NY2 = NewY + NewH - 1;
        const int32 EX2 = E.X + E.Width - 1;
        const int32 EY2 = E.Y + E.Height - 1;

        const bool bHit = !(NX2 < E.X || EX2 < NewX || NY2 < E.Y || EY2 < NewY);
        if (bHit)
        {
            Other = &E;
            break;
        }
    }
    if (!Other) return false;

    // 아이템 종류가 같을 때
    if (Other->Item->ItemId == Entry.Item->ItemId)
    {
        const int32 Remain = Other->Item->MaxStack - Other->Quantity;
        if (Remain <= 0)
        {
            return false;
        }
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

    const int32 NewW = Me->Height;
    const int32 NewH = Me->Width;

    // 같은 위치에서 회전이 가능한지
    if (CanPlaceRect(Me->X, Me->Y, NewW, NewH, Me->Id))
    {
        Me->Width = NewW;
        Me->Height = NewH;
        Me->bRotated = !Me->bRotated;
        MarkItemDirty(*Me);
        if (OwnerComp) OwnerComp->OnItemChanged.Broadcast(EntryId);
        return true;
    }

    // 안되면 첫 적합 위치 탐색
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
    if (OwnerComp)
    {
        OwnerComp->OnItemRemoved.Broadcast(EntryId);
        OwnerComp->GetOwner()->ForceNetUpdate();
    }
    return true;
}

bool FInventoryGrid::DecreaseQtyById(int32 EntryId, int32 Num)
{
    FInventoryEntry* E = GetById(EntryId);
    if (E->Quantity - Num > 0)
    {
        E->Quantity -= Num;
        if (OwnerComp)
        {
            OwnerComp->OnItemChanged.Broadcast(E->Id);
            if (OwnerComp->IsHasAuthority())    //서버에만 배열 마크
            {
                MarkItemDirty(*E);
                OwnerComp->GetOwner()->ForceNetUpdate();
            }
        }
        return true;
    }
    //0이하 일 때 삭제
    else
    {
        RemoveById(EntryId);
    }
    return false;
}

bool FInventoryGrid::DecreaseQtyByRef(FInventoryEntry& E, int32 Num)
{
    if (E.Quantity - Num > 0)
    {
        E.Quantity -= Num;
        if (OwnerComp)
        {
            OwnerComp->OnItemChanged.Broadcast(E.Id);
            if (OwnerComp->IsHasAuthority())    //서버에만 배열 마크
            {
                MarkItemDirty(E);
                OwnerComp->GetOwner()->ForceNetUpdate();
            }
        }
        return true;
    }
    //0이하 일 때 삭제
    else
    {
        RemoveById(E.Id);
    }
    return false;
}

bool FInventoryGrid::IncreaseQtyById(int32 EntryId, int32 Num)
{
    if (Num <= 0)
    {
        return false;
    }
    FInventoryEntry* E = GetById(EntryId);
    if (E->Quantity + Num <= E->Item->MaxStack)
    {
        E->Quantity += Num;
        if (OwnerComp)
        {
            OwnerComp->OnItemChanged.Broadcast(E->Id);
            if (OwnerComp->IsHasAuthority())    //서버에만 배열 마크
            {
                MarkItemDirty(*E);
                OwnerComp->GetOwner()->ForceNetUpdate();
            }
        }
        return true;
    }
    return false;
}

bool FInventoryGrid::IncreaseQtyByRef(FInventoryEntry& E, int32 Num)
{
    if (Num <= 0)
    {
        return false;
    }

    if (E.Quantity + Num <= E.Item->MaxStack)
    {
        E.Quantity += Num;
        if (OwnerComp)
        {
            OwnerComp->OnItemChanged.Broadcast(E.Id);
            if (OwnerComp->IsHasAuthority())    //서버에만 배열 마크
            {
                MarkItemDirty(E);
                OwnerComp->GetOwner()->ForceNetUpdate();
            }
        }
        return true;
    }
    return false;
}


bool FInventoryGrid::PreviewRemoveById(int32 PreviewId)
{
  /*  int32 Num = PreviewEntries.RemoveAll(
        [&](const FInventoryEntry& P) 
        {
            return P.Id == PreviewId; 
        });
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(-1, 30.0f, FColor::Cyan, TEXT("PreviewRemoveById"));*/
    return 0;
}

bool FInventoryGrid::PreviewMoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
    const FInventoryEntry* Me = GetById(EntryId);
    if (!Me || !OwnerComp) return false;
    if (!OwnerComp->IsLocallyOwned()) return false;

    const int32 NewW = bIsRotate ? Me->Height : Me->Width;
    const int32 NewH = bIsRotate ? Me->Width : Me->Height;

    // 1) 이동 프리뷰: 단일 Ignore로 기존 함수 그대로
    if (CanPlaceRect(NewX, NewY, NewW, NewH, Me->Id))
    {
        PreviewRemoveById(Me->Id);

        FInventoryEntry Pre = *Me;
        Pre.X = NewX;  Pre.Y = NewY;
        Pre.Width = NewW;  Pre.Height = NewH;
        Pre.bRotated = bIsRotate;

        PreviewEntries.Add(Pre);
        OwnerComp->OnItemPreAdded.Broadcast(Pre);
        return true;
    }

    // 2) 스왑 후보 찾기 (겹치는 실제 엔트리 하나 찾기)
    const FInventoryEntry* Other = nullptr;
    {
        const int32 NX2 = NewX + NewW - 1;
        const int32 NY2 = NewY + NewH - 1;
        for (const auto& E : Entries)
        {
            if (E.Id == Me->Id) continue;
            const int32 EX2 = E.X + E.Width - 1;
            const int32 EY2 = E.Y + E.Height - 1;
            const bool bOverlap = !(NX2 < E.X || EX2 < NewX || NY2 < E.Y || EY2 < NewY);
            if (bOverlap) { Other = &E; break; }
        }
    }
    if (!Other) return false;

    const int32 MeOldX = Me->X, MeOldY = Me->Y;

    // 3) 스왑 가능 확인: **두 엔트리(Me/Other) 모두 예외 처리**
    const bool bMeFitInNew = CanPlaceRect(NewX, NewY, NewW, NewH, Other->Id);
    const bool bOtherFitInMy = CanPlaceRect(MeOldX, MeOldY, Other->Width, Other->Height, Me->Id);
    if (!bMeFitInNew || !bOtherFitInMy) return false;

    // 4) 프리뷰 스왑 적용
    PreviewRemoveById(Me->Id);
    PreviewRemoveById(Other->Id);

    FInventoryEntry PreA = *Me;
    PreA.X = NewX;  PreA.Y = NewY;
    PreA.Width = NewW;  PreA.Height = NewH;
    PreA.bRotated = bIsRotate;
    PreviewEntries.Add(PreA);
    OwnerComp->OnItemPreAdded.Broadcast(PreA);

    FInventoryEntry PreB = *Other;
    PreB.X = MeOldX;  PreB.Y = MeOldY; // 회전/사이즈 유지
    PreviewEntries.Add(PreB);
    OwnerComp->OnItemPreAdded.Broadcast(PreB);

    return true;
}


