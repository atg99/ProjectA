// Fill out your copyright notice in the Description page of Project Settings.

#include "Widget/ATGItemContextMenuWidget.h"

#include "ATGInventoryComponent.h"
#include "ATGInventoryItemWidget.h"
#include "Components/Button.h"
#include "Data/ATGItemData.h"

void UATGItemContextMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Use)
	{
		Btn_Use->OnClicked.AddDynamic(this, &UATGItemContextMenuWidget::OnUseClicked);
	}
	if (Btn_Drop)
	{
		Btn_Drop->OnClicked.AddDynamic(this, &UATGItemContextMenuWidget::OnDropClicked);
	}
}

void UATGItemContextMenuWidget::InitMenu(FATGItemInfo& ItemInfo)
{
	CurrentItemInfo = ItemInfo;

	UATGItemData* ItemData = ItemInfo.ItemDef.Get();
	if (!ItemData)
	{
		ItemData = ItemInfo.ItemDef.LoadSynchronous();
	}

	if (Btn_Use)
	{
		const bool bCanUse = ItemData && ItemData->ItemType == EItemType::Consumables;
		Btn_Use->SetVisibility(bCanUse ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UATGItemContextMenuWidget::OnUseClicked()
{
	if (InvenComp && IsValid(CurrentItemInfo.ItemWidget))
	{
		InvenComp->UseItem(CurrentItemInfo);
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UATGItemContextMenuWidget::OnDropClicked()
{
	if (InvenComp && IsValid(CurrentItemInfo.ItemWidget))
	{
		InvenComp->TryDropItem(CurrentItemInfo.EntryId);
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
