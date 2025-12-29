// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DamageEvents.h"
//#include "CustomDamageEvents.generated.h"
/**
 * 
 */

#define GUN_POINT_DAMAGE_ID 102

//USTRUCT()
struct FGunPointDamageEvent : public FPointDamageEvent
{
	//GENERATED_BODY()

	FGunPointDamageEvent()
		: FPointDamageEvent(), ImpulseScale(1.f), OwnedTags(FGameplayTagContainer())
	{
	}

	FGunPointDamageEvent(FGameplayTagContainer InOwnedTags, float InDamage, float InImpulseScale, const FHitResult& InHitInfo, FVector const& InShotDirection, TSubclassOf<UDamageType> InDamageTypeClass)
		: FPointDamageEvent(InDamage, InHitInfo, InShotDirection, InDamageTypeClass), ImpulseScale(InImpulseScale), OwnedTags(InOwnedTags)
	{
	}

	float ImpulseScale;

	/** ID for this class. NOTE this must be unique for all damage events. */
	static const int32 ClassID = GUN_POINT_DAMAGE_ID;

	//GAS Tag
	FGameplayTagContainer OwnedTags;

	virtual int32 GetTypeID() const override { return FGunPointDamageEvent::ClassID; };
	virtual bool IsOfType(int32 InID) const override { return (FGunPointDamageEvent::ClassID == InID) || FPointDamageEvent::IsOfType(InID); };
};

