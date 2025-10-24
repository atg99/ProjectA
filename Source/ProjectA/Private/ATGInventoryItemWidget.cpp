// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGInventoryItemWidget.h"

#include "ATGInventoryComponent.h"
#include "ATGItemData.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h" // DetectDragIfPressed
#include "InputCoreTypes.h" // EKeys

void UATGInventoryItemWidget::SetupFromEntry(const FInventoryEntry& InEntry, UATGInventoryComponent* InComp, int32 InCellSize, int32 InCellPadding)
{
	EntryId = InEntry.Id;
	InventoryComp = InComp;
	CachedEntry = InEntry;
	CachedCellSize = InCellSize;
	CachedCellPadding = InCellPadding;


	// 아이콘 & 수량 갱신
	if (ItemIcon)
	{
		if (InEntry.Item && InEntry.Item->Icon)
		{
			ItemIcon->SetBrushFromTexture(InEntry.Item->Icon);
		}
	}

	const float Pitch = float(InCellSize + 2 * InCellPadding);
	const float WidthPx = InEntry.Width * Pitch - 2 * InCellPadding;
	const float HeightPx = InEntry.Height * Pitch - 2 * InCellPadding;

	if (RootSizeBox)
	{
		RootSizeBox->SetWidthOverride(WidthPx);
		RootSizeBox->SetHeightOverride(HeightPx);
	}

	if (QuantityText)
	{
		QuantityText->SetText(FText::AsNumber(InEntry.Quantity));
	}
}

void UATGInventoryItemWidget::RefreshFromEntry(const FInventoryEntry& InEntry, int32 InCellSize, int32 InCellPadding)
{
	CachedEntry = InEntry;
	CachedCellSize = InCellSize;
	CachedCellPadding = InCellPadding;

	const float Pitch = float(InCellSize + 2 * InCellPadding);
	const float WidthPx = InEntry.Width * Pitch - 2 * InCellPadding;
	const float HeightPx = InEntry.Height * Pitch - 2 * InCellPadding;

	if (RootSizeBox)
	{
		RootSizeBox->SetWidthOverride(WidthPx);
		RootSizeBox->SetHeightOverride(HeightPx);
	}

	if (QuantityText)
	{
		QuantityText->SetText(FText::AsNumber(InEntry.Quantity));
	}
	// 회전/크기 변경은 부모 Grid 위젯이 Slot 스팬을 갱신합니다.
}

FReply UATGInventoryItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeo, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}
	return Super::NativeOnMouseButtonDown(InGeo, InMouseEvent);
}

void UATGInventoryItemWidget::NativeOnDragDetected(const FGeometry& InGeo, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UDragDropOperation* Op = NewObject<UDragDropOperation>(this);


	// 드래그 비주얼은 새 인스턴스로 만들어서 원본 위젯 분리/깜빡임 방지
	UATGInventoryItemWidget* Visual = CreateWidget<UATGInventoryItemWidget>(GetOwningPlayer(), GetClass());
	if (Visual)
	{
		Visual->SetupFromEntry(CachedEntry, InventoryComp, CachedCellSize, CachedCellPadding);
		Visual->SetVisibility(ESlateVisibility::HitTestInvisible);
		Op->DefaultDragVisual = Visual;
	}

	Op->Payload = this; // 원본 위젯 참조로 EntryId 접근
	OutOperation = Op;
}