// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryTypes.h"
#include "ATGInventoryOwnerInterface.h"
#include "ATGEquipmentComponent.generated.h"

struct FInventoryEntry;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentEvent, int32, EntryId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFirstMainWeaponEvent, FInventoryEntry, FirstMainWeapon);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSecondMainWeaponEvent, FInventoryEntry, SecondMainWeapon);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ATGGRIDINVENTORY_API UATGEquipmentComponent : public UActorComponent, public IATGInventoryOwnerInterface
{
	GENERATED_BODY()

public:
	UATGEquipmentComponent();

protected:
	virtual void BeginPlay() override;

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_FirstMainWeapon, Category = "Equipment | Weapon")
	FInventoryEntry FirstMainWeapon;

	UFUNCTION()
	void OnRep_FirstMainWeapon();

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_SecondMainWeapon, Category = "Equipment | Weapon")
	FInventoryEntry SecondMainWeapon;

	UFUNCTION()
	void OnRep_SecondMainWeapon();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment | Weapon")
	FIntPoint WeaponSlotSize = FIntPoint(2, 1);


public:

	UPROPERTY(BlueprintAssignable, Category = "Equipment | Weapon")
	FOnFirstMainWeaponEvent OnFirstMainWeaponChanged;

	UPROPERTY(BlueprintAssignable, Category = "Equipment | Weapon")
	FOnSecondMainWeaponEvent OnSecondMainWeaponChanged;

	FOnEquipmentEvent OnEquipmentAdded;
	FOnEquipmentEvent OnEquipmentRemoved;
	FOnEquipmentEvent OnEquipmentChanged;

	virtual void ItemRemoved(int32 EntryId) override;
	virtual void ItemAdded(int32 EntryId) override;
	virtual void ItemChanged(int32 EntryId) override;

	virtual void TryHandleTransItemResult(int32 EntryId, int32 RemoveQty = -1) override;

	virtual void GetEquipmentEntry(EEquipmentSlotType EquipSlotType, FInventoryEntry& OutEntry) override;

	virtual void TryAddItemAt(TScriptInterface<IATGInventoryOwnerInterface> Inven, int32 OtherGridId, TSoftObjectPtr<class UATGItemData> ItemDef, int32 Qty, int32 X, int32 Y, bool bRotate = false) override;

	UFUNCTION(Server, Reliable)
	void ServerAddEquipment(const TSoftObjectPtr<class UATGItemData>& ItemDef, uint8 EquipmentSlotType, int32 OtherGridId, const TScriptInterface<IATGInventoryOwnerInterface>& Inven);

	virtual bool CheckCanMove(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId = -1) override;

protected:
	bool CheckItemFitSlot(UATGItemData* ItemData, EEquipmentSlotType SlotType);
};
