// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGPlayerState.h"
#include "ATGInventoryComponent.h"
#include "ATGEquipmentComponent.h"


AATGPlayerState::AATGPlayerState()
{
	InventoryComponent = CreateDefaultSubobject<UATGInventoryComponent>(TEXT("InventoryComp"));
	EquipmentComponent = CreateDefaultSubobject<UATGEquipmentComponent>(TEXT("EquipmentComp"));
}
