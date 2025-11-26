// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../Public/InventoryTypes.h"
#include "ATGInventoryOwnerInterface.h"
#include "ATGEnum.h"
#include "ATGEquipmentComponent.generated.h"

struct FInventoryEntry;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentEvent, int32, EntryId);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFirstMainWeaponEvent, FInventoryEntry, FirstMainWeapon);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSecondMainWeaponEvent, FInventoryEntry, SecondMainWeapon);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTA_API UATGEquipmentComponent : public UActorComponent, public IATGInventoryOwnerInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UATGEquipmentComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:
	
	//ItemData의 W,H 관계없이 무기는 2,1크기로 고정 (텍스쳐비율도 2,1 비율)
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

	/*UPROPERTY(Replicated, BlueprintReadOnly)
	FInventoryEntry MainWeapon1;

	UPROPERTY(Replicated, BlueprintReadOnly)
	FInventoryEntry MainWeapon2;

	UPROPERTY(Replicated, BlueprintReadOnly)
	TArray<FInventoryEntry> Equipments;*/

	const FInventoryEntry* GetEquipmentById() const;

public:

	// client UI
	//UPROPERTY(BlueprintAssignable)
	//FOnFirstMainWeaponEvent OnFirstMainWeaponAdded;
	//UPROPERTY(BlueprintAssignable)
	//FOnFirstMainWeaponEvent OnFirstMainWeaponRemoved;
	UPROPERTY(BlueprintAssignable, Category = "Equipment | Weapon")
	FOnFirstMainWeaponEvent OnFirstMainWeaponChanged;

	//UPROPERTY(BlueprintAssignable)
	//FOnSecondMainWeaponEvent OnSecondMainWeaponAdded;
	//UPROPERTY(BlueprintAssignable)
	//FOnSecondMainWeaponEvent OnSecondMainWeaponRemoved;
	UPROPERTY(BlueprintAssignable, Category = "Equipment | Weapon")
	FOnSecondMainWeaponEvent OnSecondMainWeaponChanged;

	//UPROPERTY(BlueprintAssignable)
	FOnEquipmentEvent OnEquipmentAdded;

	//UPROPERTY(BlueprintAssignable)
	FOnEquipmentEvent OnEquipmentRemoved;

	//UPROPERTY(BlueprintAssignable)
	FOnEquipmentEvent OnEquipmentChanged;

	//override

	virtual void ItemRemoved(int32 EntryId) override;

	virtual void ItemAdded(int32 EntryId) override;

	virtual void ItemChanged(int32 EntryId) override;

	//virtual const TArray<struct FInventoryEntry>& GetEntries() override;

	virtual void TryHandleTransItemResult(int32 EntryId, int32 RemoveQty = -1) override;

	virtual void GetEquipmentEntry(EEquipmentSlotType EquipSlotType, FInventoryEntry& OutEntry) override;
	
	virtual void TryAddItemAt(TScriptInterface<IATGInventoryOwnerInterface> Inven, int32 OtherGridId, TSoftObjectPtr<class UATGItemData> ItemDef, int32 Qty, int32 X, int32 Y, bool bRotate = false) override;

	UFUNCTION(Server, Reliable)
	void ServerAddEquipment(const TSoftObjectPtr<class UATGItemData>& ItemDef, uint8 EquipmentSlotType, int32 OtherGridId, const TScriptInterface<IATGInventoryOwnerInterface>& Inven);

	virtual bool CheckCanMove(int32 StartX, int32 StartY, int32 W, int32 H, int32 IgnoreId = -1) override;

	//bool CheckEquipable(const class UATGItemData* ItemData) const;

protected:
	bool CheckItemFitSlot(UATGItemData* ItemData, EEquipmentSlotType SlotType);
	//static int32 GlobalEquipmentIdCounter;
};
