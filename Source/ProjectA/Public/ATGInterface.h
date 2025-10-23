// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ATGEnum.h"
#include "ATGInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, NotBlueprintable)
class UATGInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PROJECTA_API IATGInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void PlayerInteract(FInteractionData& InteractionData) = 0;
};
