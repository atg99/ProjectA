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

int32 FInventoryGrid::AddItemAt(TSoftObjectPtr<UATGItemData> ItemDef, int32 Qty, int32 X, int32 Y, int32 W, int32 H, bool bRotated, int32 PreKey)
{
    if (!ItemDef || Qty <= 0) return 0;
    if (!CanPlaceRect(X, Y, W, H)) return 0;

    FInventoryEntry NewE;
    NewE.Item = ItemDef;
    NewE.Quantity = Qty;
    NewE.X = X; NewE.Y = Y;
    NewE.Width = W; NewE.Height = H;
    NewE.bRotated = bRotated;
   
    if (OwnerComp && OwnerComp->IsHasAuthority())
    {
        NewE.Id = Entries.Num() ? (Entries.Last().Id + 1) : 1;
        NewE.PredictionKey = PreKey;
        Entries.Add(NewE);
        MarkItemDirty(Entries.Last());

        OwnerComp->OnItemAdded.Broadcast(NewE.Id);
        return NewE.Id;
    }
    else if (OwnerComp && !OwnerComp->IsHasAuthority())
    {
        // 클라: 프리뷰는 Id 자체를 PredKey로 쓴다 (고유키)
        NewE.Id = PreKey;

        PreviewEntries.Add(NewE); // 복제 안하는 로컬 배열에 추가

        OwnerComp->OnItemPreAdded.Broadcast(NewE);
        return NewE.Id;
    }
    
    return 0;
}

bool FInventoryGrid::MoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
    FInventoryEntry* Me = GetById(EntryId);
    if (!Me) return false;

    // 1) 이번 이동/스왑에서 사용할 "검사용" 치수
    const int32 NewW = bIsRotate ? Me->Height : Me->Width;
    const int32 NewH = bIsRotate ? Me->Width : Me->Height;

    // 2) 빈 자리면 이동 (검사는 NewW/NewH로)
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
        if (OwnerComp) OwnerComp->OnItemChanged.Broadcast(EntryId);
        return true;
    }

    // 3) 스왑 후보 찾기 (겹침 판정도 NewW/NewH로)
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

    // 4) 스왑 가능성 검사
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
    if (OwnerComp) OwnerComp->OnItemRemoved.Broadcast(EntryId);
    return true;
}

bool FInventoryGrid::PreviewRemoveById(int32 PreviewId)
{
    const int32 Idx = PreviewEntries.IndexOfByPredicate(
        [&](const FInventoryEntry& P) { return P.Id == PreviewId; });
    if (Idx == INDEX_NONE) return false;

    const FInventoryEntry Removed = PreviewEntries[Idx];
    PreviewEntries.RemoveAt(Idx);

    return true;
}

bool FInventoryGrid::PreviewMoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate)
{
    const FInventoryEntry* Me = GetById(EntryId);
    if (!Me || !OwnerComp) return false;
    if (!OwnerComp->IsLocallyOwned()) return false;

    const int32 NewW = bIsRotate ? Me->Height : Me->Width;
    const int32 NewH = bIsRotate ? Me->Width : Me->Height;

    auto RectOverlaps = [](int32 AX, int32 AY, int32 AW, int32 AH,
        int32 BX, int32 BY, int32 BW, int32 BH) -> bool
        {
            // [AX, AX+AW) x [AY, AY+AH) 규칙(반열림)으로 겹침 체크
            const bool NoOverlap =
                (AX + AW) <= BX || (BX + BW) <= AX ||
                (AY + AH) <= BY || (BY + BH) <= AY;
            return !NoOverlap;
        };

    // 프리뷰/실제 모두 포함한 충돌 체크에서 특정 두 Id를 제외
    auto CanPlaceExcept = [&](int32 PX, int32 PY, int32 PW, int32 PH, int32 IgnoreA, int32 IgnoreB) -> bool
        {
            // 실제 엔트리와의 충돌
            for (const FInventoryEntry& E : Entries)
            {
                if (E.Id == IgnoreA || E.Id == IgnoreB) continue;
                if (RectOverlaps(PX, PY, PW, PH, E.X, E.Y, E.Width, E.Height))
                    return false;
            }
            // 현재 떠 있는 프리뷰와의 충돌도 방지 (내/상대 프리뷰는 곧 갱신 예정이므로 제외)
            for (const FInventoryEntry& P : PreviewEntries)
            {
                if (P.Id == IgnoreA || P.Id == IgnoreB) continue;
                if (RectOverlaps(PX, PY, PW, PH, P.X, P.Y, P.Width, P.Height))
                    return false;
            }
            return true;
        };

    // 1) 단순 이동 프리뷰 시도
    if (CanPlaceExcept(NewX, NewY, NewW, NewH, Me->Id, -1))
    {
        // 이전 프리뷰 정리 후 내 프리뷰만 생성
        PreviewRemoveById(Me->Id);

        FInventoryEntry Pre = *Me;
        Pre.X = NewX;  Pre.Y = NewY;
        Pre.Width = NewW;  Pre.Height = NewH;
        Pre.bRotated = bIsRotate;

        PreviewEntries.Add(Pre);
        OwnerComp->OnItemPreAdded.Broadcast(Pre);
        return true;
    }

    // 2) 스왑 후보 찾기(새 자리와 겹치는 상대)
    const FInventoryEntry* Other = nullptr;
    for (const FInventoryEntry& E : Entries)
    {
        if (E.Id == Me->Id) continue;
        if (RectOverlaps(NewX, NewY, NewW, NewH, E.X, E.Y, E.Width, E.Height))
        {
            Other = &E;
            break;
        }
    }
    if (!Other) return false;

    const int32 MeOldX = Me->X, MeOldY = Me->Y;
    const int32 OtOldX = Other->X, OtOldY = Other->Y;

    // 3) 스왑 가능성 검사
    //    - 나는 새 자리(NewX/NewY/NewW/NewH)로, Other는 내 기존 자리(MeOldX/MeOldY)로
    //    - 두 검사 모두에서 서로(Me/Other) **둘 다 제외**해야 함
    const bool bMeFitInNew = CanPlaceExcept(NewX, NewY, NewW, NewH, Me->Id, Other->Id);
    const bool bOtherFitInMy = CanPlaceExcept(MeOldX, MeOldY, Other->Width, Other->Height, Me->Id, Other->Id);

    if (!bMeFitInNew || !bOtherFitInMy)
        return false;

    // 4) 스왑 프리뷰 생성 (기존 프리뷰 정리 → 두 엔트리 모두 프리뷰 추가)
    PreviewRemoveById(Me->Id);
    PreviewRemoveById(Other->Id);

    // 내 프리뷰(회전 반영)
    {
        FInventoryEntry PreA = *Me;
        PreA.X = NewX;  PreA.Y = NewY;
        PreA.Width = NewW;  PreA.Height = NewH;
        PreA.bRotated = bIsRotate;
        PreviewEntries.Add(PreA);
        OwnerComp->OnItemPreAdded.Broadcast(PreA);
    }

    // 상대 프리뷰(회전 유지: 필요 시 정책에 맞게 수정)
    {
        FInventoryEntry PreB = *Other;
        PreB.X = MeOldX;  PreB.Y = MeOldY;
        // PreB.Width/Height/bRotated는 그대로 유지
        PreviewEntries.Add(PreB);
        OwnerComp->OnItemPreAdded.Broadcast(PreB);
    }

    return true;
}