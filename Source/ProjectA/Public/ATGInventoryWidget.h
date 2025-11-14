// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ATGInventoryWidget.generated.h"

class UATGInventoryGirdWidget;



/**
 * 
 */

UCLASS()
class PROJECTA_API UATGInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UATGInventoryGirdWidget* PlayerGrid = nullptr;

	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UATGInventoryGirdWidget* ContainerGrid = nullptr;

protected:

	virtual void NativeConstruct() override;


	void TogglePlayerGrid(bool bIsVisibie);
	
};
