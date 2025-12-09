// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGCAnimInstance.h"
#include "ATGEnum.h"
#include "ATGPlayerEquipComponent.h"

void UATGCAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UATGCAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{	
	Super::NativeUpdateAnimation(DeltaSeconds);

	UATGPlayerEquipComponent* PlayerEquip = TryGetPawnOwner() ? TryGetPawnOwner()->GetComponentByClass<UATGPlayerEquipComponent>() : nullptr;
	if (IsValid(PlayerEquip))
	{
		CurrentEquippedWeaponSlotType = PlayerEquip->CurrentUsingSlot;
		FRotator AimOffset = GetAimOffset();
		AOYaw = AimOffset.Yaw;
		AOPitch = AimOffset.Pitch;
	}
}

FRotator UATGCAnimInstance::GetAimOffset() const
{
	return TryGetPawnOwner()->ActorToWorld().InverseTransformVectorNoScale(TryGetPawnOwner()->GetBaseAimRotation().Vector()).Rotation();
}
