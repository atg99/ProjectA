// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGInventoryWidget.h"
#include "ATGInventoryGirdWidget.h"

void UATGInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PlayerGrid->InitPlayerGrid();
}

void UATGInventoryWidget::TogglePlayerGrid(bool bIsVisibie)
{
	if (!PlayerGrid) return;

	if (bIsVisibie)
	{
		PlayerGrid->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		PlayerGrid->SetVisibility(ESlateVisibility::Collapsed);
	}
}
