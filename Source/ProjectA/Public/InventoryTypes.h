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
    * �� �Լ��� �迭�� ����� �κи��� �����Ͽ� ��Ʈ��ũ ���ϸ� ���̴� ������ �Ѵ�. 
    FFastArraySerializer::FastArrayDeltaSerialize �Լ��� ȣ��Ǿ�, ���� �������� ��ȭ���� �����մϴ�.
    */
    //Custom Network Serialize
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryEntry, FInventoryGrid>(Entries, DeltaParms, *this);
    }

    ////��Ʈ��ũ TSoftObjectPtr rapping
    //void AddOrStack(UATGItemData* Def, int32 Qty);
    //bool RemoveItem(UATGItemData* Def, int32 Qty);

    //// ���� �Լ���
    //void AddOrStack(const TSoftObjectPtr<UATGItemData>& ItemDef, int32 Quantity);
    //bool RemoveItem(const TSoftObjectPtr<UATGItemData>& ItemDef, int32 Quantity);

    // === ���� ===
    bool CanPlaceRect(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId = -1) const;
    bool FindFirstFit(int32 W, int32 H, int32& OutX, int32& OutY, int32 IgnoreId = -1) const;

    // === ���� API (OwnerComp���� ȣ���) ===
    int32 AddItemAt(UATGItemData* Def, int32 Qty, int32 X, int32 Y, int32 W, int32 H, bool bRotated);
    bool MoveOrSwap(int32 EntryId, int32 NewX, int32 NewY, bool bIsRotate); // ��� �ڸ��� �̵�, �� ������ ����
    bool Rotate(int32 EntryId); // ���μ��� ��ȯ �� ��ġ �����ϸ� ȸ��
    bool RemoveById(int32 EntryId);
    const FInventoryEntry* GetById(int32 EntryId) const;
    FInventoryEntry* GetById(int32 EntryId);

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

