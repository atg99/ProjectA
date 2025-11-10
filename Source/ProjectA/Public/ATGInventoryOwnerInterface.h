// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ATGInventoryOwnerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UATGInventoryOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class PROJECTA_API IATGInventoryOwnerInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

    virtual void ItemRemoved(int32 EntryId) {}

    virtual void ItemAdded(int32 EntryId) {}

	virtual void ItemChanged(int32 EntryId) {}

	virtual void InventoryForceNetUpdate() {}

	virtual bool IsLocallyOwned() { return false; }

};
