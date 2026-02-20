// Fill out your copyright notice in the Description page of Project Settings.


#include "Animation/NotifyState_MeleeAttack.h"
#include "ATGPlayerEquipComponent.h"
#include "Weapon/ATGMeleeWeapon.h"
#include "Interface/MeleeWeaponInterface.h"

FString UNotifyState_MeleeAttack::GetNotifyName_Implementation() const
{
	return FString("MeleeAttackWindow");
}

void UNotifyState_MeleeAttack::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (IMeleeWeaponInterface* Interface = GetWeaponInterface(MeshComp))
	{
		Interface->StartHitCheck();
	}
}

void UNotifyState_MeleeAttack::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	if (IMeleeWeaponInterface* Interface = GetWeaponInterface(MeshComp))
	{
		Interface->TickHitCheck();
	}
}

void UNotifyState_MeleeAttack::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (IMeleeWeaponInterface* Interface = GetWeaponInterface(MeshComp))
	{
		Interface->EndHitCheck();
	}
}

IMeleeWeaponInterface* UNotifyState_MeleeAttack::GetWeaponInterface(USkeletalMeshComponent* MeshComp)
{
	if (IMeleeWeaponInterface* OwnerInterface = Cast<IMeleeWeaponInterface>(MeshComp->GetOwner()))
	{
		return OwnerInterface;
	}

	if (UATGPlayerEquipComponent* EquipComp = Cast<UATGPlayerEquipComponent>(MeshComp->GetOwner()->GetComponentByClass(UATGPlayerEquipComponent::StaticClass())))
	{
		if (FEquipmentSlot* Slot = EquipComp->GetSlotByType(EquipComp->CurrentUsingSlot))
		{
			return Cast<IMeleeWeaponInterface>(Slot->EquippedActor);
		}
	}
	return nullptr;
}
