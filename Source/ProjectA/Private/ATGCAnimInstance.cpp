// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGCAnimInstance.h"
#include "ATGPlayerEquipComponent.h"

void UATGCAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UATGCAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{	
	Super::NativeUpdateAnimation(DeltaSeconds);

	UATGPlayerEquipComponent* PlayerEquip = TryGetPawnOwner() ? TryGetPawnOwner()->GetComponentByClass<UATGPlayerEquipComponent>() : nullptr;
	if (PlayerEquip)
	{
		CurrentEquippedWeaponSlotType = PlayerEquip->CurrentUsingSlot;
	}
}
