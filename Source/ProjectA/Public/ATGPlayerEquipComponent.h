// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATGEnum.h"
#include "ATGPlayerEquipComponent.generated.h"

class UATGEquipmentComponent;
class USceneComponent;
class AATGWeaponBase;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTA_API UATGPlayerEquipComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UATGPlayerEquipComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	FATGCharacterInputState ATGCharacterInputState;

	//슬롯의 상태
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TArray<FEquipmentSlot> EquipmentSlots;

	//현재 사용중인 슬롯
	UPROPERTY(ReplicatedUsing = "OnRep_CurrentUsingSlot", BlueprintReadOnly, Category = "Equipment")
	EEquipmentSlotType CurrentUsingSlot = EEquipmentSlotType::MeleeWeapon;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment");
	FName SniperSocketName = TEXT("HandGrip_Sniper");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment");
	FName Main1BackSocketName = TEXT("Main1Back_Sniper");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment");
	FName Main2BackSocketName = TEXT("Main2Back_Sniper");

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
	FTimerHandle ADSTimerHandle;

	bool bReadyToFire = false;

	float MoveRecoveryTime = 0.5f;
protected:

	void InitSlot(EEquipmentSlotType InEquipmentSlotType);
};
