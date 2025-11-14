// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ATGHUDWidget.generated.h"

class UATGInventoryWidget;
class UATGHUDComponent;
/**
 * 
 */
UCLASS()
class PROJECTA_API UATGHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UATGInventoryWidget* InventoryWidget = nullptr;

	UATGHUDComponent* HUDComp = nullptr;
	
protected:

	UFUNCTION()
	void OnToggleInvent(bool bVisible);
};
