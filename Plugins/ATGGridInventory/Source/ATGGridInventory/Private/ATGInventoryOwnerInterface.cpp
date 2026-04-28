// Fill out your copyright notice in the Description page of Project Settings.

#include "ATGInventoryOwnerInterface.h"
#include "InventoryTypes.h"

const FInventoryGrid& IATGInventoryOwnerInterface::GetInventory()
{
    static const FInventoryGrid EmptyGrid;
    return EmptyGrid;
}

const TArray<struct FInventoryEntry>& IATGInventoryOwnerInterface::GetEntries()
{
    static const TArray<FInventoryEntry> EmptyEntries;
    return EmptyEntries;
}
