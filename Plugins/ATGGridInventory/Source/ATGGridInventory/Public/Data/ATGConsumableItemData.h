// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Data/ATGItemData.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "ATGConsumableItemData.generated.h"

class UAnimMontage;

USTRUCT(BlueprintType)
struct ATGGRIDINVENTORY_API FItemEffectInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag DataTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Magnitude = 0.f;
};

UCLASS()
class ATGGRIDINVENTORY_API UATGConsumableItemData : public UATGItemData
{
	GENERATED_BODY()

public:
	UATGConsumableItemData();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 ConsumeAmount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Use")
	FGameplayTag AbilityTriggerTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Use")
	TArray<FItemEffectInfo> ItemEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Use")
	TObjectPtr<UAnimMontage> UseMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Use | Throw")
	TSubclassOf<AActor> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Use | Throw")
	float ThrowSpeed = 1000.f;
};
