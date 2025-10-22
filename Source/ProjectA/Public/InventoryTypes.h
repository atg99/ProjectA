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

    void PreReplicatedRemove(const struct FInventoryList& InArraySerializer);
    void PostReplicatedAdd(const struct FInventoryList& InArraySerializer);
    void PostReplicatedChange(const struct FInventoryList& InArraySerializer);
};

USTRUCT(BlueprintType)
struct FInventoryList : public FFastArraySerializer
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FInventoryEntry> Entries;

    UPROPERTY(NotReplicated)
    UATGInventoryComponent* OwnerComp = nullptr;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryEntry, FInventoryList>(Entries, DeltaParms, *this);
    }

    //네트워크 TSoftObjectPtr rapping
    void AddOrStack(UATGItemData* Def, int32 Qty);
    bool RemoveItem(UATGItemData* Def, int32 Qty);

    // 헬퍼 함수들
    void AddOrStack(const TSoftObjectPtr<UATGItemData>& ItemDef, int32 Quantity);
    bool RemoveItem(const TSoftObjectPtr<UATGItemData>& ItemDef, int32 Quantity);
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
    enum { WithNetDeltaSerializer = true };
};

