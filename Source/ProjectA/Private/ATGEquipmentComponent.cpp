// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGEquipmentComponent.h"
#include "Net/UnrealNetwork.h"
#include "ATGItemData.h"
#include "ATGEnum.h"
#include "ATGInventoryOwnerInterface.h"

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

	//ItemData의 W,H 관계없이 무기는 크기 고정 (텍스쳐비율 일치) 위치 0,0 고정
	//각 슬롯마다 ID 고정
	if(GetOwner()->HasAuthority())
	{
		//=========== FirstMainWeapon ==========
		FirstMainWeapon.Id = (int32)EEquipmentSlotType::MainWeapon1;

		FirstMainWeapon.Width = WeaponSlotSize.X;
		FirstMainWeapon.Height = WeaponSlotSize.Y;

		FirstMainWeapon.X = 0;
		FirstMainWeapon.Y = 0;

		//=========== SecondMainWeapon ==========
		SecondMainWeapon.Id = (int32)EEquipmentSlotType::MainWeapon2;

		SecondMainWeapon.Width = WeaponSlotSize.X;
		SecondMainWeapon.Height = WeaponSlotSize.Y;

		SecondMainWeapon.X = 0;
		SecondMainWeapon.Y = 0;
	}
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

	DOREPLIFETIME_CONDITION(UATGEquipmentComponent, FirstMainWeapon, COND_None);
	DOREPLIFETIME_CONDITION(UATGEquipmentComponent, SecondMainWeapon, COND_None);

}

//변경시 클라에 전파
void UATGEquipmentComponent::OnRep_FirstMainWeapon()
{	//위젯과 플레어어가 구독
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

//사용안함
//const TArray<struct FInventoryEntry>& UATGEquipmentComponent::GetEntries()
//{
//	UE_LOG(LogTemp, Warning, TEXT("!!! Don't Use UATGEquipmentComponent::GetEntries() Interface !!!"))
//	TArray<FInventoryEntry> TrashEntry;
//	TrashEntry.Empty();
//	return TrashEntry;
//}

void UATGEquipmentComponent::TryHandleTransItemResult(int32 EntryId, int32 RemoveQty)
{
	if (RemoveQty < 1)
	{
		return;
	}

	switch (EntryId)
	{
	case (int32)EEquipmentSlotType::MainWeapon1:
		FirstMainWeapon.Item = nullptr;
		break;
	case (int32)EEquipmentSlotType::MainWeapon2:
		SecondMainWeapon.Item = nullptr;
		break;
	default:
		UE_LOG(LogTemp, Warning, TEXT("!!! UATGEquipmentComponent::TryHandleTransItemResult Invaild EntryId"));
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
	case EEquipmentSlotType::MainWeapon1:
		OutEntry = FirstMainWeapon;
		break;
	case EEquipmentSlotType::MainWeapon2:
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
	if (UATGItemData* ItemData = ItemDef.LoadSynchronous())
	{
		if (ItemData->ItemType != EItemType::Equipment)
		{
			return;
		}
	}

	FInventoryEntry* TargetEntry = nullptr;

	EEquipmentSlotType SlotType = (EEquipmentSlotType)EquipmentSlotType;

	if (SlotType == EEquipmentSlotType::MainWeapon1)
	{
		TargetEntry = &FirstMainWeapon;
	}
	else if (SlotType == EEquipmentSlotType::MainWeapon2)
	{
		TargetEntry = &SecondMainWeapon;
	}

	if (TargetEntry)
	{
		UE_LOG(LogTemp, Display, TEXT("UATGEquipmentComponent::ServerAddEquipment"));

		//아이템 복사
		TargetEntry->Item = ItemDef;
		TargetEntry->Quantity = 1;

		//TargetEntry->X = 0;
		//TargetEntry->Y = 0;
		//TargetEntry->Id = ++GlobalEquipmentIdCounter;

		// 원래 인벤토리에서 제거 요청 무기 장착은 슬롯 하나당 1개만 가능 하므로 1개 제거 (무기가 스택이 쌓였을 수도 있음)
		Inven->TryHandleTransItemResult(OtherGridId, 1);
		GetOwner()->ForceNetUpdate();

		switch (SlotType)
		{
		case EEquipmentSlotType::None:
			break;
		case EEquipmentSlotType::MainWeapon1:
			OnFirstMainWeaponChanged.Broadcast(FirstMainWeapon);
			break;
		case EEquipmentSlotType::MainWeapon2:
			OnSecondMainWeaponChanged.Broadcast(SecondMainWeapon);
			break;
		default:
			break;
		}
	}

	
}

bool UATGEquipmentComponent::CheckCanMove(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId)
{
	// 단순 범위 체크 혹은 항상 true (슬롯이 고정되어 있으므로)
	// 여기서는 일단 true 리턴 (위젯에서 CheckEquipable로 타입 체크함)
	return true;
}

bool UATGEquipmentComponent::CheckEquipable(const UATGItemData* ItemData) const
{
	if (!ItemData) return false;
	
	return ItemData->EquipmentType == EEquipmentType::Weapon;
}

