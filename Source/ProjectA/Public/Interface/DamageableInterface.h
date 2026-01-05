// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Engine/HitResult.h"
#include "DamageableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, NotBlueprintable)
class UDamageableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PROJECTA_API IDamageableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	/** Handles damage and knockback events */
	UFUNCTION(BlueprintCallable, Category = "Damageable")
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& DamageLocation, const FVector& DamageImpulse) = 0;

	/** Handles death events */
	UFUNCTION(BlueprintCallable, Category = "Damageable")
	virtual void HandleDeath(const FHitResult& InHitResult) = 0;

	/** Handles healing events */
	UFUNCTION(BlueprintCallable, Category = "Damageable")
	virtual void ApplyHealing(float Healing, AActor* Healer) = 0;
};
