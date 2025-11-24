// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGEquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "ATGItemData.h"

// Sets default values for this component's properties
UATGEquipmentComponent::UATGEquipmentComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
	// ...
}


// Called when the game starts
void UATGEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UATGEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UATGEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UATGEquipmentComponent, MainWeapon1, COND_None);
	DOREPLIFETIME_CONDITION(UATGEquipmentComponent, MainWeapon2, COND_None);
}

void UATGEquipmentComponent::ItemRemoved(int32 EntryId)
{
	OnEquipmentRemoved.Broadcast(EntryId);
}

void UATGEquipmentComponent::ItemAdded(int32 EntryId)
{
	OnEquipmentAdded.Broadcast(EntryId);
}

void UATGEquipmentComponent::ItemChanged(int32 EntryId)
{
	OnEquipmentChanged.Broadcast(EntryId);
}

const TArray<struct FInventoryEntry>& UATGEquipmentComponent::GetEntries()
{

	Entries.Empty();
	if (MainWeapon1.Item)
	{
		Entries.Add(MainWeapon1);
	}
	if (MainWeapon2.Item)
	{
		Entries.Add(MainWeapon2);
	}
	
	return Entries;
}

