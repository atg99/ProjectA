// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ATGInventoryInterface.h"
#include "ATGItemContextMenuWidget.generated.h"

class UButton;
class UATGInventoryComponent;

UCLASS()
class ATGGRIDINVENTORY_API UATGItemContextMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void InitMenu(FATGItemInfo& ItemInfo);

	UPROPERTY()
	TObjectPtr<UATGInventoryComponent> InvenComp = nullptr;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Use = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Drop = nullptr;

private:
	UFUNCTION()
	void OnUseClicked();

	UFUNCTION()
	void OnDropClicked();

	UPROPERTY()
	FATGItemInfo CurrentItemInfo;
};
