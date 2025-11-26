// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ATGEnum.h"
#include "ATGPlayerEquipComponent.generated.h"

class UATGEquipmentComponent;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment")
	TArray<FEquipmentSlot> EquipmentSlots;

protected:
	FTimerHandle TimerHandle_InitCheck;

	ACharacter* GetOwningPlayerCharacter();

	UFUNCTION()
	void HandleFirstMainWeaponChanged(FInventoryEntry InFirstMainWeapon);

	UFUNCTION()
	void HandleSecondMainWeaponChanged(FInventoryEntry InSecondMainWeapon);

	bool CheckPlayerStateCompReady();

	void InitEquipComponent(UATGEquipmentComponent* EquipmentComponent);
};
