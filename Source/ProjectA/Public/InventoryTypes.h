// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryTypes.generated.h"

class UATGItemData;
class UATGInventoryComponent;

// InventoryTypes.h
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

    // 클라가 지정(그리드 드롭)했을 수도 있고, 자동 배치면 -1 유지
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
    Rejected        UMETA(DisplayName = "Rejected"),     // 일반 거절(사유코드로 구체화)
    PartialSuccess  UMETA(DisplayName = "Partial Success"), // 일부 수량만 확정
    Error           UMETA(DisplayName = "Error"),
};

UENUM(BlueprintType)
enum class EInventoryRejectReason : uint8
{
    None,
    AlreadyTaken,
    NoSpace,            // 빈칸 없음
    StackFull,          // 스택 한도 초과
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

    /** 처리 결과 */
    UPROPERTY(BlueprintReadOnly)
    EInventoryChangeStatus Status = EInventoryChangeStatus::Error;

    /** 실패 사유 (Status==Rejected일 때 의미 있음) */
    UPROPERTY(BlueprintReadOnly)
    EInventoryRejectReason Reason = EInventoryRejectReason::None;

    /** 클라 예측 식별 키 (프리뷰 매칭용, 반드시 Echo) */
    UPROPERTY(BlueprintReadOnly)
    int32 PredictionKey = 0;

    /** 인벤토리 스냅샷 버전(순증가). 최신만 적용 */
    UPROPERTY(BlueprintReadOnly)
    int32 InventoryRevision = 0;

    /** 아이템 정의 ID (경량) */
    UPROPERTY(BlueprintReadOnly)
    FPrimaryAssetId ItemDefId; // or FName/RowHandle

    /** 최종 확정된 엔트리 Id (신규 생성 시) */
    UPROPERTY(BlueprintReadOnly)
    int32 NewEntryId = -1;

    /** 기존 스택과 병합됐다면 병합 대상 엔트리 Id */
    UPROPERTY(BlueprintReadOnly)
    int32 MergedIntoEntryId = -1;

    /** 최종 좌표/크기/회전 (성공 또는 PartialSuccess일 때) */
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

    /** 수량 정보 */
    UPROPERTY(BlueprintReadOnly)
    int32 RequestedQuantity = 0;
    UPROPERTY(BlueprintReadOnly)
    int32 GrantedQuantity = 0;    // 확정/부분확정 수량
    UPROPERTY(BlueprintReadOnly)
    int32 PreviousQuantity = 0;   // 병합 전 기존 스택 수량(있으면)

    /** 서버가 자동배치 했는지(클라 프리뷰와 다를 수 있음) */
    UPROPERTY(BlueprintReadOnly)
    bool bServerAutoPlaced = false;

    /** 실패 시 대안(선택) */
    UPROPERTY(BlueprintReadOnly)
    int32 SuggestedX = -1;
    UPROPERTY(BlueprintReadOnly)
    int32 SuggestedY = -1;
};


/**
 * 
 */

USTRUCT(BlueprintType)
struct FInventoryEntry : public FFastArraySerializerItem
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UATGItemData> Item;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Quantity = 0;

    // grid placement
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 X = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Y = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Width = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Height = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRotated = true;

    UPROPERTY()
    int32 Id = -1;

    UPROPERTY() 
    int32 PredictionKey = 0;

    void PreReplicatedRemove(const struct FInventoryGrid& InArraySerializer);
    void PostReplicatedAdd(const struct FInventoryGrid& InArraySerializer);
    void PostReplicatedChange(const struct FInventoryGrid& InArraySerializer);
};

//https://corma642.tistory.com/565
USTRUCT(BlueprintType)
struct FInventoryGrid : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FInventoryEntry> Entries;

    //preview 전용 복제안하는 배열
    UPROPERTY(NotReplicated)
    TArray<FInventoryEntry> PreviewEntries;

    UPROPERTY(NotReplicated)
    UATGInventoryComponent* OwnerComp = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 GridWidth = 10;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 GridHeight = 10;

    //Custom Network Serialize
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryEntry, FInventoryGrid>(Entries, DeltaParms, *this);
    }

    bool CanPlaceRect(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId = -1) const;

    bool FindFirstFit(int32 W, int32 H, int32& OutX, int32& OutY, int32 IgnoreId = -1) const;

    int32 AddItemAt(TSoftObjectPtr<UATGItemData> ItemDef, int32 Qty, int32 X, int32 Y, int32 W, int32 H, bool bRotated, int32 PreKey);

    bool MoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate); // ��� �ڸ��� �̵�, �� ������ ����

    bool Rotate(int32 EntryId); 

    bool RemoveById(int32 EntryId);

    const FInventoryEntry* GetById(int32 EntryId) const;

    FInventoryEntry* GetById(int32 EntryId);

    bool PreviewRemoveById(int32 EntryId);

    bool PreviewMoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate);
};


/*
* �� ���ø� Ư���� FFastArrayPlayerList ����ü�� ��Ʈ��ũ ��Ÿ ����ȭ(NetDeltaSerialize)�� �������� �𸮾� ������ �˷��ش�.
WithNetDeltaSerializer �÷��װ� true�� �����Ǿ� �־�, ��Ʈ��ũ ���� �� NetDeltaSerialize �Լ��� ȣ��ȴ�.
*/
template<>
struct TStructOpsTypeTraits<FInventoryGrid> : public TStructOpsTypeTraitsBase2<FInventoryGrid>
{
    enum { WithNetDeltaSerializer = true };
};

