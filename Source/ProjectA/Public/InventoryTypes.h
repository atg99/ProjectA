// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryTypes.generated.h"

class UATGItemData;
class UATGInventoryComponent;

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

    UPROPERTY(NotReplicated)
    UATGInventoryComponent* OwnerComp = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 GridWidth = 10;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 GridHeight = 10;

    /*
    * 이 함수는 배열의 변경된 부분만을 복제하여 네트워크 부하를 줄이는 역할을 한다. 
    FFastArraySerializer::FastArrayDeltaSerialize 함수가 호출되어, 단일 데이터의 변화만을 전송합니다.
    */
    //Custom Network Serialize
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryEntry, FInventoryGrid>(Entries, DeltaParms, *this);
    }

    ////네트워크 TSoftObjectPtr rapping
    //void AddOrStack(UATGItemData* Def, int32 Qty);
    //bool RemoveItem(UATGItemData* Def, int32 Qty);

    //// 헬퍼 함수들
    //void AddOrStack(const TSoftObjectPtr<UATGItemData>& ItemDef, int32 Quantity);
    //bool RemoveItem(const TSoftObjectPtr<UATGItemData>& ItemDef, int32 Quantity);

    // === 헬퍼 ===
    bool CanPlaceRect(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId = -1) const;
    bool FindFirstFit(int32 W, int32 H, int32& OutX, int32& OutY, int32 IgnoreId = -1) const;

    // === 조작 API (OwnerComp에서 호출됨) ===
    int32 AddItemAt(UATGItemData* Def, int32 Qty, int32 X, int32 Y, int32 W, int32 H, bool bRotated);
    bool MoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate); // 비는 자리면 이동, 차 있으면 스왑
    bool Rotate(int32 EntryId); // 가로세로 교환 후 배치 가능하면 회전
    bool RemoveById(int32 EntryId);
    const FInventoryEntry* GetById(int32 EntryId) const;
    FInventoryEntry* GetById(int32 EntryId);

};


/*
* 이 템플릿 특성은 FFastArrayPlayerList 구조체가 네트워크 델타 직렬화(NetDeltaSerialize)를 지원함을 언리얼 엔진에 알려준다.
WithNetDeltaSerializer 플래그가 true로 설정되어 있어, 네트워크 복제 시 NetDeltaSerialize 함수가 호출된다.
*/
template<>
struct TStructOpsTypeTraits<FInventoryGrid> : public TStructOpsTypeTraitsBase2<FInventoryGrid>
{
    enum { WithNetDeltaSerializer = true };
};

