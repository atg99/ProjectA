// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "ATGDamageType.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EDamageType : uint8
{
	None = 0	UMETA(DisplayName = "None"),
	Normal = 10	UMETA(DisplayName = "Normal"),
	Fire = 20	UMETA(DisplayName = "Fire"),
	Ice = 30	UMETA(DisplayName = "Ice"),
};

UCLASS()
class PROJECTA_API UATGDamageType : public UDamageType
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = DamageType)
	EDamageType DamageType = EDamageType::Normal;	
	
};
