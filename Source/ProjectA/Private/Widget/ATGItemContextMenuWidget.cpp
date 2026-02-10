// Fill out your copyright notice in the Description page of Project Settings.


#include "Widget/ATGItemContextMenuWidget.h"
#include "Components/Button.h"
#include "Data/ATGItemData.h"
#include "Data/ATGConsumableItemData.h"
#include "ATGInventoryComponent.h"

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

void UATGItemContextMenuWidget::InitMenu(UATGItemData* InItemData)
{
    UATGConsumableItemData* ConsumableItemData = Cast<UATGConsumableItemData>(InItemData);

    if (ConsumableItemData)
    {
        Btn_Use->SetVisibility(ESlateVisibility::Visible);
    }
    else
    {
        Btn_Use->SetVisibility(ESlateVisibility::Collapsed);
    }

    CurrentItemData = InItemData;
}

void UATGItemContextMenuWidget::OnUseClicked()
{
    if (CurrentItemData)
    {
        // 아이템 사용 로직 호출
        UATGConsumableItemData* ConsumableItemData = Cast<UATGConsumableItemData>(CurrentItemData);
        InvenComp->UseItem(ConsumableItemData);

        // 메뉴 닫기 (부모에게 요청하거나 스스로 숨김)
        SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UATGItemContextMenuWidget::OnDropClicked()
{
    if (CurrentItemData)
    {
        // 아이템 버리기 로직 호출
        // CurrentItemData->Drop();

        SetVisibility(ESlateVisibility::Collapsed);
    }
}