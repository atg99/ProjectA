// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATGEnum.h"
#include "ATGPlayerEquipComponent.generated.h"

class UATGEquipmentComponent;
class USceneComponent;
class AATGWeaponBase;
class UATGRangeWeaponData;
class UATGMeleeWeaponData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipWeapon, EWeaponCategory, WeaponCategory);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTA_API UATGPlayerEquipComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UATGPlayerEquipComponent();

protected:
	virtual void BeginPlay() override;

public:
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	FATGCharacterInputState ATGCharacterInputState;

	// Replicated equipment slot table.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
	TArray<FEquipmentSlot> EquipmentSlots;

	// Slot currently controlled by player input.
	UPROPERTY(ReplicatedUsing = "OnRep_CurrentUsingSlot", BlueprintReadOnly, Category = "Equipment")
	EEquipmentSlotType CurrentUsingSlot = EEquipmentSlotType::None;

	UPROPERTY(BlueprintAssignable)
	FOnEquipWeapon OnEquipWeapon;

	UFUNCTION()
	void OnRep_CurrentUsingSlot();

	void ChangeWeaponEquip();

	void TryChangePlayerUsingSlot(EEquipmentSlotType DesiredSlot);

	UFUNCTION(Server, Reliable)
	void ServerChangePlayerUsingSlot(EEquipmentSlotType DesiredSlot);

	void ChangePlayerUsingSlot(EEquipmentSlotType DesiredSlot);

	void TryFire();

	UFUNCTION(Server, Reliable)
	void ServerDoFire();

	UFUNCTION()
	void DoFire();

	void TryWeaponFire();

	void WeaponFire(AATGWeaponBase* WeaponBase);

	UFUNCTION(BlueprintCallable)
	void ReadyToFire();

	UFUNCTION(BlueprintCallable)
	void ReleaseAim();

	float GetReadyToFireTime();

	UPROPERTY(BlueprintReadWrite)
	ECGait CGait = ECGait::Walk;

protected:

	FTimerHandle TimerHandle_InitCheck;

	ACharacter* GetOwningPlayerCharacter();

	UFUNCTION()
	void HandleFirstMainWeaponChanged(FInventoryEntry InFirstMainWeapon);

	UFUNCTION()
	void HandleSecondMainWeaponChanged(FInventoryEntry InSecondMainWeapon);

	void ClearSlot(FEquipmentSlot& Slot);

	bool CheckPlayerStateCompReady();

	void InitEquipComponent(UATGEquipmentComponent* EquipmentComponent);

	USceneComponent* GetSlaveMesh();

	FTimerHandle STFTimerHandle;
	FTimerHandle FireToMoveTimerHandle;

	bool bReadyToFire = false;

	float MoveRecoveryTime = 0.5f;
protected:

	void InitSlot(EEquipmentSlotType InEquipmentSlotType);

	void SpawnRangeWeaponInSlot(FEquipmentSlot& Slot, UATGRangeWeaponData* RangeWeaponData);

	void SpawnMeleeWeaponInSlot(FEquipmentSlot& Slot, UATGMeleeWeaponData* MeleeWeaponData);

public:
	FEquipmentSlot* GetSlotByType(EEquipmentSlotType SlotType);

	UFUNCTION(BlueprintCallable)
	EWeaponCategory GetCurrentEquippedWeaponCategory();

};
