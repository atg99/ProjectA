// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ATGInventoryWidget.generated.h"

class UATGInventoryGirdWidget;
class UATGInventoryComponent;

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

	// 인벤토리 소스
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	UATGInventoryComponent* InventoryComponent = nullptr;

	UFUNCTION() 
	void HandleInitInventoryComp(UATGInventoryComponent* GetInventoryComponent);


protected:

	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;


	void TogglePlayerGrid(bool bIsVisibie);
	

	void InjectInvenComp(UATGInventoryComponent* GetInventoryComponent);
};
