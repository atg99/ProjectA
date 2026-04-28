// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Data/ATGEquipmentData.h"
#include "GameFramework/DamageType.h"
#include "ATGWeaponData.generated.h"

USTRUCT(BlueprintType)
struct ATGGRIDINVENTORY_API FWeaponAbilityBind
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EPlayerAbilityInputID InputID = EPlayerAbilityInputID::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayAbility> AbilityClass;
};

class AATGWeaponBase;

UCLASS(Abstract)
class ATGGRIDINVENTORY_API UATGWeaponData : public UATGEquipmentData
{
	GENERATED_BODY()

public:
	UATGWeaponData();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EWeaponType WeaponType = EWeaponType::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EWeaponCategory WeaponCategory = EWeaponCategory::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<USkeletalMesh> WeaponSkeletalMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Socket")
	FName HandSocketName = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Socket")
	FName Main1BackSocketName = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Socket")
	FName Main2BackSocketName = TEXT("");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Socket")
	FName SubBackSocketName = TEXT("");

	virtual FName GetSocketName(EEquipmentSlotType SlotType, bool bIsEquipped) const;

	virtual TSubclassOf<AATGWeaponBase> GetWeaponClass() const PURE_VIRTUAL(UATGWeaponData::GetWeaponClass, return nullptr;);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSubclassOf<UDamageType> DamageTypeClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS")
	TArray<FWeaponAbilityBind> WeaponAbilitys;
};
