// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_UseItem.h"
#include "Data/ATGConsumableItemData.h"                                                                                          
#include "AbilitySystemComponent.h"                                                                                              
#include "AbilitySystemBlueprintLibrary.h"      
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "ATGInventoryComponent.h"

UGA_UseItem::UGA_UseItem()
{
    // Default settings                                                                                                          
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

const UATGItemObject* UGA_UseItem::GetItemDataFromEvent(const FGameplayEventData& EventData) const
{    
    return nullptr;
}

// Decrease Item Amounts
void UGA_UseItem::ConsumeItem(const FGameplayEventData& EventData)
{

    // Check Authority
    if (!GetOwningActorFromActorInfo()->HasAuthority())
    {
        return;
    }

    const AActor* AvatarActor = GetAvatarActorFromActorInfo();
    const APlayerController* PC = Cast<APlayerController>(AvatarActor->GetOwner());
    if (PC)
    {
        const APlayerState* PS = PC->GetPlayerState<APlayerState>();
        UATGInventoryComponent* InvenComp = PS->GetComponentByClass<UATGInventoryComponent>();
        if (InvenComp)
        {
            int32 DecreaseAmount = 0;
            const UATGConsumableItemData* ConsumableItemData = Cast<const UATGConsumableItemData>(EventData.OptionalObject);
            if (ConsumableItemData)
            {
                DecreaseAmount = ConsumableItemData->ConsumeAmount;
            }
            // call InvenComp decrease Item Qty
            int32 ItemID = FMath::RoundToInt(EventData.EventMagnitude);
            InvenComp->ServerDecreaseItem(ItemID, DecreaseAmount);
        }
    }
}

TArray<FActiveGameplayEffectHandle> UGA_UseItem::ApplyItemEffectsToSelf(const UATGConsumableItemData* ItemData)
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