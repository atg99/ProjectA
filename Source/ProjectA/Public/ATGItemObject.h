// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ATGItemObject.generated.h"

/**
 * 
 */
class UATGItemData;

UCLASS(BlueprintType)
class PROJECTA_API UATGItemObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly)
	int32 EntryID; // 서버에서 아이템 찾을 키

	UPROPERTY(BlueprintReadOnly)
	UATGItemData* ItemData; // 아이템 정보
};
