// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_MeleeCombo.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTA_API UGA_MeleeCombo : public UGameplayAbility
{
	GENERATED_BODY()
	
public:

	UGA_MeleeCombo();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
    UPROPERTY(EditAnywhere)
    UAnimMontage* MeleeMontage;

    UPROPERTY(EditAnywhere)
    TArray<FName> ComboSections;

    UPROPERTY(EditAnywhere)
    FGameplayTag ComboTag;

    int32 CurrentComboIndex;

    void WaitForNextInput();
    void WaitForHitTask();

    UFUNCTION()
    void OnInputPressed(float TimeWaited);

    UFUNCTION()
    void OnMontageEnded();

    UFUNCTION()
    void OnHitReceived(const FGameplayAbilityTargetDataHandle& Data);

protected:
 
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    FGameplayTag HitEventTag;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    TSubclassOf<UGameplayEffect> MeleeGameplayEffectClass;
};
