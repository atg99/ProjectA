// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ATGPlayerController.generated.h"

class UInputMappingContext;
class UATGInventoryGirdWidget;

/**
 * 
 */
UCLASS()
class PROJECTA_API AATGPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;
public:

	virtual void BeginPlay() override;

protected:

	virtual void OnRep_PlayerState() override;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UATGInventoryGirdWidget> InventoryWidgetClass;

	UPROPERTY(Transient)
	UATGInventoryGirdWidget* InventoryWidget = nullptr;

	void EnsureWidgetCreated();

	//void SetupUIMode(bool bShowMouse);
	
};
