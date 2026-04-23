// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/ATGItemContextMenuWidget.h"
#include "Components/Button.h"
#include "Data/ATGItemData.h"
#include "Data/ATGConsumableItemData.h"
#include "ATGInventoryComponent.h"
#include "ATGItemObject.h"
#include "ATGInventoryItemWidget.h"

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
        ensure(ItemData);
    }

    UATGConsumableItemData* ConsumableItemData = Cast<UATGConsumableItemData>(ItemData);

    if (ConsumableItemData)
    {
        Btn_Use->SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        Btn_Use->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UATGItemContextMenuWidget::OnUseClicked()
{
    if (IsValid(CurrentItemInfo.ItemWidget))
    {
        InvenComp->UseItem(CurrentItemInfo);

        SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UATGItemContextMenuWidget::OnDropClicked()
{
    if (IsValid(CurrentItemInfo.ItemWidget))
    {
        // CurrentItemData->Drop();

        SetVisibility(ESlateVisibility::Collapsed);
    }
}