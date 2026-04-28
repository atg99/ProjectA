// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGHUDWidget.h"
#include "Kismet/GameplayStatics.h"
#include "ATGHUDComponent.h"
#include "ATGInventoryWidget.h"

void UATGHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HUDComp)
	{
		HUDComp->OnInventToggle.RemoveDynamic(this, &UATGHUDWidget::OnToggleInvent);
		HUDComp->OnInventToggle.AddDynamic(this, &UATGHUDWidget::OnToggleInvent);

		//인벤토리에 주입
		if (InventoryWidget)
		{
			InventoryWidget->SetHUDComp(HUDComp);
			InventoryWidget->RefreshInventoryBindingsFromPlayerState();
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
