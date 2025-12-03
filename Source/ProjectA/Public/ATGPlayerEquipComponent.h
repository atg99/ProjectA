// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATGEnum.h"
#include "ATGPlayerEquipComponent.generated.h"

class UATGEquipmentComponent;
class USceneComponent;

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

	//슬롯의 상태
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TArray<FEquipmentSlot> EquipmentSlots;

	//현재 사용중인 슬롯
	UPROPERTY(ReplicatedUsing = "OnRep_CurrentUsingSlot", BlueprintReadOnly, Category = "Equipment")
	EEquipmentSlotType CurrentUsingSlot = EEquipmentSlotType::None;

	UFUNCTION()
	void OnRep_CurrentUsingSlot();

	void ChangeWeaponEquip();

	UFUNCTION(Server, Reliable)
	void ServerChangePlayerUsingSlot(EEquipmentSlotType TryUsingSlot);

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

	bool CheckPlayerStateCompReady();

	void InitEquipComponent(UATGEquipmentComponent* EquipmentComponent);

	USceneComponent* GetSlaveMesh();
protected:

};
