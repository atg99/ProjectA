// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ATGEnum.h"
#include "ATGPlayerController.generated.h"

class UInputMappingContext;
class UATGInventoryGirdWidget;
class UATGInventoryComponent;
/**
 * 
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInitOnInventoryComponent, UATGInventoryComponent*, InvenComponent);


UCLASS()
class PROJECTA_API AATGPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	UInputMappingContext* MotionMatcingContexts;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* InventoryMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* GunWeaponMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* MeleeWeaponMappingContexts;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;
public:

	virtual void BeginPlay() override;

protected:

	virtual void OnRep_PlayerState() override;

	void StartInitInventoryWidget();

protected:

	//UPROPERTY(EditDefaultsOnly, Category = "UI")
	//TSubclassOf<UATGInventoryGirdWidget> InventoryWidgetClass;

	//UPROPERTY(Transient)
	//UATGInventoryGirdWidget* InventoryWidget = nullptr;

	//void EnsureWidgetCreated();

	//void SetupUIMode(bool bShowMouse);

	

public:
	FInitOnInventoryComponent InitInventoryComponent;

	class UATGInventoryComponent* InvenComp = nullptr;

	void ToggleInventoryInputMapping(bool bIsInvent);

	void WeaponInputMapping(EEquipmentSlotType CurrentUsingSlot);
	//void ToggleInventoryUI();
	
};
