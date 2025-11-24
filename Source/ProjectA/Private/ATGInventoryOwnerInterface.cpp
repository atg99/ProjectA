// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGInventoryOwnerInterface.h"
#include "InventoryTypes.h"
// Add default functionality here for any IATGInventoryOwnerInterface functions that are not pure virtual.
const FInventoryGrid& IATGInventoryOwnerInterface::GetInventory()
{
    static const FInventoryGrid EmptyGrid;

    return EmptyGrid;
}