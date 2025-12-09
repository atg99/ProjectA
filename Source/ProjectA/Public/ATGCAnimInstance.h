// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ATGEnum.h"
#include "ATGCAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTA_API UATGCAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	EEquipmentSlotType CurrentEquippedWeaponSlotType = EEquipmentSlotType::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	float AOYaw;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weapon")
	float AOPitch;

	FRotator GetAimOffset() const;
};
