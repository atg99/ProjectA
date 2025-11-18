// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGHUDWidget.h"
#include "Kismet/GameplayStatics.h"
#include "ATGHUDComponent.h"
#include "ATGInventoryWidget.h"

void UATGHUDWidget::NativeConstruct()
{
	if (HUDComp)
	{
		HUDComp->OnInventToggle.AddDynamic(this, &UATGHUDWidget::OnToggleInvent);

		//인벤토리에 주입
		if (InventoryWidget)
		{
			InventoryWidget->SetHUDComp(HUDComp);
		}
	}
}

void UATGHUDWidget::OnToggleInvent(bool bVisible)
{
	if (!InventoryWidget)
	{
		return;
	}

	if (bVisible)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}
