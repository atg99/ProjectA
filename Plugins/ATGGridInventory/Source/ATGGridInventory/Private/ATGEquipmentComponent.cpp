// Fill out your copyright notice in the Description page of Project Settings.

#include "ATGEquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "Data/ATGItemData.h"
#include "Data/ATGEquipmentData.h"
#include "Data/ATGWeaponData.h"
#include "ATGInventoryOwnerInterface.h"

UATGEquipmentComponent::UATGEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UATGEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		FirstMainWeapon.Id = (int32)EEquipmentSlotType::MainWeapon1Slot;
		FirstMainWeapon.Width = WeaponSlotSize.X;
		FirstMainWeapon.Height = WeaponSlotSize.Y;
		FirstMainWeapon.X = 0;
		FirstMainWeapon.Y = 0;

		SecondMainWeapon.Id = (int32)EEquipmentSlotType::MainWeapon2Slot;
		SecondMainWeapon.Width = WeaponSlotSize.X;
		SecondMainWeapon.Height = WeaponSlotSize.Y;
		SecondMainWeapon.X = 0;
		SecondMainWeapon.Y = 0;
	}
}

void UATGEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UATGEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UATGEquipmentComponent, FirstMainWeapon, COND_None);
	DOREPLIFETIME_CONDITION(UATGEquipmentComponent, SecondMainWeapon, COND_None);
}

void UATGEquipmentComponent::OnRep_FirstMainWeapon()
{
	OnFirstMainWeaponChanged.Broadcast(FirstMainWeapon);
}

void UATGEquipmentComponent::OnRep_SecondMainWeapon()
{
	OnSecondMainWeaponChanged.Broadcast(SecondMainWeapon);
}

const FInventoryEntry* UATGEquipmentComponent::GetEquipmentById() const
{
	return nullptr;
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

void UATGEquipmentComponent::TryHandleTransItemResult(int32 EntryId, int32 RemoveQty)
{
	if (RemoveQty < 1)
	{
		return;
	}

	switch (EntryId)
	{
	case (int32)EEquipmentSlotType::MainWeapon1Slot:
		FirstMainWeapon.Item = nullptr;
		OnRep_FirstMainWeapon();
		break;
	case (int32)EEquipmentSlotType::MainWeapon2Slot:
		SecondMainWeapon.Item = nullptr;
		OnRep_SecondMainWeapon();
		break;
	default:
		UE_LOG(LogTemp, Warning, TEXT("UATGEquipmentComponent::TryHandleTransItemResult Invalid EntryId"));
		break;
	}
	GetOwner()->ForceNetUpdate();
}

void UATGEquipmentComponent::GetEquipmentEntry(EEquipmentSlotType EquipSlotType, FInventoryEntry& OutEntry)
{
	switch (EquipSlotType)
	{
	case EEquipmentSlotType::None:
		break;
	case EEquipmentSlotType::MainWeapon1Slot:
		OutEntry = FirstMainWeapon;
		break;
	case EEquipmentSlotType::MainWeapon2Slot:
		OutEntry = SecondMainWeapon;
		break;
	default:
		break;
	}
}

void UATGEquipmentComponent::TryAddItemAt(TScriptInterface<IATGInventoryOwnerInterface> Inven, int32 OtherGridId, TSoftObjectPtr<class UATGItemData> ItemDef, int32 Qty, int32 X, int32 Y, bool bRotate)
{
	uint8 SlotType = (uint8)X;
	ServerAddEquipment(ItemDef, SlotType, OtherGridId, Inven);
}

void UATGEquipmentComponent::ServerAddEquipment_Implementation(const TSoftObjectPtr<class UATGItemData>& ItemDef, uint8 EquipmentSlotType, int32 OtherGridId, const TScriptInterface<IATGInventoryOwnerInterface>& Inven)
{
	if (ItemDef.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] ServerAddEquipment: ItemDef is Null"), *GetName());
		return;
	}

	if (!ItemDef.IsValid())
	{
		if (!ItemDef.LoadSynchronous())
		{
			UE_LOG(LogTemp, Error, TEXT("[%s] ServerAddEquipment: Failed to Load ItemDef"), *GetName());
			return;
		}
	}

	EEquipmentSlotType SlotType = (EEquipmentSlotType)EquipmentSlotType;

	if (UATGItemData* ItemData = ItemDef.Get(); !ItemData || !CheckItemFitSlot(ItemData, SlotType))
	{
		return;
	}

	FInventoryEntry* TargetEntry = nullptr;
	int32 TargetId = 0;
	if (SlotType == EEquipmentSlotType::MainWeapon1Slot)
	{
		TargetEntry = &FirstMainWeapon;
		TargetId = TargetEntry->Id;
	}
	else if (SlotType == EEquipmentSlotType::MainWeapon2Slot)
	{
		TargetEntry = &SecondMainWeapon;
		TargetId = TargetEntry->Id;
	}

	if (Inven.GetObject() == this && OtherGridId == TargetId)
	{
		UE_LOG(LogTemp, Log, TEXT("UATGEquipmentComponent: Same Slot"));
		return;
	}

	if (TargetEntry)
	{
		UE_LOG(LogTemp, Display, TEXT("UATGEquipmentComponent::ServerAddEquipment"));

		if (TargetEntry->Item == ItemDef)
		{
			return;
		}

		TargetEntry->Item = ItemDef;
		TargetEntry->Quantity = 1;
		TargetEntry->bRotated = false;

		Inven->TryHandleTransItemResult(OtherGridId, 1);

		GetOwner()->ForceNetUpdate();

		switch (SlotType)
		{
		case EEquipmentSlotType::None:
			break;
		case EEquipmentSlotType::MainWeapon1Slot:
			OnRep_FirstMainWeapon();
			break;
		case EEquipmentSlotType::MainWeapon2Slot:
			OnRep_SecondMainWeapon();
			break;
		default:
			break;
		}
	}
}

bool UATGEquipmentComponent::CheckCanMove(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId)
{
	return true;
}

bool UATGEquipmentComponent::CheckItemFitSlot(UATGItemData* ItemData, EEquipmentSlotType SlotType)
{
	if (UATGEquipmentData* EquipData = Cast<UATGEquipmentData>(ItemData))
	{
		switch (SlotType)
		{
		case EEquipmentSlotType::MainWeapon1Slot:
		case EEquipmentSlotType::MainWeapon2Slot:
		{
			UATGWeaponData* WeaponData = Cast<UATGWeaponData>(EquipData);
			return WeaponData && WeaponData->WeaponType == EWeaponType::MainWeapon;
		}
		default:
			break;
		}
	}
	return false;
}
