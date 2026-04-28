// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ATGItemObject.generated.h"

class UATGItemData;

UCLASS(BlueprintType)
class ATGGRIDINVENTORY_API UATGItemObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	int32 EntryID;

	UPROPERTY(BlueprintReadOnly)
	UATGItemData* ItemData;
};
