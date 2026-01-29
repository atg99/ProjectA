// Fill out your copyright notice in the Description page of Project Settings.


#include "ATGInventoryItemWidget.h"
#include "ATGDragDropOperation.h"
#include "ATGInventoryComponent.h"
#include "Data/ATGItemData.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h" // DetectDragIfPressed
#include "InputCoreTypes.h" // EKeys

void UATGInventoryItemWidget::SetupFromEntry(const TScriptInterface<IATGInventoryOwnerInterface> InInven, const FInventoryEntry& InEntry, int32 InCellSize, int32 InCellPadding)
{
	EntryId = InEntry.Id;
	ItemDef = InEntry.Item;
	Inven = InInven;
	CachedEntry = InEntry;
	CachedCellSize = InCellSize;
	CachedCellPadding = InCellPadding;
	Quantity = InEntry.Quantity;
	bIsRotated = InEntry.bRotated;

	// 거래소 : 재화, 물건, 판매,구매 

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
		//QuantityText->SetText(FText::AsNumber(InEntry.Quantity));
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

	UE_LOG(LogTemp, Warning, TEXT("UATGInventoryItemWidget::RefreshFromEntry W : %d , H : %d"), InEntry.Width, InEntry.Height)

	if (RootSizeBox)
	{
		RootSizeBox->SetWidthOverride(WidthPx);
		RootSizeBox->SetHeightOverride(HeightPx);
	}

	Quantity = InEntry.Quantity;
	bIsRotated = InEntry.bRotated;
	if (QuantityText)
	{
		//QuantityText->SetText(FText::AsNumber(InEntry.Quantity));
	}
	// 회전/크기 변경은 부모 Grid 위젯이 Slot 스팬을 갱신합니다.
}

FReply UATGInventoryItemWidget::NativeOnMouseButtonDown(const FGeometry& InGeo, const FPointerEvent& InMouseEvent)
{
	if (OnItemPressed.IsBound())
	{
		FATGItemInfo Info;
		Info.ItemDef = ItemDef;
		Info.Quantity = Quantity;
		Info.bIsRotated = bIsRotated;
		Info.EntryId = EntryId;

		OnItemPressed.Broadcast(Info);
	}

	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}
	return Super::NativeOnMouseButtonDown(InGeo, InMouseEvent);
}

void UATGInventoryItemWidget::NativeOnDragDetected(const FGeometry& InGeo, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	UATGDragDropOperation* Op = NewObject<UATGDragDropOperation>(this);

	// 드래그 비주얼은 새 인스턴스로 만들어서 원본 위젯 분리/깜빡임 방지
	UATGInventoryItemWidget* Visual = CreateWidget<UATGInventoryItemWidget>(GetOwningPlayer(), GetClass());
	if (Visual)
	{
		Visual->SetupFromEntry(Inven, CachedEntry, CachedCellSize, CachedCellPadding);
		Visual->SetVisibility(ESlateVisibility::HitTestInvisible);
		Visual->SetRenderOpacity(0.4f);
		Op->DefaultDragVisual = Visual;
		Op->Pivot = EDragPivot::TopLeft;
	}

	Op->Payload = this; // 원본 위젯 참조로 EntryId 접근
	OutOperation = Op;
}