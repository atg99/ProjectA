// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Data/ATGConsumableItemData.h"  
#include "GA_UseItem.generated.h"

/**
 * 
 */
class UATGItemObject;

UCLASS()
class PROJECTA_API UGA_UseItem : public UGameplayAbility
{
	GENERATED_BODY()

public:
    UGA_UseItem();

    /**
     * Extracts UATGConsumableItemData from the GameplayEventData.
     * Expects the ItemData to be passed in OptionalObject.
     */
    UFUNCTION(BlueprintCallable, Category = "Item")
    const UATGItemObject* GetItemDataFromEvent(const FGameplayEventData& EventData) const;

    /**
     * Applies the effects defined in the ItemData to the owner.
     * Typically called from Blueprint after animation/delay.
     */
    UFUNCTION(BlueprintCallable, Category = "Item")
    TArray<FActiveGameplayEffectHandle> ApplyItemEffectsToSelf(const UATGConsumableItemData* ItemData);

    UFUNCTION(BlueprintCallable, Category = "Item")
    void ConsumeItem(const FGameplayEventData& EventData);
};
