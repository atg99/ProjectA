// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ATGPlayerState.generated.h"

class UATGInventoryComponent;
class UATGEquipmentComponent;
/**
 * 
 */
UCLASS()
class PROJECTA_API AATGPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	AATGPlayerState();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UATGInventoryComponent* InventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UATGEquipmentComponent* EquipmentComponent;
	
};
