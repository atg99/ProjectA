// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ATGInventoryWidget.generated.h"

class UATGInventoryGirdWidget;
class UATGInventoryComponent;
class UATGHUDComponent;

/**
 * 
 */

UCLASS()
class PROJECTA_API UATGInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UATGInventoryWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UATGInventoryGirdWidget* PlayerGrid = nullptr;

	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UATGInventoryGirdWidget* ContainerGrid = nullptr;

	void SetHUDComp(UATGHUDComponent* InHUDComp);

protected:
	UATGHUDComponent* HUDComp = nullptr;

	UFUNCTION()
	void HandleContainerToggle(class UATGContainerComponent* ContainerComp);

	// 인벤토리 소스
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	UATGInventoryComponent* InventoryComponent = nullptr;

	UFUNCTION() 
	void HandleInitInventoryComp(UATGInventoryComponent* GetInventoryComponent);


protected:
	//void HandleItemRotated();
	//드래그관련

	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragEnter(const FGeometry& InGeo, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	/*virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;*/

	void TogglePlayerGrid(bool bIsVisibie);
	

	void InjectInvenComp(UATGInventoryComponent* GetInventoryComponent);
};
