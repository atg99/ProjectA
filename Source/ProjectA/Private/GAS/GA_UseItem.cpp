// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_UseItem.h"

#include "Data/ATGConsumableItemData.h"                                                                                          
#include "AbilitySystemComponent.h"                                                                                              
#include "AbilitySystemBlueprintLibrary.h"                                                                                       

UGA_UseItem::UGA_UseItem()
{
    // Default settings                                                                                                          
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

const UATGConsumableItemData* UGA_UseItem::GetItemDataFromEvent(const FGameplayEventData& EventData) const
{    
    return Cast<const UATGConsumableItemData>(EventData.OptionalObject);
}

TArray<FActiveGameplayEffectHandle> UGA_UseItem::ApplyItemEffects(const UATGConsumableItemData* ItemData)
{
    TArray<FActiveGameplayEffectHandle> ActiveHandles;

    if (!ItemData)
    {
        return ActiveHandles;
    }

    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    if (!AvatarActor)
    {
        return ActiveHandles;
    }

    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (!ASC)
    {
        return ActiveHandles;
    }

    // Loop through all effects defined in ItemData                                                                              
    for (const FItemEffectInfo& EffectInfo : ItemData->ItemEffects)
    {
        if (!EffectInfo.EffectClass)
        {
            continue;
        }

        // Create the GameplayEffect Spec                                                                                        
        FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
        ContextHandle.AddSourceObject(this);

        FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectInfo.EffectClass, GetAbilityLevel(), ContextHandle);

        if (SpecHandle.IsValid())
        {
            // Set the SetByCaller magnitude using the tag defined in data                                                       
            if (EffectInfo.DataTag.IsValid())
            {
                SpecHandle.Data.Get()->SetSetByCallerMagnitude(EffectInfo.DataTag, EffectInfo.Magnitude);
            }

            // Apply the effect                                                                                                  
            FActiveGameplayEffectHandle ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
            ActiveHandles.Add(ActiveHandle);
        }
    }

    return ActiveHandles;
}