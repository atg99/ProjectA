// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ATGBTInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UATGBTInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PROJECTA_API IATGBTInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual float TryPlayMontage(UAnimMontage* Montage, float PlayRate = 1.f, FName StartSessionName = NAME_None) { return 0; }

	virtual void TryStopMontage(UAnimMontage* Montage) {}
};
