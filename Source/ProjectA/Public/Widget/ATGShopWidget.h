// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ATGShopWidget.generated.h"

class UATGInventoryGridWidget;
/**
 * 
 */
UCLASS()
class PROJECTA_API UATGShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UATGInventoryGridWidget* PlayerGrid = nullptr;

	//UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	//UATGInventoryGridWidget* TransactionGrid = nullptr;

	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UATGInventoryGridWidget* MerchantGrid = nullptr;

protected:

	virtual bool NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

};
