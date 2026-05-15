// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ATGInventoryWidget.generated.h"

class UATGInventoryGridWidget;
class UATGEquipmentGridWidget;
class UATGInventoryComponent;
class UATGHUDComponent;
class UATGEquipmentComponent;

/**
 * 
 */

UCLASS()
class PROJECTA_API UATGInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UATGInventoryGridWidget* PlayerGrid = nullptr;

	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UATGInventoryGridWidget* ContainerGrid = nullptr;

	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UATGEquipmentGridWidget* MainWeapon1Grid = nullptr;

	UPROPERTY(meta = (BindWidget), EditAnywhere, BlueprintReadWrite)
	UATGEquipmentGridWidget* MainWeapon2Grid = nullptr;

	void SetHUDComp(UATGHUDComponent* InHUDComp);
	void RefreshInventoryBindingsFromPlayerState();

protected:
	UATGHUDComponent* HUDComp = nullptr;

	UFUNCTION()
	void HandleContainerToggle(class UATGContainerComponent* ContainerComp);

	// 인벤토리 소스
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	UATGInventoryComponent* InventoryComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	UATGEquipmentComponent* EquipmentComponent = nullptr;

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
	

	void InjectInvenComp(UATGInventoryComponent* InInventoryComponent, UATGEquipmentComponent* InEquipmentComponent);
};
